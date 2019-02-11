#include "renderer.h"
#include "../vulkan/shaders.h"
#include <cassert>
#include <stdio.h>

namespace Renderer
{

Renderer::Renderer(Vulkan::Device* device) : device(device)
{
}

void Renderer::init()
{
	create_graphics_pipeline();

	vertex_count = 3;
	vertices = new Vertex[vertex_count];
	vertices[0] = { {0.0f, -0.5f}, {1.0f, 1.0f, 0.0f} };
	vertices[1] = { {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f} };
	vertices[2] = { {-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f} };

	vertex_buffer = new Vulkan::Buffer(
		device->context->device,
		device->context->gpu,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		sizeof(vertices[0]) * vertex_count);

	void* data;
	vkMapMemory(device->context->device, vertex_buffer->device_memory, 0, vertex_buffer->size, 0, &data);
	memcpy(data, vertices, (size_t)vertex_buffer->size);
	vkUnmapMemory(device->context->device, vertex_buffer->device_memory);
}

void Renderer::render_frame()
{
	Vulkan::FrameResources& frame_resources = device->begin_draw_frame();

	vkCmdBindPipeline(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

	VkViewport viewport = { 0, 0, (float)device->wsi->swapchain_extent.width, (float)device->wsi->swapchain_extent.height, 0.0f, 1.0f };
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = device->wsi->swapchain_extent;

	vkCmdSetViewport(frame_resources.command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(frame_resources.command_buffer, 0, 1, &scissor);

	VkBuffer vertex_buffers[] = { vertex_buffer->buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(frame_resources.command_buffer, 0, 1, vertex_buffers, offsets);

	vkCmdDraw(frame_resources.command_buffer, vertex_count, 1, 0, 0);

	device->end_draw_frame(frame_resources);
}

void Renderer::cleanup()
{
	vkQueueWaitIdle(device->context->graphics_queue);

	vertex_buffer->destroy(device->context->device);

	if (graphics_pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device->context->device, graphics_pipeline, nullptr);
		graphics_pipeline = VK_NULL_HANDLE;
	}

	if (pipeline_layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(device->context->device, pipeline_layout, nullptr);
		pipeline_layout = VK_NULL_HANDLE;
	}
}

void Renderer::create_graphics_pipeline()
{
	VkPipelineShaderStageCreateInfo shader_stages[2];
	shader_stages[0] = Vulkan::Shader::load_shader(device->context->device, device->context->gpu, "../data/shaders/simple_triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = Vulkan::Shader::load_shader(device->context->device, device->context->gpu, "../data/shaders/simple_triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Pipeline Fixed Functions
	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertex_input_ci = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	// A vertex binding describes at which rate to load data from memory throughout the vertices. 
	// It specifies the number of bytes between data entries and whether to move to the next 
	// data entry after each vertex or after each instance.
	VkVertexInputBindingDescription vertex_binding_description = {};
	vertex_binding_description.binding = 0;
	vertex_binding_description.stride = sizeof(Vertex);
	vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertex_input_ci.vertexBindingDescriptionCount = 1;
	vertex_input_ci.pVertexBindingDescriptions = &vertex_binding_description;

	// An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data 
	// originating from a binding description. We have two attributes, position and color,
	// so we need two attribute description structs
	VkVertexInputAttributeDescription vertex_input_attribute_descriptions[2];
	// Position
	vertex_input_attribute_descriptions[0].binding = 0;
	vertex_input_attribute_descriptions[0].location = 0;
	vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_input_attribute_descriptions[0].offset = offsetof(Vertex, position);
	// Color
	vertex_input_attribute_descriptions[1].binding = 0;
	vertex_input_attribute_descriptions[1].location = 1;
	vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attribute_descriptions[1].offset = offsetof(Vertex, color);

	vertex_input_ci.vertexAttributeDescriptionCount = ARRAYSIZE(vertex_input_attribute_descriptions);
	vertex_input_ci.pVertexAttributeDescriptions = vertex_input_attribute_descriptions;

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_ci.primitiveRestartEnable = VK_FALSE;
	// Viewport and Scissor
	VkViewport viewport = { 0, 0, (float)device->wsi->swapchain_extent.width, (float)device->wsi->swapchain_extent.height, 0.0f, 1.0f };
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = device->wsi->swapchain_extent;
	VkPipelineViewportStateCreateInfo viewport_ci = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewport_ci.viewportCount = 1;
	viewport_ci.pViewports = &viewport;
	viewport_ci.scissorCount = 1;
	viewport_ci.pScissors = &scissor;
	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer_ci = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	// If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them 
	// as opposed to discarding them. This is useful in some special cases like shadow maps.
	// Using this requires enabling a GPU feature.
	rasterizer_ci.depthClampEnable = VK_FALSE;
	// If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage.
	// This basically disables any output to the framebuffer.
	rasterizer_ci.rasterizerDiscardEnable = VK_FALSE;
	rasterizer_ci.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer_ci.lineWidth = 1.0f;
	rasterizer_ci.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer_ci.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer_ci.depthBiasEnable = VK_FALSE;
	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling_ci = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling_ci.sampleShadingEnable = VK_FALSE;
	multisampling_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	// Color Blend
	VkPipelineColorBlendAttachmentState color_blend_attachment_ci = {};
	color_blend_attachment_ci.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment_ci.blendEnable = VK_FALSE;
	// NOTE: If we want to enable blend we have to set it up this way
	// color_blend_attachment_ci.blendEnable = VK_TRUE;
	// color_blend_attachment_ci.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	// color_blend_attachment_ci.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	// color_blend_attachment_ci.colorBlendOp = VK_BLEND_OP_ADD;
	// color_blend_attachment_ci.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	// color_blend_attachment_ci.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// color_blend_attachment_ci.alphaBlendOp = VK_BLEND_OP_ADD;
	// 
	VkPipelineColorBlendStateCreateInfo color_blend_ci = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	color_blend_ci.logicOpEnable = VK_FALSE;
	color_blend_ci.attachmentCount = 1;
	color_blend_ci.pAttachments = &color_blend_attachment_ci;
	// Dynamic States (state that can be changed without having to recreate the pipeline)
	VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH };
	VkPipelineDynamicStateCreateInfo dynamic_state_ci = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamic_state_ci.dynamicStateCount = ARRAYSIZE(dynamic_states);
	dynamic_state_ci.pDynamicStates = dynamic_states;
	// Pipeline Layout
	VkPipelineLayoutCreateInfo pipeline_layout_ci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	// NOTE: Fill the CI when we have actual uniform data to send to shaders
	VkResult result = vkCreatePipelineLayout(device->context->device, &pipeline_layout_ci, nullptr, &pipeline_layout);
	assert(result == VK_SUCCESS);

	VkGraphicsPipelineCreateInfo pipeline_ci = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipeline_ci.stageCount = ARRAYSIZE(shader_stages);
	pipeline_ci.pStages = shader_stages;
	pipeline_ci.pVertexInputState = &vertex_input_ci;
	pipeline_ci.pInputAssemblyState = &input_assembly_ci;
	pipeline_ci.pViewportState = &viewport_ci;
	pipeline_ci.pRasterizationState = &rasterizer_ci;
	pipeline_ci.pMultisampleState = &multisampling_ci;
	pipeline_ci.pColorBlendState = &color_blend_ci;
	pipeline_ci.pDynamicState = &dynamic_state_ci;
	pipeline_ci.layout = pipeline_layout;
	pipeline_ci.renderPass = device->render_pass;
	pipeline_ci.subpass = 0;

	result = vkCreateGraphicsPipelines(device->context->device, nullptr, 1, &pipeline_ci, nullptr, &graphics_pipeline);
	assert(result == VK_SUCCESS);

	for (uint32_t i = 0; i < ARRAYSIZE(shader_stages); ++i)
	{
		vkDestroyShaderModule(device->context->device, shader_stages[i].module, nullptr);
	}
}

} // namespace Renderer
