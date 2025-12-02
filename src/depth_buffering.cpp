#include "tutorial.hpp"
/**
 * @brief create depth buffer resource for depth testing
 *
 */
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
/**
 * @brief check if a format has a stencil component
 *
 * @param  vk::Format format
 * @return true
 * @return false
 */
bool hasStencilComponent(vk::Format format)
{
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

/**
 * @brief find a supported format from candidates
 *  use VK_FORMAT_FEATURE_ instead of VK_IMAGE_USAGE_ since we are checking format features here
 * @return vk::Format
 */
vk::Format HelloTriangleApplication::findDepthFormat()
{
    // helper function to select a format with a depth component that supports usage as depth attachment
    return findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}