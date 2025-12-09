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