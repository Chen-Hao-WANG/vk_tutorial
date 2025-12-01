#include "tutorial.hpp"

void HelloTriangleApplication::createDepthResources()
{
    vk::Format depthFormat = findDepthFormat();
    //
    createImage(swapChainExtent.width,
                swapChainExtent.height,
                depthFormat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                depthImage,
                depthImageMemory);
    //
    depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}
bool hasStencilComponent(vk::Format format)
{
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

// use VK_FORMAT_FEATURE_ instead of VK_IMAGE_USAGE_
vk::Format HelloTriangleApplication::findDepthFormat()
{
    // helper function to select a format with a depth component that supports usage as depth attachment
    return findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}