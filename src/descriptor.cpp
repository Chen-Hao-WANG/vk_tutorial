#include "tutorial.hpp"
/*
createDescriptorSetLayout():ubo for MVP transformation at vertex shader stage, CombinedImageSampler for texture sampler at frag shader stage
createDescriptorPool(): two pool size for ubo and texture sampler
createDescriptorSets():bind two different kind of discriptor info to desciptor set
*/
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
    // create an array of bindings if multiple bindings are needed
    std::array bindings = {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr)};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{.bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()};
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
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
    std::array poolSize{
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT)};

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
        .pPoolSizes = poolSize.data()};

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
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()};

    descriptorSets.clear();
    descriptorSets = device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo bufferInfo{.buffer = uniformBuffers[i], .offset = 0, .range = sizeof(UniformBufferObject)};
        vk::DescriptorImageInfo imageInfo{.sampler = textureSampler, .imageView = textureImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};
        //
        std::array descriptorWrites{
            vk::WriteDescriptorSet{.dstSet = descriptorSets[i], .dstBinding = 0, .dstArrayElement = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &bufferInfo},
            vk::WriteDescriptorSet{.dstSet = descriptorSets[i], .dstBinding = 1, .dstArrayElement = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &imageInfo}};
        //
        device.updateDescriptorSets(descriptorWrites, {});
    }
}