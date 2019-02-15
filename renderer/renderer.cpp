#include "renderer.h"
#include "../vulkan/shaders.h"
#include <cassert>
#include <stdio.h>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Renderer
{

Renderer::Renderer(Vulkan::Device* device) : device(device)
{
}

void Renderer::init()
{
	create_graphics_pipeline();

	meshes_count = 2;
	meshes = new Mesh[meshes_count];

	{
		Vertex vertices[4];
		vertices[0] = { {-0.5f, -0.5f, 0.5f,}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} };
		vertices[1] = { {0.5f, -0.5f, 0.5f},   {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} };
		vertices[2] = { {0.5f, 0.5f, 0.5f},    {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} };
		vertices[3] = { {-0.5f, 0.5f, 0.5f},   {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} };

		uint16_t indices[6];
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
		indices[3] = 2;
		indices[4] = 3;
		indices[5] = 0;

		VkDeviceSize vertices_size = sizeof(vertices[0]) * 4;

		Vulkan::Buffer* vertex_staging_buffer = new Vulkan::Buffer(
			device->context->device,
			device->context->gpu,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			vertices_size);

		void* vertex_data;
		vkMapMemory(device->context->device, vertex_staging_buffer->device_memory, 0, vertex_staging_buffer->size, 0, &vertex_data);
		memcpy(vertex_data, vertices, (size_t)vertex_staging_buffer->size);
		vkUnmapMemory(device->context->device, vertex_staging_buffer->device_memory);

		VkDeviceSize indices_size = sizeof(indices[0]) * 6;

		Vulkan::Buffer* index_staging_buffer = new Vulkan::Buffer(
			device->context->device,
			device->context->gpu,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			indices_size);

		void* index_data;
		vkMapMemory(device->context->device, index_staging_buffer->device_memory, 0, index_staging_buffer->size, 0, &index_data);
		memcpy(index_data, indices, (size_t)index_staging_buffer->size);
		vkUnmapMemory(device->context->device, index_staging_buffer->device_memory);

		uint32_t vertex_offset = device->upload_vertex_buffer(vertex_staging_buffer);
		uint32_t index_offset = device->upload_index_buffer(index_staging_buffer);

		meshes[0].index_offset = index_offset;
		meshes[0].index_count = 6;
		meshes[0].vertex_offset = vertex_offset;

		vertex_staging_buffer->destroy(device->context->device);
		index_staging_buffer->destroy(device->context->device);
	}

	{
		Vertex vertices[4];
		vertices[0] = { {-0.5f, -0.5f, 0.0f,}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };
		vertices[1] = { {0.5f, -0.5f, 0.0f},   {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };
		vertices[2] = { {0.5f, 0.5f, 0.0f},    {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };
		vertices[3] = { {-0.5f, 0.5f, 0.0f},   {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };

		uint16_t indices[6];
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
		indices[3] = 2;
		indices[4] = 3;
		indices[5] = 0;

		VkDeviceSize vertices_size = sizeof(vertices[0]) * 4;

		Vulkan::Buffer* vertex_staging_buffer = new Vulkan::Buffer(
			device->context->device,
			device->context->gpu,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			vertices_size);

		void* vertex_data;
		vkMapMemory(device->context->device, vertex_staging_buffer->device_memory, 0, vertex_staging_buffer->size, 0, &vertex_data);
		memcpy(vertex_data, vertices, (size_t)vertex_staging_buffer->size);
		vkUnmapMemory(device->context->device, vertex_staging_buffer->device_memory);

		VkDeviceSize indices_size = sizeof(indices[0]) * 6;

		Vulkan::Buffer* index_staging_buffer = new Vulkan::Buffer(
			device->context->device,
			device->context->gpu,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			indices_size);

		void* index_data;
		vkMapMemory(device->context->device, index_staging_buffer->device_memory, 0, index_staging_buffer->size, 0, &index_data);
		memcpy(index_data, indices, (size_t)index_staging_buffer->size);
		vkUnmapMemory(device->context->device, index_staging_buffer->device_memory);

		uint32_t vertex_offset = device->upload_vertex_buffer(vertex_staging_buffer);
		uint32_t index_offset = device->upload_index_buffer(index_staging_buffer);

		meshes[1].index_offset = index_offset / sizeof(uint16_t);
		meshes[1].index_count = 6;
		meshes[1].vertex_offset = vertex_offset / sizeof(Vertex);

		vertex_staging_buffer->destroy(device->context->device);
		index_staging_buffer->destroy(device->context->device);
	}
}

void Renderer::render_frame()
{
	auto frame_cpu_start = std::chrono::high_resolution_clock::now();

	Vulkan::FrameResources& frame_resources = device->begin_draw_frame();

	if (frame_resources.descriptor_set == VK_NULL_HANDLE)
	{
		VkResult result = device->allocate_descriptor_set(descriptor_set_layout, frame_resources.descriptor_set);
		assert(result == VK_SUCCESS);

		VkDescriptorBufferInfo buffer_info = {};
		buffer_info.buffer = frame_resources.ubo_buffer->buffer;
		buffer_info.offset = 0;
		buffer_info.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet descriptor_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		descriptor_write.dstSet = frame_resources.descriptor_set;
		descriptor_write.dstBinding = 0;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pBufferInfo = &buffer_info;

		vkUpdateDescriptorSets(device->context->device, 1, &descriptor_write, 0, nullptr);
	}

	vkCmdBindPipeline(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

	VkViewport viewport = { 0, 0, (float)device->wsi->swapchain_extent.width, (float)device->wsi->swapchain_extent.height, 0.0f, 1.0f };
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = device->wsi->swapchain_extent;

	vkCmdSetViewport(frame_resources.command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(frame_resources.command_buffer, 0, 1, &scissor);

	update_uniform_buffer(frame_resources);
	vkCmdBindDescriptorSets(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &frame_resources.descriptor_set, 0, nullptr);

	VkBuffer vertex_buffers[] = { device->vertex_buffer->buffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(frame_resources.command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(frame_resources.command_buffer, device->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT16);

	for (uint32_t i = 0; i < meshes_count; ++i)
	{
		vkCmdDrawIndexed(frame_resources.command_buffer, meshes[i].index_count, 1, meshes[i].index_offset, meshes[i].vertex_offset, 0);
	}

	device->end_draw_frame(frame_resources);

	auto frame_cpu_end = std::chrono::high_resolution_clock::now();
	frame_cpu_avg = frame_cpu_avg * 0.95 + (frame_cpu_end - frame_cpu_start).count() * 1e-6 * 0.05;
}

void Renderer::update_uniform_buffer(Vulkan::FrameResources& frame_resources)
{
	static auto start_time = std::chrono::high_resolution_clock::now();
	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.projection = glm::perspective(glm::radians(45.0f), (float)device->wsi->swapchain_extent.width / (float)device->wsi->swapchain_extent.height, 0.1f, 10.0f);
	ubo.projection[1][1] *= -1;

	void* data;
	vkMapMemory(device->context->device, frame_resources.ubo_buffer->device_memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device->context->device, frame_resources.ubo_buffer->device_memory);
}

void Renderer::cleanup()
{
	vkQueueWaitIdle(device->context->graphics_queue);

	if (descriptor_set_layout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(device->context->device, descriptor_set_layout, nullptr);
	}

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
	shader_stages[0] = Vulkan::Shader::load_shader(device->context->device, device->context->gpu, "../data/shaders/simple.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = Vulkan::Shader::load_shader(device->context->device, device->context->gpu, "../data/shaders/simple.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
	// originating from a binding description. We have 3 attributes, position, normal and color,
	// so we need 3 attribute description structs
	VkVertexInputAttributeDescription vertex_input_attribute_descriptions[3];
	// Position
	vertex_input_attribute_descriptions[0].binding = 0;
	vertex_input_attribute_descriptions[0].location = 0;
	vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attribute_descriptions[0].offset = offsetof(Vertex, position);
	// Normal
	vertex_input_attribute_descriptions[1].binding = 0;
	vertex_input_attribute_descriptions[1].location = 1;
	vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attribute_descriptions[1].offset = offsetof(Vertex, normal);
	// Color
	vertex_input_attribute_descriptions[2].binding = 0;
	vertex_input_attribute_descriptions[2].location = 2;
	vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attribute_descriptions[2].offset = offsetof(Vertex, color);

	vertex_input_ci.vertexAttributeDescriptionCount = ARRAYSIZE(vertex_input_attribute_descriptions);
	vertex_input_ci.pVertexAttributeDescriptions = vertex_input_attribute_descriptions;

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_ci.primitiveRestartEnable = VK_FALSE;

	// Uniform Buffers
	VkDescriptorSetLayoutBinding ubo_layout_binding = {};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo descriptor_layout_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptor_layout_ci.bindingCount = 1;
	descriptor_layout_ci.pBindings = &ubo_layout_binding;

	VkResult result = vkCreateDescriptorSetLayout(device->context->device, &descriptor_layout_ci, nullptr, &descriptor_set_layout);
	assert(result == VK_SUCCESS);

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
	// NOTE: We had to change this from CLOCKWISE to COUNTER_CLOCKWISE after the introduction
	// of the projection matrix. Since we have to flip the Y-axis (ie multiply by -1) the vertices
	// are drawn
	rasterizer_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
	pipeline_layout_ci.setLayoutCount = 1;
	pipeline_layout_ci.pSetLayouts = &descriptor_set_layout;

	result = vkCreatePipelineLayout(device->context->device, &pipeline_layout_ci, nullptr, &pipeline_layout);
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
