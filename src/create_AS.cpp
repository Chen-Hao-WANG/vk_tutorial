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
            .size = buildSizes.accelerationStructureSize,
            .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
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

    // instance buffer
    vk::DeviceSize instanceBufferSize = sizeof(vk::AccelerationStructureInstanceKHR) * instances.size();
    // staging buffer to upload instance data
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

    createBuffer(instanceBufferSize,
                 vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                     vk::BufferUsageFlagBits::eTransferDst,
                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                 instanceBuffer,
                 instanceMemory);
    copyBuffer(stagingBuffer, instanceBuffer, instanceBufferSize);

    // TLAS info(point to instance buffer)
    vk::BufferDeviceAddressInfo instanceBufAddrInfo{.buffer = *instanceBuffer};
    vk::DeviceAddress instanceAddr = device.getBufferAddressKHR(instanceBufAddrInfo);

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData{.arrayOfPointers = vk::False, .data = instanceAddr};

    vk::AccelerationStructureGeometryKHR tlasGeometry{.geometryType = vk::GeometryTypeKHR::eInstances, .geometry = instancesData};

    vk::AccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{.type          = vk::AccelerationStructureTypeKHR::eTopLevel,
                                                                .mode          = vk::BuildAccelerationStructureModeKHR::eBuild,
                                                                .geometryCount = 1,
                                                                .pGeometries   = &tlasGeometry};

    uint32_t instanceCount = static_cast<uint32_t>(instances.size());
    vk::AccelerationStructureBuildSizesInfoKHR tlasBuildSizes =
        device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, tlasBuildInfo, {instanceCount});

    // create TLAS buffer
    createBuffer(tlasBuildSizes.accelerationStructureSize,
                 vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                 tlasBuffer,
                 tlasMemory);

    // create TLAS handle
    vk::AccelerationStructureCreateInfoKHR tlasCreateInfo{
        .buffer = *tlasBuffer, .offset = 0, .size = tlasBuildSizes.accelerationStructureSize, .type = vk::AccelerationStructureTypeKHR::eTopLevel};

    tlas                                   = device.createAccelerationStructureKHR(tlasCreateInfo);
    tlasBuildInfo.dstAccelerationStructure = *tlas;

    // scratch buffer for TLAS build
    vk::raii::Buffer tlasScratchBuffer       = nullptr;
    vk::raii::DeviceMemory tlasScratchMemory = nullptr;
    createBuffer(tlasBuildSizes.buildScratchSize,
                 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                 tlasScratchBuffer,
                 tlasScratchMemory);
    vk::BufferDeviceAddressInfo tlasScratchAddrInfo{.buffer = *tlasScratchBuffer};
    tlasBuildInfo.scratchData.deviceAddress = device.getBufferAddressKHR(tlasScratchAddrInfo);

    // TLAS build range
    vk::AccelerationStructureBuildRangeInfoKHR tlasRange{
        .primitiveCount = instanceCount, .primitiveOffset = 0, .firstVertex = 0, .transformOffset = 0};

    auto cmd = beginSingleTimeCommands();
    cmd->buildAccelerationStructuresKHR({tlasBuildInfo}, {&tlasRange});
    endSingleTimeCommands(*cmd);
}