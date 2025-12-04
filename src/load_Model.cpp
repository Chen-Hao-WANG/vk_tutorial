#include "tutorial.hpp"
/**
 * @brief Load model from file using tinyobjloader
 *
 */
void HelloTriangleApplication::loadModel()
{
    // tinyobj attribute
    // shapes and materials

    // attrib holds the vertex data like positions, normals, texture coordinates
    tinyobj::attrib_t attrib;
    // shapes holds the info for each separate object and their faces(array of vertices) in the model
    std::vector<tinyobj::shape_t> shapes;
    // materials holds the material data for the model
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // error check
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    for (const auto &shape : shapes)
    {
        for (const auto &index : shape.mesh.indices)
        {
            Vertex vertex{};
            // load vertex position
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]};
            // load vertex normal
            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]};
            // load vertex texture coordinate
            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

            vertex.color = {1.0f, 1.0f, 1.0f}; // set default color to white
            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }
    }
}
