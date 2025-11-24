#include "tutorial.hpp"
void HelloTriangleApplication::createDescriptorSetLayout()
{
    /*
    every binding need to be described in vk::DescriptorSetLayoutBinding structure
    1. binding : binding number in shader
    2. descriptorType : type of resource (uniform buffer, storage buffer, image sampler, etc.)
    3. descriptorCount : number of descriptors in this binding
    4. stageFlags : which shader stages will access this binding
    5. pImmutableSamplers : used for image sampler, can be nullptr for uniform buffer
    */
    vk::DescriptorSetLayoutBinding uboLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);
    vk::DescriptorSetLayoutCreateInfo layoutInfo{.bindingCount = 1, .pBindings = &uboLayoutBinding};
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}
void HelloTriangleApplication::createUniformBuffers()
{
    /*
    map uniform buffer using vk::DeviceMemory::mapMemory
    and keep it mapped for the entire duration of the application,
    this is called "persistent mapping"
    but careful for performance because mapping is not free, so avoid frequent mapping and unmapping
    */
    uniformBuffers.clear();
    uniformBuffersMemory.clear();
    uniformBuffersMapped.clear();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        vk::raii::Buffer buffer({});
        vk::raii::DeviceMemory bufferMem({});
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, bufferMem);
        uniformBuffers.emplace_back(std::move(buffer));
        uniformBuffersMemory.emplace_back(std::move(bufferMem));
        uniformBuffersMapped.emplace_back(uniformBuffersMemory[i].mapMemory(0, bufferSize));
    }
}
void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage)
{
    /*
    1. using uniform buffer object to pass transformation matrices to the vertex shader
    2. it is not efficient to update the uniform buffer every frame for multiple frames in flight, better to use push constants or dynamic uniform buffers for larger data
    3. here we just use a simple example to demonstrate how to update the uniform buffer
    */
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void HelloTriangleApplication::createDescriptorPool()
{
    /*
    1. descriptor pool is used to allocate descriptor sets
    2. how many of each type of descriptor we need
    3. which type of disciptor we need
    4. specify max number of descriptor sets that can be allocated from the pool
    5. create the descriptor pool
    */
    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT);
    vk::DescriptorPoolCreateInfo poolInfo{.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, .maxSets = MAX_FRAMES_IN_FLIGHT, .poolSizeCount = 1, .pPoolSizes = &poolSize};
    descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}

void HelloTriangleApplication::createDescriptorSets()
{
    /*
    1. descriptor sets are describe by vk::DesciptorSetLayoutAllocateInfo structure
    2. specify the descriptor pool and layouts for each descriptor set
    3. allocate the descriptor sets from the pool using vkAllocateDescriptorSets
    4. use for loop to update each descriptor set with the corresponding uniform buffer info
    */
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{.descriptorPool = descriptorPool, .descriptorSetCount = static_cast<uint32_t>(layouts.size()), .pSetLayouts = layouts.data()};

    descriptorSets.clear();
    descriptorSets = device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo bufferInfo{.buffer = uniformBuffers[i], .offset = 0, .range = sizeof(UniformBufferObject)};
        vk::WriteDescriptorSet descriptorWrite{.dstSet = descriptorSets[i], .dstBinding = 0, .dstArrayElement = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &bufferInfo};
        device.updateDescriptorSets(descriptorWrite, {});
    }
}