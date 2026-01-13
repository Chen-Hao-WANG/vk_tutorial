#include "tutorial.hpp"

void HelloTriangleApplication::createGbufferResources() {
    // Clear existing referencing vectors
    gBufferPositionImage.clear();
    gBufferPositionImageMemory.clear();
    gBufferPositionImageView.clear();
    //
    gBufferNormalImage.clear();
    gBufferNormalImageMemory.clear();
    gBufferNormalImageView.clear();
    //
    gBufferAlbedoImage.clear();
    gBufferAlbedoImageMemory.clear();
    gBufferAlbedoImageView.clear();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Position
        vk::raii::Image posImage              = nullptr;
        vk::raii::DeviceMemory posImageMemory = nullptr;
        createImage(swapChainExtent.width, std::max(swapChainExtent.height, 1u), vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
                        vk::ImageUsageFlagBits::eStorage,
                    vk::MemoryPropertyFlagBits::eDeviceLocal, posImage, posImageMemory);
        gBufferPositionImageView.push_back(createImageView(posImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor));
        gBufferPositionImage.push_back(std::move(posImage));
        gBufferPositionImageMemory.push_back(std::move(posImageMemory));

        // Normal
        vk::raii::Image normImage              = nullptr;
        vk::raii::DeviceMemory normImageMemory = nullptr;
        createImage(swapChainExtent.width, std::max(swapChainExtent.height, 1u), vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
                        vk::ImageUsageFlagBits::eStorage,
                    vk::MemoryPropertyFlagBits::eDeviceLocal, normImage, normImageMemory);
        gBufferNormalImageView.push_back(createImageView(normImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor));
        gBufferNormalImage.push_back(std::move(normImage));
        gBufferNormalImageMemory.push_back(std::move(normImageMemory));

        // Albedo
        vk::raii::Image albedoImage              = nullptr;
        vk::raii::DeviceMemory albedoImageMemory = nullptr;
        createImage(swapChainExtent.width, std::max(swapChainExtent.height, 1u), vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
                        vk::ImageUsageFlagBits::eStorage,
                    vk::MemoryPropertyFlagBits::eDeviceLocal, albedoImage, albedoImageMemory);
        gBufferAlbedoImageView.push_back(createImageView(albedoImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor));
        gBufferAlbedoImage.push_back(std::move(albedoImage));
        gBufferAlbedoImageMemory.push_back(std::move(albedoImageMemory));
    }
}
/**
 * @brief create storage image for compute shader to write to.
 *
 *  vk::ImageUsageFlagBits usage for compute shader to write and pass it to swapchain for
 * presentation
 */
void HelloTriangleApplication::createStorageImage() {
    storageImage.clear();
    storageImageMemory.clear();
    storageImageView.clear();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::raii::Image img              = nullptr;
        vk::raii::DeviceMemory imgMemory = nullptr;

        createImage(swapChainExtent.width, std::max(swapChainExtent.height, 1u), vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled,
                    vk::MemoryPropertyFlagBits::eDeviceLocal, img, imgMemory);

        storageImageView.push_back(createImageView(img, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor));
        
        // Initial transition
        transitionImageLayout(*img, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, vk::AccessFlagBits2::eNone,
                              vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eTopOfPipe,
                              vk::PipelineStageFlagBits2::eComputeShader, vk::ImageAspectFlagBits::eColor);
        
        storageImage.push_back(std::move(img));
        storageImageMemory.push_back(std::move(imgMemory));
    }
    std::cout << "[Info] Storage Images created (Ready for Compute Shader output)" << std::endl;
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
