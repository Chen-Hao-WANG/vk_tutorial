#include "tutorial.hpp"

void HelloTriangleApplication::createVertexBuffer()
{
    /*
    Create two ver
    */
    /*
    1. create buffer in VK need to fill VkBufferCreateInfo structure
    2. size : size of buffer in bytes
    3. usage : what the buffer will be used for (vertex buffer, index buffer, uniform buffer, etc.)
    4. sharingMode : how the buffer will be accessed (exclusive or concurrent)
    5. allocate memory for the buffer
    6. bind the buffer with the allocated memory
    */

    // specify a staging buffer to upload data to GPU memory
    vk::BufferCreateInfo stagingInfo{.size = sizeof(vertices[0]) * vertices.size(), .usage = vk::BufferUsageFlagBits::eTransferSrc, .sharingMode = vk::SharingMode::eExclusive};
    vk::raii::Buffer stagingBuffer = vk::raii::Buffer(device, stagingInfo);
    // allocate memory for staging buffer
    vk::MemoryRequirements memRequirementsStaging = stagingBuffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfoStaging{.allocationSize = memRequirementsStaging.size, .memoryTypeIndex = findMemoryType(memRequirementsStaging.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)};
    vk::raii::DeviceMemory stagingBufferMemory(device, memoryAllocateInfoStaging);
    stagingBuffer.bindMemory(stagingBufferMemory, 0);
    // copy vertex data to staging buffer
    void *dataStaging = stagingBufferMemory.mapMemory(0, stagingInfo.size);
    memcpy(dataStaging, vertices.data(), stagingInfo.size);
    stagingBufferMemory.unmapMemory();

    vk::BufferCreateInfo bufferInfo{.size = sizeof(vertices[0]) * vertices.size(), .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, .sharingMode = vk::SharingMode::eExclusive};
    vertexBuffer = vk::raii::Buffer(device, bufferInfo);

    vk::MemoryRequirements memRequirements = vertexBuffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfo{.allocationSize = memRequirements.size, .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)};
    vertexBufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);

    vertexBuffer.bindMemory(*vertexBufferMemory, 0);

    copyBuffer(stagingBuffer, vertexBuffer, stagingInfo.size);
}

uint32_t HelloTriangleApplication::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}
void HelloTriangleApplication::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer &buffer, vk::raii::DeviceMemory &bufferMemory)
{
    vk::BufferCreateInfo bufferInfo{.size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};
    buffer = vk::raii::Buffer(device, bufferInfo);
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{.allocationSize = memRequirements.size, .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)};
    bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
    buffer.bindMemory(*bufferMemory, 0);
}
void HelloTriangleApplication::copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size)
{
    /*
    copy data from srcBuffer to dstBuffer, the whole process is done by command buffer,just like drawing commands

    */
   // create a temporary command buffer for copy operation
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};
    vk::raii::CommandBuffer commandCopyBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());
    // start recording command buffer
    commandCopyBuffer.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy{.size = size});
    commandCopyBuffer.end();
    // submit command buffer
    queue.submit(vk::SubmitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandCopyBuffer}, nullptr);
    queue.waitIdle();
}
void HelloTriangleApplication::createIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::raii::Buffer stagingBuffer({});
    vk::raii::DeviceMemory stagingBufferMemory({});
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, indices.data(), (size_t) bufferSize);
    stagingBufferMemory.unmapMemory();

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}
