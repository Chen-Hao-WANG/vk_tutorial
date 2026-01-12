#pragma once
#include <assert.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include "camera.hpp"

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

constexpr uint32_t WIDTH           = 800;
constexpr uint32_t HEIGHT          = 600;
const std::string MODEL_PATH       = "../../../../model/bunny.obj";
const std::string TEXTURE_PATH     = "../../../../textures/viking_room.png";
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<char const*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef ENABLE_VALIDATION_LAYERS
#pragma message("ENABLE_VALIDATION_LAYERS is defined")
constexpr bool enableValidationLayers = true;
#else
#pragma message("ENABLE_VALIDATION_LAYERS is NOT defined")
constexpr bool enableValidationLayers = false;
#endif
class SDLException final : public std::runtime_error {
   public:
    explicit SDLException(const std::string& message) : std::runtime_error(std::format("{}: {}", message, SDL_GetError())) {}
};
struct BufferResource {
    vk::raii::Buffer buffer       = nullptr;
    vk::raii::DeviceMemory memory = nullptr;
    vk::DeviceSize size           = 0;
    void* mapped                  = nullptr;  // for vulkan persistent mapped memory
};
/**
 * @brief Texture structure to hold texture image, its memory, image view and sampler
 *
 */
struct Texture {
    // texture image and its memory
    vk::raii::Image textureImage              = nullptr;
    vk::raii::DeviceMemory textureImageMemory = nullptr;
    vk::raii::ImageView textureImageView      = nullptr;
    vk::raii::Sampler textureSampler          = nullptr;
};

/**
 * @brief SubMesh structure to hold information about a subset of a mesh
 *
 */
struct SubMesh {
    uint32_t indexOffset;
    uint32_t indexCount;
    uint32_t maxVertex;
    bool alphaCut = false;
};
/**
 * @brief
 *
 */
struct Light {
    glm::vec4 position;
    glm::vec4 color;
};
/**
 * @brief Vertex data structure
 *
 */
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
    }

    /**
     * @brief Get the Binding Description object: binding, stride, inputRate
     *
     * @return vk::VertexInputBindingDescription
     */
    static vk::VertexInputBindingDescription getBindingDescription() { return {0, sizeof(Vertex), vk::VertexInputRate::eVertex}; }
    /**
     * @brief Get the Attribute Descriptions object: pos, color, texCoord, normal
     *
     * @return std::array<vk::VertexInputAttributeDescription, 4>
     */
    static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions() {
        return {vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
                vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
                vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)),
                vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal))};
    }
};
namespace std {
template <>
struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
        size_t h = std::hash<glm::vec3>()(vertex.pos) ^ (std::hash<glm::vec3>()(vertex.color) << 1);
        h        = (h >> 1) ^ (std::hash<glm::vec2>()(vertex.texCoord) << 1);
        h        = (h >> 1) ^ (std::hash<glm::vec3>()(vertex.normal) << 1);
        return h;
    }
};
}  // namespace std
struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};
struct MeshPushConstants {
    glm::mat4 modelMatrix;
};
class HelloTriangleApplication {
    bool running = true;

   public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

   private:
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{nullptr, SDL_DestroyWindow};
    vk::raii::Context context;
    vk::raii::Instance instance                     = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::SurfaceKHR surface                    = nullptr;
    vk::raii::PhysicalDevice physicalDevice         = nullptr;
    vk::raii::Device device                         = nullptr;
    uint32_t queueIndex                             = ~0;
    vk::raii::Queue queue                           = nullptr;
    vk::raii::SwapchainKHR swapChain                = nullptr;
    std::vector<vk::Image> swapChainImages;
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::raii::ImageView> swapChainImageViews;
    //
    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    vk::raii::PipelineLayout pipelineLayout           = nullptr;
    vk::raii::Pipeline graphicsPipeline               = nullptr;
    //
    vk::raii::CommandPool commandPool         = nullptr;
    vk::raii::Buffer vertexBuffer             = nullptr;
    vk::raii::DeviceMemory vertexBufferMemory = nullptr;
    vk::raii::Buffer indexBuffer              = nullptr;
    vk::raii::DeviceMemory indexBufferMemory  = nullptr;
    // uniform buffer
    std::vector<vk::raii::Buffer> uniformBuffers;
    std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    // descriptor pool
    vk::raii::DescriptorPool descriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet> descriptorSets;
    // light buffer
    std::vector<Light> lights;
    BufferResource lightBufferResource;
    // vk::raii::Buffer lightBuffer             = nullptr;
    // vk::raii::DeviceMemory lightBufferMemory = nullptr;
    //
    std::vector<vk::raii::CommandBuffer> commandBuffers;
    //
    std::vector<vk::raii::Semaphore> presentCompleteSemaphore;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphore;
    // 2 fences for GPU and CPU can work on their own task at the same time
    std::vector<vk::raii::Fence> inFlightFences;
    // texture
    Texture viking_room;
    // depth buffering
    vk::raii::Image depthImage              = nullptr;
    vk::raii::DeviceMemory depthImageMemory = nullptr;
    vk::raii::ImageView depthImageView      = nullptr;
    // G-Buffer Normal
    vk::raii::Image gBufferNormalImage              = nullptr;
    vk::raii::DeviceMemory gBufferNormalImageMemory = nullptr;
    vk::raii::ImageView gBufferNormalImageView      = nullptr;
    // G-Buffer Position
    vk::raii::Image gBufferPositionImage              = nullptr;
    vk::raii::DeviceMemory gBufferPositionImageMemory = nullptr;
    vk::raii::ImageView gBufferPositionImageView      = nullptr;
    // G-Buffer alebedo
    vk::raii::Image gBufferAlbedoImage              = nullptr;
    vk::raii::DeviceMemory gBufferAlbedoImageMemory = nullptr;
    vk::raii::ImageView gBufferAlbedoImageView      = nullptr;

    // class member for model
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    //
    bool framebufferResized = false;
    uint32_t currentFrame   = 0;
    uint32_t semaphoreIndex = 0;
    // compute pipeline
    vk::raii::Pipeline computePipeline                       = nullptr;
    vk::raii::DescriptorSetLayout computeDescriptorSetLayout = nullptr;
    vk::raii::PipelineLayout computePipelineLayout           = nullptr;
    vk::raii::DescriptorSet computeDescriptorSet             = nullptr;
    std::vector<vk::raii::DescriptorSet> computeDescriptorSets;
    // storage_Image processed by compute shader
    vk::raii::Image storageImage              = nullptr;
    vk::raii::DeviceMemory storageImageMemory = nullptr;
    vk::raii::ImageView storageImageView      = nullptr;
    // maintain the time and matrix for animation
    glm::mat4 currentModelMatrix;

    // Camera and Input State
    Camera camera;
    bool wPressed = false, sPressed = false, aPressed = false, dPressed = false;
    bool leftMouseButtonPressed  = false;
    bool rightMouseButtonPressed = false;
    float lastX                  = WIDTH / 2.0f;
    float lastY                  = HEIGHT / 2.0f;
    //
    std::vector<const char*> requiredDeviceExtension = {vk::KHRSwapchainExtensionName,
                                                        vk::KHRSpirv14ExtensionName,
                                                        vk::KHRSynchronization2ExtensionName,
                                                        vk::KHRCreateRenderpass2ExtensionName,
                                                        vk::KHRAccelerationStructureExtensionName,
                                                        vk::KHRRayQueryExtensionName,
                                                        vk::KHRDeferredHostOperationsExtensionName,
                                                        vk::KHRBufferDeviceAddressExtensionName};

    void initWindow() {
        if (!SDL_Init(SDL_INIT_VIDEO)) throw SDLException("Failed to initialize SDL");
        if (!SDL_Vulkan_LoadLibrary(nullptr)) throw SDLException("Failed to load Vulkan library");
        window.reset(SDL_CreateWindow("ReSTIR_Vulkan", 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN));
        if (!window) throw SDLException("Failed to create window");
        // vk::raii::Context default constructor loads vulkan library automatically
        context = vk::raii::Context();
        auto const vulkanVersion{context.enumerateInstanceVersion()};
        std::cout << "Vulkan " << VK_API_VERSION_MAJOR(vulkanVersion) << "." << VK_API_VERSION_MINOR(vulkanVersion) << std::endl;
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        testValidationLayers();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        //
        createDescriptorSetLayout();
        createComputeDescriptorSetLayout();
        //
        createGraphicsPipeline();
        createComputePipeline();
        createCommandPool();
        //
        createDepthResources();
        createGbufferResources();
        createStorageImage();
        //
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        //
        loadModel();
        //
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createLightBuffer();
        createAccelerationStructures();
        //
        createDescriptorPool();
        createDescriptorSets();
        createComputeDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void mainLoop() {
        SDL_ShowWindow(window.get());
        while (running) {
            HandleEvents();

            // Continuous movement
            if (wPressed) camera.moveForward();
            if (sPressed) camera.moveBackward();
            if (aPressed) camera.moveLeft();
            if (dPressed) camera.moveRight();

            updateUniformBuffer(currentFrame);
            drawFrame();
        }
        device.waitIdle();  // wait for device to finish operations before destroying resources
    }
    void HandleEvents() {
        for (SDL_Event event; SDL_PollEvent(&event);) switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                    recreateSwapChain();
                    break;

                // Keyboard: Set flags
                case SDL_EVENT_KEY_DOWN:
                    switch (event.key.key) {
                        case SDLK_W:
                            wPressed = true;
                            break;
                        case SDLK_S:
                            sPressed = true;
                            break;
                        case SDLK_A:
                            aPressed = true;
                            break;
                        case SDLK_D:
                            dPressed = true;
                            break;
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    switch (event.key.key) {
                        case SDLK_W:
                            wPressed = false;
                            break;
                        case SDLK_S:
                            sPressed = false;
                            break;
                        case SDLK_A:
                            aPressed = false;
                            break;
                        case SDLK_D:
                            dPressed = false;
                            break;
                    }
                    break;

                // Mouse: Rotation with tracking
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        leftMouseButtonPressed = true;
                        lastX                  = event.button.x;
                        lastY                  = event.button.y;
                    }
                    if (event.button.button == SDL_BUTTON_RIGHT) {
                        rightMouseButtonPressed = true;
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        leftMouseButtonPressed = false;
                    }
                    if (event.button.button == SDL_BUTTON_RIGHT) {
                        rightMouseButtonPressed = false;
                    }
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    if (leftMouseButtonPressed) {
                        float xoffset = lastX - event.motion.x;  // Inverted
                        float yoffset = event.motion.y - lastY;
                        lastX         = event.motion.x;
                        lastY         = event.motion.y;
                        camera.rotate(xoffset, yoffset);
                    }
                    break;

                default:
                    break;
            }
    }
    void cleanup() {
        cleanupSwapChain();

        SDL_Quit();
    }
    void createInstance();
    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo);
        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
            .messageSeverity = severityFlags, .messageType = messageTypeFlags, .pfnUserCallback = &debugCallback};
        debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);

        std::cout << "[DEBUG] Validation layers are active. Callback is registered and ready!" << std::endl;

        // Send a test message through the debug messenger to confirm it's working
        vk::DebugUtilsMessengerCallbackDataEXT testCallbackData{};
        testCallbackData.pMessage =
            "Validation layer callback test - if you see this, the debug messenger is working "
            "correctly!";
        instance.submitDebugUtilsMessageEXT(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo, vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral, testCallbackData);
    }

    void createSurface();

    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();

    void createImageViews();
    void createDescriptorSetLayout();

    void createGraphicsPipeline();
    void createCommandPool();

    void createTextureImage();

    void loadModel();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();

    void createCommandBuffers();
    void createSyncObjects();
    void recordCommandBuffer(uint32_t imageIndex);
    void draw_transition_image_layout(vk::Image image,
                                      vk::ImageLayout oldLayout,
                                      vk::ImageLayout newLayout,
                                      vk::AccessFlags2 srcAccessMask,
                                      vk::AccessFlags2 dstAccessMask,
                                      vk::PipelineStageFlags2 srcStageMask,
                                      vk::PipelineStageFlags2 dstStageMask,
                                      vk::ImageAspectFlags image_aspectMask);
    // Waiting for the previous frame
    void drawFrame();
    // recreate swap chain
    void recreateSwapChain();
    void cleanupSwapChain();
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void createBuffer(vk::DeviceSize size,
                      vk::BufferUsageFlags usage,
                      vk::MemoryPropertyFlags properties,
                      vk::raii::Buffer& buffer,
                      vk::raii::DeviceMemory& bufferMemory);
    void updateUniformBuffer(uint32_t currentImage);
    void createDescriptorPool();
    void createDescriptorSets();
    void transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void copyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height);
    void createTextureImageView();
    void createTextureSampler();
    void createDepthResources();
    vk::Format findDepthFormat();
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
    static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities) {
        auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
        if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount)) {
            minImageCount = surfaceCapabilities.maxImageCount;
        }
        return minImageCount;
    }
    /**
     * @brief find a supported format from candidates
     *
     * @return vk::Format
     */
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
        for (const auto format : candidates) {
            vk::FormatProperties props = physicalDevice.getFormatProperties(format);

            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("failed to find supported format!");
    }
    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
        assert(!availableFormats.empty());
        const auto formatIt = std::ranges::find_if(availableFormats, [](const auto& format) {
            return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
        return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
    }

    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
        assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
        return std::ranges::any_of(availablePresentModes, [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; })
                   ? vk::PresentModeKHR::eMailbox
                   : vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

    std::vector<const char*> getRequiredExtensions();

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                          vk::DebugUtilsMessageTypeFlagsEXT type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                          void*) {
        // Print all validation messages (errors, warnings, verbose info)
        std::string severityStr;
        switch (severity) {
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
                severityStr = "[ERROR]";
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
                severityStr = "[WARNING]";
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
                severityStr = "[INFO]";
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
                severityStr = "[VERBOSE]";
                break;
            default:
                severityStr = "[UNKNOWN]";
        }

        std::cerr << severityStr << " validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

        return vk::False;
    }
    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }
        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        file.close();
        return buffer;
    }

    std::unique_ptr<vk::raii::CommandBuffer> beginSingleTimeCommands() {
        // helper funtion for transition
        vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};
        std::unique_ptr<vk::raii::CommandBuffer> commandBuffer =
            std::make_unique<vk::raii::CommandBuffer>(std::move(device.allocateCommandBuffers(allocInfo).front()));

        vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
        commandBuffer->begin(beginInfo);
        return commandBuffer;
    }
    void endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer) {
        commandBuffer.end();

        vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
        queue.submit(submitInfo, nullptr);
        queue.waitIdle();
    }
    void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size) {
        /*
        copy data from srcBuffer to dstBuffer, the whole process is done by command buffer,just like
        drawing commands

        */

        vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};

        vk::raii::CommandBuffer commandCopyBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());
        // start recording command buffer
        commandCopyBuffer.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy{.size = size});
        commandCopyBuffer.end();
        // submit the command buffer and wait until it finishes
        queue.submit(vk::SubmitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandCopyBuffer}, nullptr);
        queue.waitIdle();
    }
    /**
     * @brief create an image object and allocate memory for it
     *
     * @param width
     * @param height
     * @param format
     * @param tiling
     * @param usage
     * @param properties
     * @param image
     * @param imageMemory
     */
    void createImage(uint32_t width,
                     uint32_t height,
                     vk::Format format,
                     vk::ImageTiling tiling,
                     vk::ImageUsageFlags usage,
                     vk::MemoryPropertyFlags properties,
                     vk::raii::Image& image,
                     vk::raii::DeviceMemory& imageMemory) {
        vk::ImageCreateInfo imageInfo{.imageType   = vk::ImageType::e2D,
                                      .format      = format,
                                      .extent      = {width, height, 1},
                                      .mipLevels   = 1,
                                      .arrayLayers = 1,
                                      .samples     = vk::SampleCountFlagBits::e1,
                                      .tiling      = tiling,
                                      .usage       = usage,
                                      .sharingMode = vk::SharingMode::eExclusive};

        image = vk::raii::Image(device, imageInfo);

        vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{.allocationSize  = memRequirements.size,
                                         .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)};
        imageMemory = vk::raii::DeviceMemory(device, allocInfo);
        image.bindMemory(*imageMemory, 0);
    }
    /**
     * @brief create an image view for the given image
     *
     * @param image
     * @param format
     * @param aspectFlags
     * @return vk::raii::ImageView
     */
    vk::raii::ImageView createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags) {
        vk::ImageViewCreateInfo viewInfo{
            .image = image, .viewType = vk::ImageViewType::e2D, .format = format, .subresourceRange = {aspectFlags, 0, 1, 0, 1}};
        return vk::raii::ImageView(device, viewInfo);
    }
    void createGbufferResources();
    void testValidationLayers() {
        std::cout << "=== VALIDATION LAYER TEST ===" << std::endl;
        std::cout << "enableValidationLayers = " << (enableValidationLayers ? "TRUE" : "FALSE") << std::endl;

        if (enableValidationLayers) {
            std::cout << "Debug messenger created: " << (*debugMessenger ? "YES" : "NO") << std::endl;
            std::cout << "Requested layers:" << std::endl;
            for (const auto& layer : validationLayers) {
                std::cout << "  - " << layer << std::endl;
            }
        } else {
            std::cout << "Validation layers are DISABLED" << std::endl;
        }
        std::cout << "=============================" << std::endl;
    }
    //
    void createLightBuffer();
    // compute shader related functions
    void createStorageImage();
    void createComputeDescriptorSetLayout();
    void createComputePipeline();
    void createComputeDescriptorSets();
    void transitionImageLayout(vk::Image image,
                               vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout,
                               vk::AccessFlags2 srcAccessMask,
                               vk::AccessFlags2 dstAccessMask,
                               vk::PipelineStageFlags2 srcStageMask,
                               vk::PipelineStageFlags2 dstStageMask,
                               vk::ImageAspectFlags image_aspectMask);

    /*Ray tracing*/
    void createAccelerationStructures();

    std::vector<SubMesh> submeshes;
    std::vector<tinyobj::material_t> materials;

    std::vector<vk::raii::AccelerationStructureKHR> blasHandles;
    std::vector<vk::raii::Buffer> blasBuffers;
    std::vector<vk::raii::DeviceMemory> blasMemories;

    std::vector<vk::raii::AccelerationStructureKHR> tlas;
    std::vector<vk::raii::Buffer> tlasBuffer;
    std::vector<vk::raii::DeviceMemory> tlasMemory;
    std::vector<vk::raii::Buffer> tlasScratchBuffer;
    std::vector<vk::raii::DeviceMemory> tlasScratchMemory;

    void updateTLAS(const vk::raii::CommandBuffer& commandBuffer);

    std::vector<vk::raii::Buffer> instanceBuffer;
    std::vector<vk::raii::DeviceMemory> instanceMemory;

    vk::DeviceAddress getVertAddress(const vk::raii::Buffer& buffer) {
        vk::BufferDeviceAddressInfo vertex_addr_info{.buffer = *buffer};
        return device.getBufferAddressKHR(vertex_addr_info);
    }
    vk::DeviceAddress getIndexAddr(const vk::raii::Buffer& buffer) {
        vk::BufferDeviceAddressInfo index_addr_info{.buffer = *buffer};
        return device.getBufferAddressKHR(index_addr_info);
    }
    void addQuad(std::vector<Vertex>& vertices,
                 std::vector<uint32_t>& indices,
                 glm::vec3 p1,
                 glm::vec3 p2,
                 glm::vec3 p3,
                 glm::vec3 p4,
                 glm::vec3 color,
                 uint32_t& indexOffset) {
        // p1-p2
        // |  |
        // p4-p3
        glm::vec3 normal = glm::normalize(glm::cross(p2 - p1, p4 - p1));

        vertices.push_back({p1, color, {0.0f, 0.0f}, normal});  // TL
        vertices.push_back({p2, color, {1.0f, 0.0f}, normal});  // TR
        vertices.push_back({p3, color, {1.0f, 1.0f}, normal});  // BR
        vertices.push_back({p4, color, {0.0f, 1.0f}, normal});  // BL

        indices.push_back(indexOffset + 0);
        indices.push_back(indexOffset + 1);
        indices.push_back(indexOffset + 2);
        indices.push_back(indexOffset + 2);
        indices.push_back(indexOffset + 3);
        indices.push_back(indexOffset + 0);
        indexOffset += 4;
    }
};
