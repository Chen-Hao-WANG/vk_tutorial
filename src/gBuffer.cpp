#include "tutorial.hpp"

void HelloTriangleApplication::createGbufferResources() {
    // position
    createImage(swapChainExtent.width, swapChainExtent.height, vk::Format::eR32G32B32A32Sfloat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
                    vk::ImageUsageFlagBits::eStorage,
                vk::MemoryPropertyFlagBits::eDeviceLocal, gBufferPositionImage,
                gBufferPositionImageMemory);
    gBufferPositionImageView = createImageView(
        gBufferPositionImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);
    // normal
    createImage(swapChainExtent.width, swapChainExtent.height, vk::Format::eR32G32B32A32Sfloat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
                    vk::ImageUsageFlagBits::eStorage,
                vk::MemoryPropertyFlagBits::eDeviceLocal, gBufferNormalImage,
                gBufferNormalImageMemory);

    gBufferNormalImageView = createImageView(gBufferNormalImage, vk::Format::eR32G32B32A32Sfloat,
                                             vk::ImageAspectFlagBits::eColor);
}
/**
 * @brief create storage image for compute shader to write to.
 *
 *  vk::ImageUsageFlagBits usage for compute shader to write and pass it to swapchain for
 * presentation
 */
void HelloTriangleApplication::createStorageImage() {
    createImage(swapChainExtent.width, swapChainExtent.height, vk::Format::eR32G32B32A32Sfloat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc |
                    vk::ImageUsageFlagBits::eSampled,
                vk::MemoryPropertyFlagBits::eDeviceLocal,  // GPU local memory for best performance
                storageImage, storageImageMemory);

    storageImageView = createImageView(storageImage, vk::Format::eR32G32B32A32Sfloat,
                                       vk::ImageAspectFlagBits::eColor);

    //
    transitionImageLayout(*storageImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                          vk::AccessFlagBits2::eNone,                  // srcAccess
                          vk::AccessFlagBits2::eShaderWrite,           // dstAccess
                          vk::PipelineStageFlagBits2::eTopOfPipe,      // srcStage
                          vk::PipelineStageFlagBits2::eComputeShader,  // dstStage
                          vk::ImageAspectFlagBits::eColor);
    std::cout << "[Info] Storage Image created (Ready for Compute Shader output)" << std::endl;
}

void HelloTriangleApplication::transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout,
                                                     vk::ImageLayout newLayout,
                                                     vk::AccessFlags2 srcAccessMask,
                                                     vk::AccessFlags2 dstAccessMask,
                                                     vk::PipelineStageFlags2 srcStageMask,
                                                     vk::PipelineStageFlags2 dstStageMask,
                                                     vk::ImageAspectFlags image_aspectMask) {
    auto commandBuffer = beginSingleTimeCommands();

    vk::ImageMemoryBarrier2 barrier = {.srcStageMask = srcStageMask,
                                       .srcAccessMask = srcAccessMask,
                                       .dstStageMask = dstStageMask,
                                       .dstAccessMask = dstAccessMask,
                                       .oldLayout = oldLayout,
                                       .newLayout = newLayout,
                                       .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                       .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                       .image = image,
                                       .subresourceRange = {.aspectMask = image_aspectMask,
                                                            .baseMipLevel = 0,
                                                            .levelCount = 1,
                                                            .baseArrayLayer = 0,
                                                            .layerCount = 1}};

    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier};

    commandBuffer->pipelineBarrier2(dependencyInfo);

    endSingleTimeCommands(*commandBuffer);
}
