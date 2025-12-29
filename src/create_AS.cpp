#include "tutorial.hpp"

void HelloTriangleApplication::createAccelerationStructures() {
    // Create a BLAS for each submesh

    // Prepare the geometry data
    auto trianglesData = vk::AcclerationStructureGeometryTrianglesDataKHR{.vertexFormat = vk::Format::eR32G32B32Sfloat,
                                                                          .vertexData = vertexAddr,
                                                                          .vertexStride = sizeof(Vertex),
                                                                          .maxVertex = submesh.maxVertex,
                                                                          .indexType = vk::IndexType::eUint32,
                                                                          .indexData = indexAddr + submesh.indexOffset * sizeof(uint32_t)};
    vk::AccelerationGeometryDataKHR geometryData(trianglesData);
    vk::AccelerationStructureGeometryKHR blasGeometry{
        .geometryType = vk::GeometryTypeKHR::eTriangles, .geometry = geometryData, .flags = vk::GeometryFlagBitsKHR::eOpaque};

    // record the geometry info into a BLAS build info
    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{
        .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
        .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
        .geometryCount = 1,
        .pGeometries = &blasGeometry,
    };
    //  Query the memory sizes that will be needed for this BLAS
    vk::AccelerationStructureBuildSizesInfoKHR blasBuildSizes =
        device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, blasBuildGeometryInfo, {primitiveCount});
}