#include "tutorial.hpp"

void HelloTriangleApplication::createGbufferResources()
{

    // position
    createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        vk::Format::eR32G32B32A32Sfloat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        gBufferPositionImage,
        gBufferPositionImageMemory);
    gBufferPositionImageView = createImageView(gBufferPositionImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);
    // normal
    createImage(swapChainExtent.width, swapChainExtent.height, vk::Format::eR32G32B32A32Sfloat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                gBufferNormalImage, gBufferNormalImageMemory);

    gBufferNormalImageView = createImageView(gBufferNormalImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);
}
/**
 * @brief create storage image for compute shader to write to.
 *
 *  vk::ImageUsageFlagBits usage for compute shader to write and pass it to swapchain for presentation
 */
void HelloTriangleApplication::createStorageImage()
{

    createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        vk::Format::eR32G32B32A32Sfloat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal, // GPU local memory for best performance
        storageImage,
        storageImageMemory);

    storageImageView = createImageView(storageImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);

    //
    auto cmdBuffer = beginSingleTimeCommands();
    transitionImageLayout(
        storageImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral
    );
    endSingleTimeCommands(*cmdBuffer);
    std::cout << "[Info] Storage Image created (Ready for Compute Shader output)" << std::endl;
}
