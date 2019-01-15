#ifndef VULKAN_PIPELINE_H_
#define VULKAN_PIPELINE_H_

#include "vkr/scene.h"

void SetupPipeline(VkDevice device, uint32_t width, uint32_t height, std::vector<VkDescriptorSetLayout> descriptorSetLayouts, VkShaderModule vertShaderModule, VkShaderModule fragShaderModule, VkRenderPass renderPass, Pipeline* outPipeline)
{
	// Graphics Pipeline
	VkPipelineLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	layoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
	layoutCreateInfo.pushConstantRangeCount = 0;
	layoutCreateInfo.pPushConstantRanges = nullptr;

	VK_CHECK(vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &outPipeline->PipelineLayout));

	// Setup the shader stages
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] = {};

	shaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageCreateInfo[0].module = vertShaderModule;
	shaderStageCreateInfo[0].pName = "main"; // Shader entry point
	shaderStageCreateInfo[0].pSpecializationInfo = nullptr;

	shaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageCreateInfo[1].module = fragShaderModule;
	shaderStageCreateInfo[1].pName = "main"; // Shader entry point
	shaderStageCreateInfo[1].pSpecializationInfo = nullptr;

	// TODO: Temporary, legacy FFA IA
	// Vertex input configuration
	VkVertexInputBindingDescription vertexBindingDescription = {};
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.stride = sizeof(vkr::Vertex);
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertexAttributeDescription[4];
	// Position
	vertexAttributeDescription[0].location = 0;
	vertexAttributeDescription[0].binding = 0;
	vertexAttributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescription[0].offset = 0;

	// Normal
	vertexAttributeDescription[1].location = 1;
	vertexAttributeDescription[1].binding = 0;
	vertexAttributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescription[1].offset = sizeof(glm::vec3);

	// UV
	vertexAttributeDescription[2].location = 2;
	vertexAttributeDescription[2].binding = 0;
	vertexAttributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescription[2].offset = sizeof(glm::vec3) * 2;

	// Color
	vertexAttributeDescription[3].location = 3;
	vertexAttributeDescription[3].binding = 0;
	vertexAttributeDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescription[3].offset = sizeof(glm::vec3) * 3;

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = ARRAYSIZE(vertexAttributeDescription);
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescription;

	// Vertex Topology Configuration
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	// Viewport Configuration
	VkViewport viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	VkRect2D scissors = { {0, 0}, { width, height} };

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissors;

	// Rasterization Configuration
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0;
	rasterizationStateCreateInfo.lineWidth = 1;

	// Sampling Configuration
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.minSampleShading = 0;
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	// Color Blend Configuration
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.blendEnable = VK_FALSE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.colorWriteMask = 0xf;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_CLEAR;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0;

	// Configure Dynamic States
	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStateCreateInfo.dynamicStateCount = 2;
	dynamicStateCreateInfo.pDynamicStates = dynamicStates;

	// Configure the Depth and Stencil Buffer
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

	// Create the pipeline
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStageCreateInfo;
	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.layout = outPipeline->PipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = 0;

	VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &outPipeline->Pipeline))
}

void DestroyPipeline(VkDevice device, Pipeline* pipeline)
{
	vkDestroyPipeline(device, pipeline->Pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipeline->PipelineLayout, nullptr);
	pipeline->Pipeline = VK_NULL_HANDLE;
	pipeline->PipelineLayout = VK_NULL_HANDLE;
}

#endif // VULKAN_PIPELINE_H_ 
