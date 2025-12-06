#include "tutorial.hpp"

void HelloTriangleApplication::createGbufferResources()
{
    
    //
    createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        vk::Format::eR32G32B32A32Sfloat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        gBufferPositionImage,
        gBufferPositionImageMemory);
    gBufferPositionImageView = createImageView(gBufferPositionImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);

    createImage(swapChainExtent.width, swapChainExtent.height, vk::Format::eR16G16B16A16Sfloat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                gBufferNormalImage, gBufferNormalImageMemory);

    gBufferNormalImageView = createImageView(gBufferNormalImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
}