#include "tutorial.hpp"
/**
 * @brief Load model from file using tinyobjloader
 *
 */
void HelloTriangleApplication::loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // error check
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    uint32_t globalIndexOffset = 0;

    for (const auto& shape : shapes) {
        uint32_t shapeStartIndex = globalIndexOffset;
        uint32_t localMaxV       = 0;
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            // load vertex position
            vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                          attrib.vertices[3 * index.vertex_index + 1],
                          attrib.vertices[3 * index.vertex_index + 2]};
            // load vertex normal
            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1], attrib.normals[3 * index.normal_index + 2]};
            // load vertex texture coordinate
            vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

            vertex.color = {1.0f, 1.0f, 1.0f};  // set default color to white
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            uint32_t currentVertexIndex = uniqueVertices[vertex];
            indices.push_back(currentVertexIndex);
            // update  counters
            globalIndexOffset++;
            localMaxV = std::max(localMaxV, currentVertexIndex);
        }
        // store submesh info
        SubMesh submesh{};
        submesh.indexOffset = shapeStartIndex;
        submesh.indexCount  = globalIndexOffset - shapeStartIndex;
        submesh.maxVertex   = localMaxV;
        submesh.alphaCut    = (shape.name.find("nettle_plant") != std::string::npos);

        submeshes.push_back(submesh);
    }
}
