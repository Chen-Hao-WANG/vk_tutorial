#include "tutorial.hpp"
#include <random>
/*create light buffer and move it to SSBO for compute shaders*/

void HelloTriangleApplication::createLightBuffer()
{
    lights.resize(100);

    std::default_random_engine rndEngine(std::random_device{}());
    std::uniform_real_distribution<float> posDist(-1.0f, 1.0f);  // scene
    std::uniform_real_distribution<float> colorDist(0.5f, 1.0f); // brighter colors

    for (auto light : lights)
    {
        light.position = glm::vec4(posDist(rndEngine), posDist(rndEngine), posDist(rndEngine), 1.0f);
        light.color = glm::vec4(colorDist(rndEngine), colorDist(rndEngine), colorDist(rndEngine), 0.0f);
    }

    // create buffer
    vk::DeviceSize lightBufferSize = sizeof(Light) * lights.size();
    vk::raii::Buffer light_buffer({});
    vk::raii::DeviceMemory light_bufferMemory({});
    createBuffer(
        lightBufferSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal |
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        light_buffer,
        light_bufferMemory);

    /*zero copy*/
    void *data = light_bufferMemory.mapMemory(0, lightBufferSize);
    memcpy(data, lights.data(), static_cast<size_t>(lightBufferSize));
    light_bufferMemory.unmapMemory();
    std::cout << "[Info] Light Buffer created with APU Optimization (" << lights.size() << " lights)" << std::endl;
}