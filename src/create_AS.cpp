#include "tutorial.hpp"

void HelloTriangleApplication::createAccelerationStructures() {
    // get vertex and index buffer device address
    vk::DeviceAddress vertexAddr = getVertAddress(vertexBuffer);
    vk::DeviceAddress indexAddr  = getIndexAddr(indexBuffer);

    // reserve space for BLAS handles, buffers and memories
    blasHandles.reserve(submeshes.size());
    blasBuffers.reserve(submeshes.size());
    blasMemories.reserve(submeshes.size());

    // create BLAS for each submesh
    for (auto& submesh : submeshes) {
        vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData{.vertexFormat = vk::Format::eR32G32B32Sfloat,
                                                                        .vertexData   = vertexAddr,
                                                                        .vertexStride = sizeof(Vertex),
                                                                        .maxVertex    = submesh.maxVertex,
                                                                        .indexType    = vk::IndexType::eUint32,
                                                                        .indexData    = indexAddr + submesh.indexOffset * sizeof(uint32_t)};
        vk::AccelerationStructureGeometryDataKHR geometryData(trianglesData);

        vk::AccelerationStructureGeometryKHR blasGeometry{
            .geometryType = vk::GeometryTypeKHR::eTriangles,
            .geometry     = geometryData,
            .flags        = vk::GeometryFlagBitsKHR::eOpaque,
        };

        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{
            .type          = vk::AccelerationStructureTypeKHR::eBottomLevel,
            .mode          = vk::BuildAccelerationStructureModeKHR::eBuild,
            .geometryCount = 1,
            .pGeometries   = &blasGeometry,
        };

        uint32_t primitiveCount = submesh.indexCount / 3;
        vk::AccelerationStructureBuildSizesInfoKHR buildSizes =
            device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, {primitiveCount});

        vk::raii::Buffer blasBuffer       = nullptr;
        vk::raii::DeviceMemory blasMemory = nullptr;
        createBuffer(buildSizes.accelerationStructureSize,
                     vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     blasBuffer,
                     blasMemory);

        // store buffer and memory in vectors
        blasBuffers.push_back(std::move(blasBuffer));
        blasMemories.push_back(std::move(blasMemory));

        // BLAS Handle
        vk::AccelerationStructureCreateInfoKHR blasCreateInfo{
            .buffer = *blasBuffers.back(),
            .offset = 0,
            .size   = buildSizes.accelerationStructureSize,
            .type   = vk::AccelerationStructureTypeKHR::eBottomLevel,
        };
        blasHandles.push_back(device.createAccelerationStructureKHR(blasCreateInfo));

        // scratch buffer
        vk::raii::Buffer scratchBuffer       = nullptr;
        vk::raii::DeviceMemory scratchMemory = nullptr;
        createBuffer(buildSizes.buildScratchSize,
                     vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     scratchBuffer,
                     scratchMemory);
        // scratch buffer device address
        vk::BufferDeviceAddressInfo scratchAddrInfo{.buffer = *scratchBuffer};
        buildInfo.scratchData.deviceAddress = device.getBufferAddressKHR(scratchAddrInfo);
        buildInfo.dstAccelerationStructure  = *blasHandles.back();

        vk::AccelerationStructureBuildRangeInfoKHR buildRange{
            .primitiveCount = primitiveCount, .primitiveOffset = 0, .firstVertex = 0, .transformOffset = 0};

        auto cmd = beginSingleTimeCommands();
        cmd->buildAccelerationStructuresKHR({buildInfo}, {&buildRange});
        endSingleTimeCommands(*cmd);
    }

    // create TLAS
    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    instances.reserve(submeshes.size());

    vk::TransformMatrixKHR transformMatrix = {std::array<std::array<float, 4>, 3>{
        std::array<float, 4>{1.0f, 0.0f, 0.0f, 0.0f}, std::array<float, 4>{0.0f, 1.0f, 0.0f, 0.0f}, std::array<float, 4>{0.0f, 0.0f, 1.0f, 0.0f}}};

    // instance list(put every BLAS into the instance list)
    for (size_t i = 0; i < blasHandles.size(); i++) {
        vk::AccelerationStructureDeviceAddressInfoKHR addressInfo{.accelerationStructure = *blasHandles[i]};
        vk::DeviceAddress blasAddress = device.getAccelerationStructureAddressKHR(addressInfo);

        vk::AccelerationStructureInstanceKHR instance{};
        instance.transform                              = transformMatrix;
        instance.instanceCustomIndex                    = i;  // material index
        instance.mask                                   = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags                                  = static_cast<uint32_t>(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
        instance.accelerationStructureReference         = blasAddress;

        instances.push_back(instance);
    }

    // Resize vectors for double buffering
    tlas.clear();
    tlasBuffer.clear();
    tlasMemory.clear();
    tlasScratchBuffer.clear();
    tlasScratchMemory.clear();
    instanceBuffer.clear();
    instanceMemory.clear();

    tlas.reserve(MAX_FRAMES_IN_FLIGHT);
    tlasBuffer.reserve(MAX_FRAMES_IN_FLIGHT);
    tlasMemory.reserve(MAX_FRAMES_IN_FLIGHT);
    tlasScratchBuffer.reserve(MAX_FRAMES_IN_FLIGHT);
    tlasScratchMemory.reserve(MAX_FRAMES_IN_FLIGHT);
    instanceBuffer.reserve(MAX_FRAMES_IN_FLIGHT);
    instanceMemory.reserve(MAX_FRAMES_IN_FLIGHT);

    // staging buffer to upload instance data
    vk::DeviceSize instanceBufferSize = sizeof(vk::AccelerationStructureInstanceKHR) * instances.size();

    vk::raii::Buffer stagingBuffer       = nullptr;
    vk::raii::DeviceMemory stagingMemory = nullptr;
    createBuffer(instanceBufferSize,
                 vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer,
                 stagingMemory);

    void* data = stagingMemory.mapMemory(0, instanceBufferSize);
    memcpy(data, instances.data(), instanceBufferSize);
    stagingMemory.unmapMemory();

    // Create TLAS resources for EACH frame
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // 1. Create Instance Buffer
        vk::raii::Buffer instBuf       = nullptr;
        vk::raii::DeviceMemory instMem = nullptr;
        createBuffer(instanceBufferSize,
                     vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                         vk::BufferUsageFlagBits::eTransferDst,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     instBuf,
                     instMem);

        instanceBuffer.push_back(std::move(instBuf));
        instanceMemory.push_back(std::move(instMem));

        // Copy initial data
        copyBuffer(stagingBuffer, instanceBuffer.back(), instanceBufferSize);

        // 2. Get Instance Buffer Address
        vk::BufferDeviceAddressInfo instanceBufAddrInfo{.buffer = *instanceBuffer.back()};
        vk::DeviceAddress instanceAddr = device.getBufferAddressKHR(instanceBufAddrInfo);

        vk::AccelerationStructureGeometryInstancesDataKHR instancesData{.arrayOfPointers = vk::False, .data = instanceAddr};
        vk::AccelerationStructureGeometryKHR tlasGeometry{.geometryType = vk::GeometryTypeKHR::eInstances, .geometry = instancesData};

        vk::AccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{.type          = vk::AccelerationStructureTypeKHR::eTopLevel,
                                                                    .flags         = vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate,
                                                                    .mode          = vk::BuildAccelerationStructureModeKHR::eBuild,
                                                                    .geometryCount = 1,
                                                                    .pGeometries   = &tlasGeometry};

        uint32_t instanceCount = static_cast<uint32_t>(instances.size());
        vk::AccelerationStructureBuildSizesInfoKHR tlasBuildSizes =
            device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, tlasBuildInfo, {instanceCount});

        // 3. Create TLAS Buffer
        vk::raii::Buffer tBuffer       = nullptr;
        vk::raii::DeviceMemory tMemory = nullptr;
        createBuffer(tlasBuildSizes.accelerationStructureSize,
                     vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     tBuffer,
                     tMemory);
        tlasBuffer.push_back(std::move(tBuffer));
        tlasMemory.push_back(std::move(tMemory));

        // 4. Create TLAS Handle
        vk::AccelerationStructureCreateInfoKHR tlasCreateInfo{.buffer = *tlasBuffer.back(),
                                                              .offset = 0,
                                                              .size   = tlasBuildSizes.accelerationStructureSize,
                                                              .type   = vk::AccelerationStructureTypeKHR::eTopLevel};

        tlas.push_back(device.createAccelerationStructureKHR(tlasCreateInfo));
        tlasBuildInfo.dstAccelerationStructure = *tlas.back();

        // 5. Create Scratch Buffer
        vk::raii::Buffer sBuffer       = nullptr;
        vk::raii::DeviceMemory sMemory = nullptr;
        createBuffer(tlasBuildSizes.buildScratchSize,
                     vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     sBuffer,
                     sMemory);
        tlasScratchBuffer.push_back(std::move(sBuffer));
        tlasScratchMemory.push_back(std::move(sMemory));

        vk::BufferDeviceAddressInfo tlasScratchAddrInfo{.buffer = *tlasScratchBuffer.back()};
        tlasBuildInfo.scratchData.deviceAddress = device.getBufferAddressKHR(tlasScratchAddrInfo);

        // 6. Build Initial TLAS
        vk::AccelerationStructureBuildRangeInfoKHR tlasRange{
            .primitiveCount = instanceCount, .primitiveOffset = 0, .firstVertex = 0, .transformOffset = 0};

        auto cmd = beginSingleTimeCommands();
        cmd->buildAccelerationStructuresKHR({tlasBuildInfo}, {&tlasRange});
        endSingleTimeCommands(*cmd);
    }
}
void HelloTriangleApplication::updateTLAS(const vk::raii::CommandBuffer& cmd) {
    glm::mat4 spin        = currentModelMatrix;
    glm::mat4 standUp     = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 bunnyMatrix = spin * standUp;
    glm::mat4 wallMatrix  = glm::mat4(1.0f);

    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    instances.reserve(blasHandles.size());

    // B. Re-generate Instances with new Transform

    for (size_t i = 0; i < blasHandles.size(); i++) {
        vk::AccelerationStructureDeviceAddressInfoKHR addressInfo{.accelerationStructure = *blasHandles[i]};
        vk::DeviceAddress blasAddress = device.getAccelerationStructureAddressKHR(addressInfo);

        // [FIX] Use same logic: Last one is Box, others are Bunny
        bool isCornellBox       = (i == blasHandles.size() - 1);
        glm::mat4 selectedModel = isCornellBox ? wallMatrix : bunnyMatrix;

        vk::TransformMatrixKHR transform;
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 4; c++) {
                transform.matrix[r][c] = selectedModel[c][r];
            }
        }

        vk::AccelerationStructureInstanceKHR instance{};
        instance.transform                              = transform;
        instance.instanceCustomIndex                    = i;
        instance.mask                                   = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags                                  = static_cast<uint32_t>(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
        instance.accelerationStructureReference         = blasAddress;
        instances.push_back(instance);
    }

    // C. Upload to Buffer
    // FIX 1: Use 3 arguments. 'instances' vector converts to ArrayProxy automatically.
    // Use instanceBuffer[currentFrame]
    cmd.updateBuffer<vk::AccelerationStructureInstanceKHR>(*instanceBuffer[currentFrame], 0, instances);

    // D. Barrier: Ensure upload finishes before Build reads it
    // FIX: Add eShaderRead to dstAccessMask because instance buffer reads via device address count as shader reads
    vk::MemoryBarrier2 transferBarrier{.srcStageMask  = vk::PipelineStageFlagBits2::eTransfer,
                                       .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
                                       .dstStageMask  = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
                                       .dstAccessMask = vk::AccessFlagBits2::eAccelerationStructureReadKHR | vk::AccessFlagBits2::eShaderRead};
    vk::DependencyInfo depInfo{.memoryBarrierCount = 1, .pMemoryBarriers = &transferBarrier};
    cmd.pipelineBarrier2(depInfo);

    // E. Build TLAS
    vk::BufferDeviceAddressInfo instanceBufAddrInfo{.buffer = *instanceBuffer[currentFrame]};
    vk::DeviceAddress instanceAddr = device.getBufferAddressKHR(instanceBufAddrInfo);

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData{.arrayOfPointers = vk::False, .data = instanceAddr};
    vk::AccelerationStructureGeometryKHR tlasGeometry{.geometryType = vk::GeometryTypeKHR::eInstances, .geometry = instancesData};

    // FIX 2: Construct DeviceOrHostAddressKHR explicitly
    // Use tlasScratchBuffer[currentFrame]
    vk::DeviceAddress scratchAddr = device.getBufferAddressKHR({.buffer = *tlasScratchBuffer[currentFrame]});

    vk::AccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{.type                     = vk::AccelerationStructureTypeKHR::eTopLevel,
                                                                .flags                    = vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate,
                                                                .mode                     = vk::BuildAccelerationStructureModeKHR::eUpdate,
                                                                .srcAccelerationStructure = *tlas[currentFrame],
                                                                .dstAccelerationStructure = *tlas[currentFrame],
                                                                .geometryCount            = 1,
                                                                .pGeometries              = &tlasGeometry,
                                                                .scratchData              = vk::DeviceOrHostAddressKHR(scratchAddr)};

    vk::AccelerationStructureBuildRangeInfoKHR tlasRange{
        .primitiveCount = static_cast<uint32_t>(instances.size()), .primitiveOffset = 0, .firstVertex = 0, .transformOffset = 0};
    const vk::AccelerationStructureBuildRangeInfoKHR* pRange = &tlasRange;

    // FIX 3: Remove '1' (count) and use brackets {} to create ArrayProxy
    cmd.buildAccelerationStructuresKHR(tlasBuildInfo, pRange);

    // F. Barrier: Ensure Build finishes before Compute Shader reads it
    vk::MemoryBarrier2 buildBarrier{.srcStageMask  = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
                                    .srcAccessMask = vk::AccessFlagBits2::eAccelerationStructureWriteKHR,
                                    .dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
                                    .dstAccessMask = vk::AccessFlagBits2::eShaderRead};
    vk::DependencyInfo buildDepInfo{.memoryBarrierCount = 1, .pMemoryBarriers = &buildBarrier};
    cmd.pipelineBarrier2(buildDepInfo);
}