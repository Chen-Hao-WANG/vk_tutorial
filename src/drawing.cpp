#include "tutorial.hpp"

void HelloTriangleApplication::createCommandPool()
{
    // we have to create command pool before creating command buffers
    // command pool manages the memory for command buffers
    vk::CommandPoolCreateInfo poolInfo{.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                       .queueFamilyIndex = queueIndex};
    commandPool = vk::raii::CommandPool(device, poolInfo);
}
void HelloTriangleApplication::createCommandBuffers()
{
    /*
    1. allocate command buffers from the command pool
    2. decide level and number of command buffers
    3. depend on how many things we want to do in parallel
    4. by now we use MAX_FRAMES_IN_FLIGHT to decide the number of command buffers, because we want to record command buffers for each frame in flight
    */
    commandBuffers.clear();
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool,
                                            .level = vk::CommandBufferLevel::ePrimary,
                                            .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
    commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void HelloTriangleApplication::recordCommandBuffer(uint32_t imageIndex)
{
    // command buffer recording are used for draww operations and mamory transfer
    // those are not directly use vulkan funtion calls
    // but recorded into command buffers which are then submitted to queue for execution
    commandBuffers[currentFrame].begin({});
    // Starting dynamic rendering
    //  Before starting rendering, transition the swapchain image layout for rendering
    // in Vulkan, image can be in a layout optimal for specific operations
    // we need to transition the image layout to the appropriate layout before rendering
    transition_image_layout(
        swapChainImages[imageIndex],
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},                                                 // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,         // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,  // dstStage
        vk::ImageAspectFlagBits::eColor
    );
    transition_image_layout(
        *depthImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::ImageAspectFlagBits::eDepth);
    //
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
    // setup color attachment info
    vk::RenderingAttachmentInfo colorAttachmentInfo = {
        // which image view to render to
        .imageView = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        // what to do before rendering
        .loadOp = vk::AttachmentLoadOp::eClear,
        // what to do after rendering
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor};
    // setup dapth attachment info
    vk::RenderingAttachmentInfo depthAttachmentInfo = {
        .imageView = depthImageView,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = clearDepth};
    // set up rendering info
    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = {0, 0}, .extent = swapChainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo};

    /* ---begin rendering--- */
    commandBuffers[currentFrame].beginRendering(renderingInfo);
    // basic drawing commands
    // tell vulkan what to do by binding command buffers to graphics pipeline
    commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
    // we specify the viewport and scissor dynamically so we need to set them here
    commandBuffers[currentFrame].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
    commandBuffers[currentFrame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
    // bind vertex and index buffer
    commandBuffers[currentFrame].bindVertexBuffers(0, *vertexBuffer, {0});
    commandBuffers[currentFrame].bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);

    // bind the right descriptor set for each frame before draw indexed
    commandBuffers[currentFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptorSets[currentFrame], nullptr);
    // use drawIndexed to draw with index buffer, instead of vkCmdDraw
    commandBuffers[currentFrame].drawIndexed(indices.size(), 1, 0, 0, 0); // Finishing up
    /* ---end rendering--- */
    commandBuffers[currentFrame].endRendering();

    // After rendering, transition the image layout for presentation
    transition_image_layout(
        swapChainImages[imageIndex],
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,         // srcAccessMask
        {},                                                 // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe,          // dstStage
        vk::ImageAspectFlagBits::eColor
    );
    // end command buffer recording
    commandBuffers[currentFrame].end();
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
void HelloTriangleApplication::transition_image_layout(
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask,
    vk::ImageAspectFlags image_aspectMask)
{
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = image_aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1}};
    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier};
    commandBuffers[currentFrame].pipelineBarrier2(dependencyInfo);
}
void HelloTriangleApplication::createSyncObjects()
{
    /*
    1. decide fence(CPU, the host needs to know when the GPU has finished something) or semaphore to use
    2. what operations to synchronize
    3. how many synchronization objects are needed
    4. create synchronization objects
    */
    /*
    1. a semaphore to signal when an image has been acquired and is ready for rendering
    2. a semaphore to signal when rendering is finished and the image is ready for presentation
    3. a fence to ensure that the CPU waits for the GPU to finish rendering before starting the next frame
    */
    // clean up old synchronization objects if they exist
    presentCompleteSemaphore.clear();
    renderFinishedSemaphore.clear();
    inFlightFences.clear();
    // create new synchronization objects
    presentCompleteSemaphore.reserve(swapChainImages.size());
    renderFinishedSemaphore.reserve(swapChainImages.size());
    inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        presentCompleteSemaphore.emplace_back(device, vk::SemaphoreCreateInfo());
        renderFinishedSemaphore.emplace_back(device, vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        inFlightFences.emplace_back(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}
void HelloTriangleApplication::drawFrame()
{
    /*
    1. require fence to ensure that the CPU waits for the GPU to finish rendering before starting the next frame
    2. acquire an image from the swap chain
    3. update the uniform buffer
    4. record command buffer
    5. submit the command buffer
    6. present the image
    7. advance to the next frame
    */
    while (vk::Result::eTimeout == device.waitForFences(*inFlightFences[currentFrame], vk::True, UINT64_MAX))
        ;
    // wait until the previous frame is finished
    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphore[semaphoreIndex], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
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
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphore[semaphoreIndex],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffers[currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphore[imageIndex]};
    queue.submit(submitInfo, *inFlightFences[currentFrame]);

    //  submitting the result back to the swap chain to have it eventually show up on the screen
    try
    {
        const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1, .pWaitSemaphores = &*renderFinishedSemaphore[imageIndex], .swapchainCount = 1, .pSwapchains = &*swapChain, .pImageIndices = &imageIndex};
        result = queue.presentKHR(presentInfoKHR);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized)
        {
            // need to do this after queueupresentKHR, or semaphores will not work correctly
            framebufferResized = false;
            //
            recreateSwapChain();
        }
        else if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }
    catch (const vk::SystemError &e)
    {
        if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR))
        {
            recreateSwapChain();
            return;
        }
        else
        {
            throw;
        }
    }
    semaphoreIndex = (semaphoreIndex + 1) % presentCompleteSemaphore.size();
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
