#include "tutorial.hpp"
/**
 * @brief Create the Graphics Pipeline object
 *
 */
void HelloTriangleApplication::createGraphicsPipeline() {
    vk::raii::ShaderModule shaderModule = createShaderModule(readFile("shaders/shader.spv"));
    // declare shader stages
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{.stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain"};
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{.stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"};
    // combine shader stages
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    // color formats for multiple attachments
    vk::Format colorFormats[] = {
        vk::Format::eR32G32B32A32Sfloat,  // 0: Swapchain Color
        vk::Format::eR32G32B32A32Sfloat,  // 1: Position
        vk::Format::eR32G32B32A32Sfloat   // 2: Normal
    };

    // get two vertex input descriptions from Vertex struct
    // then create vertex input state info
    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{.vertexBindingDescriptionCount   = 1,
                                                           .pVertexBindingDescriptions      = &bindingDescription,
                                                           .vertexAttributeDescriptionCount = attributeDescriptions.size(),
                                                           .pVertexAttributeDescriptions    = attributeDescriptions.data()};
    // Input assembly
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};
    // Viewport and scissor
    vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};
    // Rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer{.depthClampEnable        = vk::False,
                                                        .rasterizerDiscardEnable = vk::False,
                                                        .polygonMode             = vk::PolygonMode::eFill,
                                                        .cullMode                = vk::CullModeFlagBits::eNone,
                                                        .frontFace               = vk::FrontFace::eCounterClockwise,
                                                        .depthBiasEnable         = vk::False,
                                                        .depthBiasSlopeFactor    = 1.0f,
                                                        .lineWidth               = 1.0f};
    // Multisampling
    vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};
    // Depth and stencil testing
    vk::PipelineDepthStencilStateCreateInfo depthStencil{.depthTestEnable       = vk::True,
                                                         .depthWriteEnable      = vk::True,
                                                         .depthCompareOp        = vk::CompareOp::eLess,
                                                         .depthBoundsTestEnable = vk::False,
                                                         .stencilTestEnable     = vk::False};

    // Color blending for three attachments
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{.blendEnable    = vk::False,
                                                               .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                                                 vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    std::vector<vk::PipelineColorBlendAttachmentState> blendAttachments = {colorBlendAttachment, colorBlendAttachment, colorBlendAttachment};
    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 3, .pAttachments = blendAttachments.data()};

    // Dynamic state
    std::vector dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                                                    .pDynamicStates    = dynamicStates.data()};
    // add descriptorSetLayout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 1, .pSetLayouts = &*descriptorSetLayout, .pushConstantRangeCount = 0};
    // pipeline layout
    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
    // add depth format
    vk::Format depthFormat = findDepthFormat();
    // Pipeline Rendering Create Info
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .colorAttachmentCount = 3, .pColorAttachmentFormats = colorFormats, .depthAttachmentFormat = depthFormat};

    vk::GraphicsPipelineCreateInfo pipelineInfo{.pNext      = &pipelineRenderingCreateInfo,  // add pNext to link to Pipeline Rendering Create Info
                                                .stageCount = 2,                             // vertex and fragment shaders, so two stages
                                                .pStages    = shaderStages,
                                                .pVertexInputState   = &vertexInputInfo,
                                                .pInputAssemblyState = &inputAssembly,
                                                .pViewportState      = &viewportState,
                                                .pRasterizationState = &rasterizer,
                                                .pMultisampleState   = &multisampling,
                                                .pDepthStencilState  = &depthStencil,
                                                .pColorBlendState    = &colorBlending,
                                                .pDynamicState       = &dynamicState,
                                                .layout              = pipelineLayout,
                                                .renderPass          = nullptr};
    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}
void HelloTriangleApplication::createComputePipeline() {
    vk::raii::ShaderModule computeShaderModule = createShaderModule(readFile("shaders/restir.spv"));

    vk::PipelineShaderStageCreateInfo computeShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eCompute, .module = computeShaderModule, .pName = "main"};

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 1, .pSetLayouts = &*computeDescriptorSetLayout};
    computePipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    vk::ComputePipelineCreateInfo pipelineInfo{.stage = computeShaderStageInfo, .layout = computePipelineLayout};

    computePipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}
[[nodiscard]] vk::raii::ShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data())};
    vk::raii::ShaderModule shaderModule{device, createInfo};

    return shaderModule;
}
