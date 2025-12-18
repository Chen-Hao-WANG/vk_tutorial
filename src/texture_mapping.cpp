#include "tutorial.hpp"
/** @brief create texture image we need:
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
    4. copy pixel data from staging buffer to image object
    5. transition image layout for shader reading
    */
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    // create staging buffer in host visible memory
    vk::raii::Buffer stagingBuffer({});
    vk::raii::DeviceMemory stagingBufferMemory({});
    // this buffer should be in host visible memory so that we can map it as a transfer source to copy to the image
    createBuffer(
        imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory);

    // copy pixel data "from image" to "staging buffer"
    void *data = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(data, pixels, imageSize);
    stagingBufferMemory.unmapMemory();
    // free image memory loaded by stb_image
    stbi_image_free(pixels);

    // better to access pixel value(now in the staging buffer) by using image object(Pixels within an image object are known as texels)

    createImage(
        texWidth,
        texHeight,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        textureImage,
        textureImageMemory);

    //  The last thing we did there was creating the texture image object, but it is still empty!
    // Now we need to copy the pixel data from the staging buffer to the image object.
    transitionImageLayout(textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}
/**
 * @brief transition image layout helper function
 *
 * @param image
 * @param oldLayout
 * @param newLayout
 */
void HelloTriangleApplication::transitionImageLayout(const vk::raii::Image &image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    auto commandBuffer = beginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier{
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }
    commandBuffer->pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
    endSingleTimeCommands(*commandBuffer);
}

/**
 * @brief Copy data from a buffer to an image
 *
 * @param buffer
 * @param image
 * @param width
 * @param height
 */
void HelloTriangleApplication::copyBufferToImage(const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height)
{

    /*before we can use the image as a texture in shader, we need to copy the pixel data from the staging buffer to the image object*/
    std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = beginSingleTimeCommands();

    vk::BufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1}};

    commandBuffer->copyBufferToImage(
        buffer,
        image,
        vk::ImageLayout::eTransferDstOptimal,
        {region});

    endSingleTimeCommands(*commandBuffer);
}

/**
 * @brief create texture image view
 * call createImageView helper function
 */
void HelloTriangleApplication::createTextureImageView()
{
    textureImageView = createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
}

/**
 * @brief create texture sampler
 *  setup sampler to read from the texture image in the shader
 */
void HelloTriangleApplication::createTextureSampler()
{
    /*
    setup sampler to read from the texture image in the shader
    1. get device properties to check max sampler anisotropy
    2. create sampler info

    */

    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    vk::SamplerCreateInfo samplerInfo{.magFilter = vk::Filter::eLinear,
                                      .minFilter = vk::Filter::eLinear,
                                      .mipmapMode = vk::SamplerMipmapMode::eLinear,
                                      .addressModeU = vk::SamplerAddressMode::eRepeat,
                                      .addressModeV = vk::SamplerAddressMode::eRepeat,
                                      .addressModeW = vk::SamplerAddressMode::eRepeat,
                                      .anisotropyEnable = vk::True,
                                      .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
                                      .compareEnable = vk::False,
                                      .compareOp = vk::CompareOp::eAlways};
    textureSampler = vk::raii::Sampler(device, samplerInfo);
}