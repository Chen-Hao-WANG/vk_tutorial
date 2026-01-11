#include "tutorial.hpp"
/*
createDescriptorSetLayout():ubo for MVP transformation at vertex shader stage, CombinedImageSampler
for texture sampler at frag shader stage createDescriptorPool(): two pool size for ubo and texture
sampler createDescriptorSets():bind two different kind of discriptor info to desciptor set
*/

void HelloTriangleApplication::createDescriptorPool() {
    /*
    1. descriptor pool is used to allocate descriptor sets
    2. how many of each type of descriptor we need
    3. which type of disciptor we need
    4. specify max number of descriptor sets that can be allocated from the pool
    5. create the descriptor pool
    */
    std::array<vk::DescriptorPoolSize, 5> poolSizes;
    // uniform buffer
    poolSizes[0] = vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = MAX_FRAMES_IN_FLIGHT + 10};
    // texture sampler
    poolSizes[1] = vk::DescriptorPoolSize{.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = MAX_FRAMES_IN_FLIGHT + 10};
    // light buffer
    poolSizes[2] = vk::DescriptorPoolSize{
        .type            = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = MAX_FRAMES_IN_FLIGHT + 10  // some work around number
    };
    // storage image
    poolSizes[3] = vk::DescriptorPoolSize{
        .type            = vk::DescriptorType::eStorageImage,
        .descriptorCount = MAX_FRAMES_IN_FLIGHT + 10  // some work around number
    };

    poolSizes[4] = vk::DescriptorPoolSize{
        .type            = vk::DescriptorType::eAccelerationStructureKHR,
        .descriptorCount = 2  // some work around number
    };
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + 10),  // total number of descriptor sets that can be allocated
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes    = poolSizes.data()};

    descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}

/*
every binding need to be described in vk::DescriptorSetLayoutBinding structure
    1. binding : binding number in shader
    2. descriptorType : type of resource (uniform buffer, storage buffer, image sampler, etc.)
    3. descriptorCount : number of descriptors in this binding
    4. stageFlags : which shader stages will access this binding
    5. pImmutableSamplers : used for image sampler, can be nullptr for uniform buffer
    */
void HelloTriangleApplication::createDescriptorSetLayout() {
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings;

    // binding 0 : uniform buffer object (MVP matrices)
    bindings[0] = vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);

    // binding 1 : combined image sampler (texture sampler)
    bindings[1] = vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr);

    vk::DescriptorSetLayoutCreateInfo layoutInfo{.bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()};
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}
/*
    1. descriptor sets are describe by vk::DesciptorSetLayoutAllocateInfo structure
    2. specify the descriptor pool and layouts for each descriptor set
    3. allocate the descriptor sets from the pool using vkAllocateDescriptorSets
    4. use for loop to update each descriptor set with the corresponding uniform buffer info
    */
void HelloTriangleApplication::createDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool, .descriptorSetCount = static_cast<uint32_t>(layouts.size()), .pSetLayouts = layouts.data()};

    descriptorSets.clear();
    descriptorSets = device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{.buffer = uniformBuffers[i], .offset = 0, .range = sizeof(UniformBufferObject)};
        vk::DescriptorImageInfo imageInfo{
            .sampler = viking_room.textureSampler, .imageView = viking_room.textureImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};
        //
        std::array descriptorWrites{// now we have two descriptor writes
                                    // one for uniform buffer and one for texture sampler
                                    vk::WriteDescriptorSet{.dstSet          = descriptorSets[i],
                                                           .dstBinding      = 0,
                                                           .dstArrayElement = 0,
                                                           .descriptorCount = 1,
                                                           .descriptorType  = vk::DescriptorType::eUniformBuffer,
                                                           .pBufferInfo     = &bufferInfo},
                                    vk::WriteDescriptorSet{.dstSet          = descriptorSets[i],
                                                           .dstBinding      = 1,
                                                           .dstArrayElement = 0,
                                                           .descriptorCount = 1,
                                                           .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                                                           .pImageInfo      = &imageInfo}};
        //
        device.updateDescriptorSets(descriptorWrites, {});
    }
}
void HelloTriangleApplication::createComputeDescriptorSetLayout() {
    /*
    0. G-Buffer Position (Input Texture)
    1. G-Buffer Normal (Input Texture)
    2. G-Buffer Alebedo (Input Texture)
    3. Light Buffer
    4. Storage Image (Output Image)
    5. TLAS (for ray tracing)
    */
    std::array<vk::DescriptorSetLayoutBinding, 6> bindings;
    // Binding 0: G-Buffer Position (Input Texture)
    bindings[0] = vk::DescriptorSetLayoutBinding{.binding         = 0,
                                                 .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                                                 .descriptorCount = 1,
                                                 .stageFlags      = vk::ShaderStageFlagBits::eCompute};

    // Binding 1: G-Buffer Normal (Input Texture)
    bindings[1] = vk::DescriptorSetLayoutBinding{.binding         = 1,
                                                 .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                                                 .descriptorCount = 1,
                                                 .stageFlags      = vk::ShaderStageFlagBits::eCompute};

    // Binding 2: G-Buffer Alebedo (Input Texture)
    bindings[2] = vk::DescriptorSetLayoutBinding{.binding         = 2,
                                                 .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                                                 .descriptorCount = 1,
                                                 .stageFlags      = vk::ShaderStageFlagBits::eCompute};
    // Binding 3: Light Buffer
    bindings[3] = vk::DescriptorSetLayoutBinding{
        .binding = 3, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute};

    // Binding 4: Storage Image (Output Image)
    bindings[4] = vk::DescriptorSetLayoutBinding{
        .binding = 4, .descriptorType = vk::DescriptorType::eStorageImage, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute};

    // Binding 5: TLAS (for ray tracing)
    bindings[5] = vk::DescriptorSetLayoutBinding{.binding         = 5,
                                                 .descriptorType  = vk::DescriptorType::eAccelerationStructureKHR,
                                                 .descriptorCount = 1,
                                                 .stageFlags      = vk::ShaderStageFlagBits::eCompute};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{.bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()};

    computeDescriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}
void HelloTriangleApplication::createComputeDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, computeDescriptorSetLayout);
    // Set
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool, .descriptorSetCount = static_cast<uint32_t>(layouts.size()), .pSetLayouts = layouts.data()};

    computeDescriptorSets = device.allocateDescriptorSets(allocInfo);

    // For legacy compatibility if you use computeDescriptorSet[0] elsewhere temporarily,
    // but better to switch usage to updateDescriptorSets[currentFrame]

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Write descriptor set info
        vk::DescriptorImageInfo posInfo{
            .sampler = *viking_room.textureSampler, .imageView = gBufferPositionImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

        vk::DescriptorImageInfo normalInfo{
            .sampler = *viking_room.textureSampler, .imageView = gBufferNormalImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

        vk::DescriptorImageInfo albedoInfo{
            .sampler = *viking_room.textureSampler, .imageView = gBufferAlbedoImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

        vk::DescriptorBufferInfo lightBufferInfo{.buffer = lightBufferResource.buffer, .offset = 0, .range = sizeof(Light) * lights.size()};

        vk::DescriptorImageInfo outputInfo{
            .imageView   = storageImageView,
            .imageLayout = vk::ImageLayout::eGeneral  // for compute shader must be general layout
        };

        // FIX: Use tlas[i]
        vk::WriteDescriptorSetAccelerationStructureKHR asInfo{
            .accelerationStructureCount = 1,
            .pAccelerationStructures    = &*tlas[i]  // point to TLAS for this frame
        };

        // FIX: Update the specific descriptor set for this frame
        vk::WriteDescriptorSet asWrite{.pNext           = &asInfo,  // AS is linked via pNext
                                       .dstSet          = *computeDescriptorSets[i],
                                       .dstBinding      = 5,  // layout binding 5
                                       .descriptorCount = 1,
                                       .descriptorType  = vk::DescriptorType::eAccelerationStructureKHR};

        // Write descriptor set
        std::array<vk::WriteDescriptorSet, 6> descriptorWrites;
        // G-Buffer Position
        descriptorWrites[0] = vk::WriteDescriptorSet{.dstSet          = *computeDescriptorSets[i],
                                                     .dstBinding      = 0,
                                                     .dstArrayElement = 0,
                                                     .descriptorCount = 1,
                                                     .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                                                     .pImageInfo      = &posInfo};
        // G-Buffer Normal
        descriptorWrites[1] = vk::WriteDescriptorSet{.dstSet          = *computeDescriptorSets[i],
                                                     .dstBinding      = 1,
                                                     .dstArrayElement = 0,
                                                     .descriptorCount = 1,
                                                     .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                                                     .pImageInfo      = &normalInfo};
        // G-Buffer Alebedo
        descriptorWrites[2] = vk::WriteDescriptorSet{.dstSet          = *computeDescriptorSets[i],
                                                     .dstBinding      = 2,
                                                     .dstArrayElement = 0,
                                                     .descriptorCount = 1,
                                                     .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                                                     .pImageInfo      = &albedoInfo};
        // Light Buffer
        descriptorWrites[3] = vk::WriteDescriptorSet{.dstSet          = *computeDescriptorSets[i],
                                                     .dstBinding      = 3,
                                                     .dstArrayElement = 0,
                                                     .descriptorCount = 1,
                                                     .descriptorType  = vk::DescriptorType::eStorageBuffer,
                                                     .pBufferInfo     = &lightBufferInfo};
        // Storage Image
        descriptorWrites[4] = vk::WriteDescriptorSet{.dstSet          = *computeDescriptorSets[i],
                                                     .dstBinding      = 4,
                                                     .dstArrayElement = 0,
                                                     .descriptorCount = 1,
                                                     .descriptorType  = vk::DescriptorType::eStorageImage,
                                                     .pImageInfo      = &outputInfo};

        // TLAS
        descriptorWrites[5] = asWrite;
        device.updateDescriptorSets(descriptorWrites, {});
    }
}
