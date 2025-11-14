#include "tutorial.hpp"

void HelloTriangleApplication::createSyncObjects()
{
    presentCompleteSemaphore = vk::raii::Semaphore(device, vk::SemaphoreCreateInfo());
    renderFinishedSemaphore = vk::raii::Semaphore(device, vk::SemaphoreCreateInfo());
    drawFence = vk::raii::Fence(device, {.flags = vk::FenceCreateFlagBits::eSignaled});
}
void HelloTriangleApplication::drawFrame()
{
    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphore, nullptr);
}