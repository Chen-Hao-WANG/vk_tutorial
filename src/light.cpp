#include <random>

#include "tutorial.hpp"
/*create light buffer and move it to SSBO for compute shaders*/

void HelloTriangleApplication::createLightBuffer() {
    lights.resize(100);

    std::default_random_engine rndEngine(std::random_device{}());

    // Spread X and Y out wider (e.g., -3 to 3)
    std::uniform_real_distribution<float> posDistXY(-3.0f, 3.0f);

    // Keep Z positive so lights are ABOVE the floor (e.g., 1.0 to 3.0)
    std::uniform_real_distribution<float> posDistZ(1.0f, 3.0f);

    std::uniform_real_distribution<float> colorDist(0.5f, 1.0f);

    for (auto& light : lights) {
        light.position = glm::vec4(posDistXY(rndEngine),  // X
                                   posDistXY(rndEngine),  // Y
                                   posDistZ(rndEngine),   // Z (Height)
                                   1.0f);
        light.color    = glm::vec4(colorDist(rndEngine), colorDist(rndEngine), colorDist(rndEngine), 0.0f);
    }
    // create buffer
    lightBufferResource.size = sizeof(Light) * lights.size();

    createBuffer(lightBufferResource.size,
                 vk::BufferUsageFlagBits::eStorageBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 lightBufferResource.buffer,
                 lightBufferResource.memory);

    /*zero copy*/
    void* data = lightBufferResource.memory.mapMemory(0, lightBufferResource.size);
    memcpy(data, lights.data(), static_cast<size_t>(lightBufferResource.size));
    lightBufferResource.memory.unmapMemory();
    std::cout << "[Info] Light Buffer created with APU Optimization (" << lights.size() << " lights)" << std::endl;
}