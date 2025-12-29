#include "tutorial.hpp"

void HelloTriangleApplication::createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) && physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) {
            // found a queue family that supports both graphics and present
            queueIndex = qfpIndex;
            break;
        }
    }
    if (queueIndex == ~0) {
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }

    // query for Vulkan 1.3 features
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features,
                       vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
                       vk::PhysicalDeviceAccelerationStructureFeaturesKHR, vk::PhysicalDeviceRayQueryFeaturesKHR>

        featureChain = {
            {.features = {.samplerAnisotropy = true}},  // vk::PhysicalDeviceFeatures2
            {.shaderDrawParameters = true},             // vk::PhysicalDeviceVulkan11Features
            {                                           // Vulkan 1.2
             .descriptorBindingSampledImageUpdateAfterBind = true,
             .descriptorBindingPartiallyBound = true,
             .runtimeDescriptorArray = true,
             .bufferDeviceAddress = true},
            {.extendedDynamicState = true},
            {.accelerationStructure = true},  //
            {.rayQuery = true},
            {.synchronization2 = true, .dynamicRendering = true},  // vk::PhysicalDeviceVulkan13Features
            {.extendedDynamicState = true}                         // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        };

    // create a Device
    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{.queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};
    vk::DeviceCreateInfo deviceCreateInfo{.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                                          .queueCreateInfoCount = 1,
                                          .pQueueCreateInfos = &deviceQueueCreateInfo,
                                          .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
                                          .ppEnabledExtensionNames = requiredDeviceExtension.data()};

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    queue = vk::raii::Queue(device, queueIndex, 0);
}
