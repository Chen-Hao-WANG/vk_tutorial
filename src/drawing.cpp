#include "tutorial.hpp"

void HelloTriangleApplication::createCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo{.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                       .queueFamilyIndex = queueIndex};
    commandPool = vk::raii::CommandPool(device, poolInfo);
}
void HelloTriangleApplication::createCommandBuffer()
{
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};
    commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());
}
