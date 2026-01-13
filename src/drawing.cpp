#include "tutorial.hpp"

/**
 * @brief Initializes the Command Pool object.
 *
 * This pool manages the memory for command buffers. It is configured to target
 * the graphics queue family and allows for individual command buffer resetting.
 *
 * @note This must be called before creating any command buffers.
 * @throws vk::SystemError if the command pool creation fails.
 */
void HelloTriangleApplication::createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo{// Internal Implementation Note:
                                       // We use 'eResetCommandBuffer' to allow command buffers to be re-recorded individually.
                                       // Without this flag, we would have to reset the entire pool to re-record a single buffer.
                                       .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,

                                       // Specify that the command buffers allocated from this pool
                                       // will be submitted to the graphics queue.
                                       .queueFamilyIndex = queueIndex};

    commandPool = vk::raii::CommandPool(device, poolInfo);
}
void HelloTriangleApplication::createCommandBuffers() {
    /*
    1. allocate command buffers from the command pool
    2. decide level and number of command buffers
    3. depend on how many things we want to do in parallel
    4. by now we use MAX_FRAMES_IN_FLIGHT to decide the number of command buffers, because we want
    to record command buffers for each frame in flight
    */
    commandBuffers.clear();
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
    commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void HelloTriangleApplication::recordCommandBuffer(uint32_t imageIndex) {
    auto& cmd = commandBuffers[currentFrame];
    cmd.begin({});
    updateTLAS(cmd);
    // --- PHASE 1: Prepare Image Layouts for Rasterization ---
    // Transition Swapchain image to Color Attachment layout
    draw_transition_image_layout(*gBufferAlbedoImage[currentFrame],
                                 vk::ImageLayout::eUndefined,
                                 vk::ImageLayout::eColorAttachmentOptimal,
                                 {},
                                 vk::AccessFlagBits2::eColorAttachmentWrite,
                                 vk::PipelineStageFlagBits2::eTopOfPipe,
                                 vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                 vk::ImageAspectFlagBits::eColor);

    // Transition Depth image to Depth Attachment layout
    draw_transition_image_layout(*depthImage[currentFrame],
                                 vk::ImageLayout::eUndefined,
                                 vk::ImageLayout::eDepthAttachmentOptimal,
                                 vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                                 vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                                 vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                                 vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                                 vk::ImageAspectFlagBits::eDepth);
    // Transition Position G-Buffer
    draw_transition_image_layout(*gBufferPositionImage[currentFrame],
                                 vk::ImageLayout::eUndefined,
                                 vk::ImageLayout::eColorAttachmentOptimal,
                                 {},
                                 vk::AccessFlagBits2::eColorAttachmentWrite,
                                 vk::PipelineStageFlagBits2::eTopOfPipe,
                                 vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                 vk::ImageAspectFlagBits::eColor);
    // Transition Normal G-Buffer
    draw_transition_image_layout(*gBufferNormalImage[currentFrame],
                                 vk::ImageLayout::eUndefined,
                                 vk::ImageLayout::eColorAttachmentOptimal,
                                 {},
                                 vk::AccessFlagBits2::eColorAttachmentWrite,
                                 vk::PipelineStageFlagBits2::eTopOfPipe,
                                 vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                 vk::ImageAspectFlagBits::eColor);

    // --- PHASE 2: Rasterization Pass (Fill G-Buffers) ---
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    // Setup G-Buffer attachments (Alebedo, World Position, Normal)
    vk::RenderingAttachmentInfo colorAttachmentInfo[] = {{.imageView   = *gBufferAlbedoImageView[currentFrame],
                                                          .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                                          .loadOp      = vk::AttachmentLoadOp::eClear,
                                                          .storeOp     = vk::AttachmentStoreOp::eStore,
                                                          .clearValue  = clearColor},
                                                         {.imageView   = *gBufferPositionImageView[currentFrame],
                                                          .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                                          .loadOp      = vk::AttachmentLoadOp::eClear,
                                                          .storeOp     = vk::AttachmentStoreOp::eStore,
                                                          .clearValue  = clearColor},
                                                         {.imageView   = *gBufferNormalImageView[currentFrame],
                                                          .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                                          .loadOp      = vk::AttachmentLoadOp::eClear,
                                                          .storeOp     = vk::AttachmentStoreOp::eStore,
                                                          .clearValue  = clearColor}};

    vk::RenderingAttachmentInfo depthAttachmentInfo = {.imageView   = *depthImageView[currentFrame],
                                                       .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
                                                       .loadOp      = vk::AttachmentLoadOp::eClear,
                                                       .storeOp     = vk::AttachmentStoreOp::eDontCare,
                                                       .clearValue  = clearDepth};

    vk::RenderingInfo renderingInfo = {.renderArea           = {.offset = {0, 0}, .extent = swapChainExtent},
                                       .layerCount           = 1,
                                       .colorAttachmentCount = 3,
                                       .pColorAttachments    = colorAttachmentInfo,
                                       .pDepthAttachment     = &depthAttachmentInfo};
    //
    cmd.beginRendering(renderingInfo);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D({0, 0}, swapChainExtent));
    //
    cmd.bindVertexBuffers(0, *vertexBuffer, {0});
    cmd.bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint32);
    //
    // Bind Graphics Descriptor Set (Set 0: MVP matrices)
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *descriptorSets[currentFrame], nullptr);
    glm::mat4 spin = currentModelMatrix;
    // Rotate -90 degrees on X to make Y-Up Bunny stand in Z-Up World
    glm::mat4 standUp     = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 bunnyMatrix = spin * standUp;
    glm::mat4 wallMatrix  = glm::mat4(1.0f);

    for (size_t i = 0; i < submeshes.size(); i++) {
        MeshPushConstants constants;

        // The Cornell Box is the LAST submesh (added in load_Model.cpp)
        // Everything before it is part of the Bunny
        bool isCornellBox = (i == submeshes.size() - 1);

        if (!isCornellBox) {
            constants.modelMatrix = bunnyMatrix;  // All bunny parts spin
        } else {
            constants.modelMatrix = wallMatrix;  // Box stays static
        }

        cmd.pushConstants<MeshPushConstants>(*pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, constants);
        cmd.drawIndexed(submeshes[i].indexCount, 1, submeshes[i].indexOffset, 0, 0);
    }
    cmd.endRendering();
    // --- PHASE 3: Synchronize and Transition G-Buffers for Compute Read ---
    // Transition Normal G-Buffer
    draw_transition_image_layout(*gBufferNormalImage[currentFrame],
                                 vk::ImageLayout::eColorAttachmentOptimal,
                                 vk::ImageLayout::eShaderReadOnlyOptimal,
                                 vk::AccessFlagBits2::eColorAttachmentWrite,
                                 vk::AccessFlagBits2::eShaderRead,
                                 vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                 vk::PipelineStageFlagBits2::eComputeShader,
                                 vk::ImageAspectFlagBits::eColor);

    // Transition Position G-Buffer
    draw_transition_image_layout(*gBufferPositionImage[currentFrame],
                                 vk::ImageLayout::eColorAttachmentOptimal,
                                 vk::ImageLayout::eShaderReadOnlyOptimal,
                                 vk::AccessFlagBits2::eColorAttachmentWrite,
                                 vk::AccessFlagBits2::eShaderRead,
                                 vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                 vk::PipelineStageFlagBits2::eComputeShader,
                                 vk::ImageAspectFlagBits::eColor);
    draw_transition_image_layout(*gBufferAlbedoImage[currentFrame],
                                 vk::ImageLayout::eColorAttachmentOptimal,
                                 vk::ImageLayout::eShaderReadOnlyOptimal,
                                 vk::AccessFlagBits2::eColorAttachmentWrite,
                                 vk::AccessFlagBits2::eShaderRead,
                                 vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                 vk::PipelineStageFlagBits2::eComputeShader,
                                 vk::ImageAspectFlagBits::eColor);

    // --- PHASE 4: Compute Pass (Lighting / ReSTIR) ---
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline);

    // Bind Compute Descriptor Set (Set 0: G-Buffers, Lights, Output Image)
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipelineLayout, 0, *computeDescriptorSets[currentFrame], nullptr);

    // Calculate Workgroup counts based on window size (assuming 16x16 local groups in shader)
    uint32_t groupCountX = (swapChainExtent.width + 15) / 16;
    uint32_t groupCountY = (swapChainExtent.height + 15) / 16;
    cmd.dispatch(groupCountX, groupCountY, 1);

    // --- PHASE 5: Transfer Compute Result (storageImage) to Swapchain ---

    // --- PHASE 5: Transfer Compute Result (storageImage) to Swapchain ---

    // 1. Transition Storage Image to Transfer Source layout
    draw_transition_image_layout(*storageImage[currentFrame],
                                 vk::ImageLayout::eGeneral,
                                 vk::ImageLayout::eTransferSrcOptimal,
                                 vk::AccessFlagBits2::eShaderWrite,
                                 vk::AccessFlagBits2::eTransferRead,
                                 vk::PipelineStageFlagBits2::eComputeShader,
                                 vk::PipelineStageFlagBits2::eTransfer,
                                 vk::ImageAspectFlagBits::eColor);

    // 2. Transition Swapchain Image to Transfer Destination layout
    draw_transition_image_layout(swapChainImages[imageIndex],
                                 vk::ImageLayout::eUndefined,
                                 vk::ImageLayout::eTransferDstOptimal,
                                 {},  // srcAccessMask (None is fine here)
                                 vk::AccessFlagBits2::eTransferWrite,
                                 vk::PipelineStageFlagBits2::eColorAttachmentOutput,  // <--- CHANGED: Wait for the semaphore!
                                 vk::PipelineStageFlagBits2::eTransfer,
                                 vk::ImageAspectFlagBits::eColor);

    // 3. Blit (scaled copy) Compute result into the Swapchain Image
    vk::ImageBlit blitRegion{
        .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        // Fix: Explicitly construct std::array
        .srcOffsets =
            std::array<vk::Offset3D, 2>{vk::Offset3D(0, 0, 0), vk::Offset3D((int32_t)swapChainExtent.width, (int32_t)swapChainExtent.height, 1)},
        .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        // Fix: Explicitly construct std::array
        .dstOffsets =
            std::array<vk::Offset3D, 2>{vk::Offset3D(0, 0, 0), vk::Offset3D((int32_t)swapChainExtent.width, (int32_t)swapChainExtent.height, 1)}};

    cmd.blitImage(*storageImage[currentFrame],
                  vk::ImageLayout::eTransferSrcOptimal,
                  swapChainImages[imageIndex],
                  vk::ImageLayout::eTransferDstOptimal,
                  blitRegion,
                  vk::Filter::eLinear);
    // --- PHASE 6: Final Transitions for Presentation ---
    // Prepare Swapchain Image for the OS presentation engine
    draw_transition_image_layout(swapChainImages[imageIndex],
                                 vk::ImageLayout::eTransferDstOptimal,
                                 vk::ImageLayout::ePresentSrcKHR,
                                 vk::AccessFlagBits2::eTransferWrite,
                                 {},
                                 vk::PipelineStageFlagBits2::eTransfer,
                                 vk::PipelineStageFlagBits2::eBottomOfPipe,
                                 vk::ImageAspectFlagBits::eColor);

    // Transition Storage Image back to General layout for the next frame's compute write
    draw_transition_image_layout(*storageImage[currentFrame],
                                 vk::ImageLayout::eTransferSrcOptimal,
                                 vk::ImageLayout::eGeneral,
                                 vk::AccessFlagBits2::eTransferRead,
                                 vk::AccessFlagBits2::eShaderWrite,
                                 vk::PipelineStageFlagBits2::eTransfer,
                                 vk::PipelineStageFlagBits2::eComputeShader,
                                 vk::ImageAspectFlagBits::eColor);

    cmd.end();
}

/**
 * @brief Transition the image layout to a new layout
 *
 * @param image
 * @param oldLayout
 * @param newLayout
 * @param srcAccessMask
 * @param dstAccessMask
 * @param srcStageMask
 * @param dstStageMask
 * @param image_aspectMask
 */
void HelloTriangleApplication::draw_transition_image_layout(vk::Image image,
                                                            vk::ImageLayout oldLayout,
                                                            vk::ImageLayout newLayout,
                                                            vk::AccessFlags2 srcAccessMask,
                                                            vk::AccessFlags2 dstAccessMask,
                                                            vk::PipelineStageFlags2 srcStageMask,
                                                            vk::PipelineStageFlags2 dstStageMask,
                                                            vk::ImageAspectFlags image_aspectMask) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask        = srcStageMask,
        .srcAccessMask       = srcAccessMask,
        .dstStageMask        = dstStageMask,
        .dstAccessMask       = dstAccessMask,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image,
        .subresourceRange    = {.aspectMask = image_aspectMask, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

    vk::DependencyInfo dependencyInfo = {.dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier};

    commandBuffers[currentFrame].pipelineBarrier2(dependencyInfo);
}
void HelloTriangleApplication::createSyncObjects() {
    /*
    1. decide fence(CPU, the host needs to know when the GPU has finished something) or semaphore to
    use
    2. what operations to synchronize
    3. how many synchronization objects are needed
    4. create synchronization objects
    */
    /*
    1. a semaphore to signal when an image has been acquired and is ready for rendering
    2. a semaphore to signal when rendering is finished and the image is ready for presentation
    3. a fence to ensure that the CPU waits for the GPU to finish rendering before starting the next
    frame
    */
    // clean up old synchronization objects if they exist
    presentCompleteSemaphore.clear();
    renderFinishedSemaphore.clear();
    inFlightFences.clear();
    // create new synchronization objects
    presentCompleteSemaphore.reserve(swapChainImages.size());
    renderFinishedSemaphore.reserve(swapChainImages.size());
    inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        presentCompleteSemaphore.emplace_back(device, vk::SemaphoreCreateInfo());
        renderFinishedSemaphore.emplace_back(device, vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        inFlightFences.emplace_back(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}
void HelloTriangleApplication::drawFrame() {
    /*
    1. require fence to ensure that the CPU waits for the GPU to finish rendering before starting
    the next frame
    2. acquire an image from the swap chain
    3. update the uniform buffer
    4. record command buffer
    5. submit the command buffer
    6. present the image
    7. advance to the next frame
    */
    while (vk::Result::eTimeout == device.waitForFences(*inFlightFences[currentFrame], vk::True, UINT64_MAX));
    // 1. Calculate Time & Matrix ONCE
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime      = std::chrono::high_resolution_clock::now();
    float time            = std::chrono::duration<float>(currentTime - startTime).count();

    // Store it in the class member
    currentModelMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // wait until the previous frame is finished
    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphore[currentFrame], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    // update uniform buffer before submitting the next frame
    updateUniformBuffer(currentFrame);

    // record command buffer
    device.resetFences(*inFlightFences[currentFrame]);
    commandBuffers[currentFrame].reset();
    recordCommandBuffer(imageIndex);

    // submit command buffer
    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{.waitSemaphoreCount   = 1,
                                    .pWaitSemaphores      = &*presentCompleteSemaphore[currentFrame],
                                    .pWaitDstStageMask    = &waitDestinationStageMask,
                                    .commandBufferCount   = 1,
                                    .pCommandBuffers      = &*commandBuffers[currentFrame],
                                    .signalSemaphoreCount = 1,
                                    .pSignalSemaphores    = &*renderFinishedSemaphore[imageIndex]};
    queue.submit(submitInfo, *inFlightFences[currentFrame]);

    //  submitting the result back to the swap chain to have it eventually show up on the screen
    try {
        const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1,
                                                .pWaitSemaphores    = &*renderFinishedSemaphore[imageIndex],
                                                .swapchainCount     = 1,
                                                .pSwapchains        = &*swapChain,
                                                .pImageIndices      = &imageIndex};
        result = queue.presentKHR(presentInfoKHR);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
            // need to do this after queueupresentKHR, or semaphores will not work correctly
            framebufferResized = false;
            //
            recreateSwapChain();
        } else if (result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    } catch (const vk::SystemError& e) {
        if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR)) {
            recreateSwapChain();
            return;
        } else {
            throw;
        }
    }
    // semaphoreIndex = (semaphoreIndex + 1) % presentCompleteSemaphore.size(); // No longer needed
    currentFrame   = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
