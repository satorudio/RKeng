#include "VulkanPipelineCreate.h"
#include "VulkanShaderLoad.h"
#include "../utils/Logger.h"
#include <array>
#include <stdexcept>

namespace RKeng::VulkanPipelineCreate
{
    void Run(VulkanState& vk)
    {
        VkShaderModule vertModule = VulkanShaderLoad::Run(vk, "shaders/triangle.vert.spv");
        VkShaderModule fragModule = VulkanShaderLoad::Run(vk, "shaders/triangle.frag.spv");

        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertModule;
        vertStage.pName  = "main";

        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragModule;
        fragStage.pName  = "main";

        std::array<VkPipelineShaderStageCreateInfo, 2> stages = { vertStage, fragStage };

        // Vertex input: pos(3) + normal(3) + color(3) = 9 floats = 36 bytes
        VkVertexInputBindingDescription binding{};
        binding.binding   = 0;
        binding.stride    = sizeof(float) * 9;
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 3> attrs{};
        attrs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
        attrs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float)*3};
        attrs[2] = {2, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float)*6};

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount   = 1;
        vertexInput.pVertexBindingDescriptions      = &binding;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
        vertexInput.pVertexAttributeDescriptions    = attrs.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount  = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode    = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.lineWidth   = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments    = &colorBlendAttachment;

        std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates    = dynamicStates.data();

        VkPushConstantRange pcr{};
        pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pcr.offset     = 0;
        pcr.size       = sizeof(float) * 16;  // mat4

        VkPipelineLayoutCreateInfo layoutCI{};
        layoutCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCI.setLayoutCount         = 1;
        layoutCI.pSetLayouts            = &vk.descSetLayout;
        layoutCI.pushConstantRangeCount = 1;
        layoutCI.pPushConstantRanges    = &pcr;
        if (vkCreatePipelineLayout(vk.device, &layoutCI, nullptr, &vk.pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout.");

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable       = VK_TRUE;
        depthStencil.depthWriteEnable      = VK_TRUE;
        depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable     = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.stageCount          = static_cast<uint32_t>(stages.size());
        pipelineCI.pStages             = stages.data();
        pipelineCI.pVertexInputState   = &vertexInput;
        pipelineCI.pInputAssemblyState = &inputAssembly;
        pipelineCI.pViewportState      = &viewportState;
        pipelineCI.pRasterizationState = &rasterizer;
        pipelineCI.pMultisampleState   = &multisampling;
        pipelineCI.pDepthStencilState  = &depthStencil;
        pipelineCI.pColorBlendState    = &colorBlending;
        pipelineCI.pDynamicState       = &dynamicState;
        pipelineCI.layout              = vk.pipelineLayout;
        pipelineCI.renderPass          = vk.renderPass;
        pipelineCI.subpass             = 0;

        if (vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1,
                                       &pipelineCI, nullptr, &vk.pipeline) != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline.");

        vkDestroyShaderModule(vk.device, vertModule, nullptr);
        vkDestroyShaderModule(vk.device, fragModule, nullptr);
        Logger::Info("Graphics pipeline created.");
    }
}
