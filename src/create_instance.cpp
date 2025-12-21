#include "tutorial.hpp"

void HelloTriangleApplication::createInstance() {
    constexpr vk::ApplicationInfo appInfo{.pApplicationName = "Hello Triangle",
                                          .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                          .pEngineName = "No Engine",
                                          .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                                          .apiVersion = vk::ApiVersion14};

    // Get the required layers
    std::vector<char const*> requiredLayers;
    if (enableValidationLayers) {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // Check if the required layers are supported by the Vulkan implementation.
    auto layerProperties = context.enumerateInstanceLayerProperties();
    for (auto const& requiredLayer : requiredLayers) {
        if (std::ranges::none_of(layerProperties, [requiredLayer](auto const& layerProperty) {
                return strcmp(layerProperty.layerName, requiredLayer) == 0;
            })) {
            throw std::runtime_error("Required layer not supported: " + std::string(requiredLayer));
        }
    }

    // Get the required extensions.
    auto requiredExtensions = getRequiredExtensions();

    // Check if the required extensions are supported by the Vulkan implementation.
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (auto const& requiredExtension : requiredExtensions) {
        if (std::ranges::none_of(
                extensionProperties, [requiredExtension](auto const& extensionProperty) {
                    return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
                })) {
            throw std::runtime_error("Required extension not supported: " +
                                     std::string(requiredExtension));
        }
    }

    auto hasExt = [&](char const* name) -> bool {
        return std::ranges::any_of(requiredExtensions,
                                   [&](char const* e) { return strcmp(e, name) == 0; });
    };

    constexpr char const* kExtLayerSettings = VK_EXT_LAYER_SETTINGS_EXTENSION_NAME;
    constexpr char const* kExtValidationFeatures = VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME;
    constexpr char const* kKhronosValidationLayer = "VK_LAYER_KHRONOS_validation";

    // Optional: advanced validation features (pNext chain)
    void const* pNext = nullptr;

    // --- Modern path: VK_EXT_layer_settings (preferred) ---
    vk::LayerSettingsCreateInfoEXT layerSettingsCI{};
    std::array<vk::LayerSettingEXT, 3> layerSettings{};
    vk::Bool32 enableBool = VK_TRUE;

    // --- Legacy fallback: VK_EXT_validation_features (deprecated) ---
    std::vector<vk::ValidationFeatureEnableEXT> validationFeatureEnables;
    vk::ValidationFeaturesEXT validationFeatures{};

    if (enableValidationLayers && hasExt(kExtLayerSettings)) {
        // These setting names are interpreted by VK_LAYER_KHRONOS_validation
        layerSettings = {
            vk::LayerSettingEXT{
                .pLayerName = kKhronosValidationLayer,
                .pSettingName = "validate_gpu_assisted",
                .type = vk::LayerSettingTypeEXT::eBool32,
                .valueCount = 1,
                .pValues = &enableBool,
            },
            vk::LayerSettingEXT{
                .pLayerName = kKhronosValidationLayer,
                .pSettingName = "validate_gpu_assisted_reserve_binding_slot",
                .type = vk::LayerSettingTypeEXT::eBool32,
                .valueCount = 1,
                .pValues = &enableBool,
            },
            vk::LayerSettingEXT{
                .pLayerName = kKhronosValidationLayer,
                .pSettingName = "validate_sync",
                .type = vk::LayerSettingTypeEXT::eBool32,
                .valueCount = 1,
                .pValues = &enableBool,
            },
        };

        layerSettingsCI = vk::LayerSettingsCreateInfoEXT{
            .settingCount = static_cast<uint32_t>(layerSettings.size()),
            .pSettings = layerSettings.data(),
        };

        pNext = &layerSettingsCI;
    } else if (enableValidationLayers && hasExt(kExtValidationFeatures)) {
        // Fallback for older runtimes (deprecated extension)
        validationFeatureEnables = {
            vk::ValidationFeatureEnableEXT::eGpuAssisted,
            vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot,
            vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
        };

        validationFeatures = vk::ValidationFeaturesEXT{
            .enabledValidationFeatureCount = static_cast<uint32_t>(validationFeatureEnables.size()),
            .pEnabledValidationFeatures = validationFeatureEnables.data(),
        };

        pNext = &validationFeatures;
    }

    vk::InstanceCreateInfo createInfo{
        .pNext = pNext,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.empty() ? nullptr : requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.empty() ? nullptr : requiredExtensions.data(),
    };

    instance = vk::raii::Instance(context, createInfo);
}

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);

        constexpr char const* kExtLayerSettings = VK_EXT_LAYER_SETTINGS_EXTENSION_NAME;
        constexpr char const* kExtValidationFeatures = VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME;

        auto available = context.enumerateInstanceExtensionProperties();

        auto supported = [&](char const* extName) -> bool {
            return std::ranges::any_of(
                available, [&](auto const& ep) { return strcmp(ep.extensionName, extName) == 0; });
        };

        // Prefer modern VK_EXT_layer_settings; fall back to deprecated validation_features if
        // needed.
        if (supported(kExtLayerSettings)) {
            extensions.push_back(kExtLayerSettings);
        } else if (supported(kExtValidationFeatures)) {
            extensions.push_back(kExtValidationFeatures);
        }
    }

    return extensions;
}