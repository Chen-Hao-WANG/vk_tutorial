#include "tutorial.hpp"
/*
create texture image we need:
1. create a image object which backed by device memory
2. fill the object with pixel data
3. create a sampler to sample the image in the shader
4. add a combined image sampler descriptor to sample from the texture
*/
void HelloTriangleApplication::createTextureImage()
{
    /*
    1. specify image width, height, format, tiling, usage, memory properties
    2. create buffer in host visible memory to load pixel data
    3. staging buffer to device local image
    */
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    // create staging buffer in host visible memory
    vk::raii::Buffer stagingBuffer({});
    vk::raii::DeviceMemory stagingBufferMemory({});
    createBuffer(
        imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory);

    // copy pixel data to staging buffer
    void *data = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(data, pixels, imageSize);
    stagingBufferMemory.unmapMemory();
    // free image memory loaded by stb_image
    stbi_image_free(pixels);

    // better to access pixel value by using image object(Pixels within an image object are known as texels)
    vk::raii::Image textureImage = nullptr;
    vk::raii::DeviceMemory textureImageMemory = nullptr;

    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive};

    image = vk::raii::Image(device, imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo(memRequirements.size, findMemoryType(memRequirements.memoryTypeBits, properties));
    imageMemory = vk::raii::DeviceMemory(device, allocInfo);
    image.bindMemory(*imageMemory, 0);
}