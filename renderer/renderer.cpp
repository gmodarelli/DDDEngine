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
	create_descriptor_pool();
	create_ubo_buffers();
	create_graphics_pipeline();

	// Static objects
	static_transforms = new Transform[static_transform_count];
	static_entity_count = 2;
	static_entitites = new StaticEntity[static_entity_count];

	// Meshes
	meshes_count = 2;
	meshes = new Mesh[meshes_count];
	uint32_t mesh_id = 0;

	// NOTE: A cube mesh
	// We use this mesh to draw obstacles in the level.
	// Since obstacles don't move, we render them as static instances
	Vertex cube_vertices[] = {
		// front
		{ { -1.0f, -1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, {0.3f, 0.3f, 0.3f} },
		{ { 1.0f, -1.0f, 1.0f, }, { 0.0f, 0.0f, 0.0f }, {0.3f, 0.3f, 0.3f} },
		{ {	1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, {0.3f, 0.3f, 0.3f} },
		{ {	-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f} },
		// back
		{ {	-1.0, -1.0, -1.0 }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f} },
		{ {	 1.0, -1.0, -1.0 }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f} },
		{ { 1.0,  1.0, -1.0 }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f} },
		{ {	-1.0,  1.0, -1.0 }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f} }
	};

	uint16_t cube_indices[] = {
		// front
		0, 1, 2,
		2, 3, 0,
		// right
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// left
		4, 0, 3,
		3, 7, 4,
		// bottom
		4, 5, 1,
		1, 0, 4,
		// top
		3, 2, 6,
		6, 7, 3
	};

	VkDeviceSize vertices_size = sizeof(cube_vertices[0]) * ARRAYSIZE(cube_vertices);

	Vulkan::Buffer* vertex_staging_buffer = new Vulkan::Buffer(
		device->context->device,
		device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		vertices_size);

	void* cube_vertex_data;
	vkMapMemory(device->context->device, vertex_staging_buffer->device_memory, 0, vertex_staging_buffer->size, 0, &cube_vertex_data);
	memcpy(cube_vertex_data, cube_vertices, (size_t)vertex_staging_buffer->size);
	vkUnmapMemory(device->context->device, vertex_staging_buffer->device_memory);

	VkDeviceSize indices_size = sizeof(cube_indices[0]) * ARRAYSIZE(cube_indices);

	Vulkan::Buffer* index_staging_buffer = new Vulkan::Buffer(
		device->context->device,
		device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		indices_size);

	void* cube_index_data;
	vkMapMemory(device->context->device, index_staging_buffer->device_memory, 0, index_staging_buffer->size, 0, &cube_index_data);
	memcpy(cube_index_data, cube_indices, (size_t)index_staging_buffer->size);
	vkUnmapMemory(device->context->device, index_staging_buffer->device_memory);

	uint32_t vertex_offset = (uint32_t)device->upload_vertex_buffer(vertex_staging_buffer);
	uint32_t index_offset = (uint32_t)device->upload_index_buffer(index_staging_buffer);

	meshes[mesh_id].index_offset = index_offset / sizeof(uint16_t);
	meshes[mesh_id].index_count = ARRAYSIZE(cube_indices);
	meshes[mesh_id].vertex_offset = vertex_offset / sizeof(Vertex);

	vertex_staging_buffer->destroy(device->context->device);
	index_staging_buffer->destroy(device->context->device);

	mesh_id++;

	// NOTE: A plane mesh
	Vertex plane_vertices[] = {
		{ { -1.0f, -1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.36f, 0.03f } },
		{ { 1.0f, -1.0f, 1.0f, }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.36f, 0.03f } },
		{ {	1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.36f, 0.03f } },
		{ {	-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.36f, 0.03f } },
	};

	uint16_t plane_indices[] = {
		0, 1, 2,
		2, 3, 0,
	};

	vertices_size = sizeof(plane_vertices[0]) * ARRAYSIZE(plane_vertices);

	vertex_staging_buffer = new Vulkan::Buffer(
		device->context->device,
		device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		vertices_size);

	void* plane_vertex_data;
	vkMapMemory(device->context->device, vertex_staging_buffer->device_memory, 0, vertex_staging_buffer->size, 0, &plane_vertex_data);
	memcpy(plane_vertex_data, plane_vertices, (size_t)vertex_staging_buffer->size);
	vkUnmapMemory(device->context->device, vertex_staging_buffer->device_memory);

	indices_size = sizeof(plane_indices[0]) * ARRAYSIZE(plane_indices);

	index_staging_buffer = new Vulkan::Buffer(
		device->context->device,
		device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		indices_size);

	void* plane_index_data;
	vkMapMemory(device->context->device, index_staging_buffer->device_memory, 0, index_staging_buffer->size, 0, &plane_index_data);
	memcpy(plane_index_data, plane_indices, (size_t)index_staging_buffer->size);
	vkUnmapMemory(device->context->device, index_staging_buffer->device_memory);

	vertex_offset = (uint32_t)device->upload_vertex_buffer(vertex_staging_buffer);
	index_offset = (uint32_t)device->upload_index_buffer(index_staging_buffer);

	meshes[mesh_id].index_offset = index_offset / sizeof(uint16_t);
	meshes[mesh_id].index_count = ARRAYSIZE(plane_indices);
	meshes[mesh_id].vertex_offset = vertex_offset / sizeof(Vertex);

	vertex_staging_buffer->destroy(device->context->device);
	index_staging_buffer->destroy(device->context->device);

	mesh_id++;

	// Static Entitites

	// A makeshift board for the game
	// Build a surrounding wall
	glm::vec3 scale(0.2f, 0.2f, 0.2f);
	float distance = 0.4f;
	float offset_x = -9.0f * distance;
	float offset_z = -11.0f * distance;

	uint32_t board_width = 20;
	uint32_t board_height = 20;

	// Generate the transforms for the walls
	uint32_t transform_index = 0;
	for (uint32_t c = 0; c < board_height; ++c)
	{
		for (uint32_t r = 0; r < board_width; ++r)
		{
			if (c == 0 || c == board_height - 1)
			{
				static_transforms[transform_index++] = { glm::vec3(r * distance + offset_x, 0.0f, c * distance + offset_z), scale };
			}
			else if (r == 0 || r == board_width - 1)
			{
				static_transforms[transform_index++] = { glm::vec3(r * distance + offset_x, 0.0f, c * distance + offset_z), scale };
			}
		}
	}

	// Upload the transforms for the walls
	VkDeviceSize instances_size = sizeof(static_transforms[0]) * transform_index;

	Vulkan::Buffer* instance_staging_buffer = new Vulkan::Buffer(
		device->context->device,
		device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		instances_size);

	void* wall_instance_data;
	vkMapMemory(device->context->device, instance_staging_buffer->device_memory, 0, instance_staging_buffer->size, 0, &wall_instance_data);
	memcpy(wall_instance_data, static_transforms, (size_t)instance_staging_buffer->size);
	vkUnmapMemory(device->context->device, instance_staging_buffer->device_memory);

	static_entitites[0] = {};
	static_entitites[0].count = transform_index;
	static_entitites[0].mesh_id = 0;
	static_entitites[0].transform_offset = (uint32_t)device->upload_instance_buffer(instance_staging_buffer);

	instance_staging_buffer->destroy(device->context->device);

	// Generate the transforms for the ground
	static_transforms[transform_index] = { glm::vec3(distance * 0.5f, -1.0f, distance * -1.5f), glm::vec3(0.2 * board_width, 1.0f, 0.2 * board_height) , glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)) };

	// Upload the transforms for the grounnd
	instances_size = sizeof(static_transforms[0]) * 1;

	instance_staging_buffer = new Vulkan::Buffer(
		device->context->device,
		device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		instances_size);

	void* ground_instance_data;
	vkMapMemory(device->context->device, instance_staging_buffer->device_memory, 0, instance_staging_buffer->size, 0, &ground_instance_data);
	memcpy(ground_instance_data, &static_transforms[transform_index], (size_t)instance_staging_buffer->size);
	vkUnmapMemory(device->context->device, instance_staging_buffer->device_memory);

	static_entitites[1] = {};
	static_entitites[1].count = transform_index;
	static_entitites[1].mesh_id = 1;
	static_entitites[1].transform_offset = (uint32_t)device->upload_instance_buffer(instance_staging_buffer);

	instance_staging_buffer->destroy(device->context->device);
	delete instance_staging_buffer;
}

void Renderer::render_frame()
{
	auto frame_cpu_start = std::chrono::high_resolution_clock::now();

	Vulkan::FrameResources& frame_resources = device->begin_draw_frame();
	Frame* frame = (Frame*) frame_resources.custom;

	if (frame == nullptr)
	{
		frame = &frames[device->frame_index];
		frame_resources.custom = frame;
	}

	if (frame->descriptor_set == VK_NULL_HANDLE)
	{
		VkResult result = allocate_descriptor_set(descriptor_set_layout, frame->descriptor_set);
		assert(result == VK_SUCCESS);

		VkDescriptorBufferInfo buffer_info = {};
		buffer_info.buffer = frame->ubo_buffer->buffer;
		buffer_info.offset = 0;
		buffer_info.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet descriptor_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		descriptor_write.dstSet = frame->descriptor_set;
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

	vkCmdBindDescriptorSets(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &frame->descriptor_set, 0, nullptr);

	VkBuffer vertex_buffers[] = { device->vertex_buffer->buffer };
	VkBuffer instance_buffers[] = { device->instance_buffer->buffer };
	VkDeviceSize offsets[] = { 0 };

	// Bind point 0: Mesh vertex buffer
	vkCmdBindVertexBuffers(frame_resources.command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(frame_resources.command_buffer, device->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT16);

	for (uint32_t i = 0; i < static_entity_count; ++i)
	{
		VkDeviceSize offsets[] = { static_entitites[i].transform_offset };
		// Bind point 1: Instance data buffer
		vkCmdBindVertexBuffers(frame_resources.command_buffer, 1, 1, instance_buffers, offsets);
		// Draw all instances at once
		Mesh& mesh = meshes[static_entitites[i].mesh_id];
		vkCmdDrawIndexed(frame_resources.command_buffer, mesh.index_count, static_entitites[i].count, mesh.index_offset, mesh.vertex_offset, 0);
	}

	device->end_draw_frame(frame_resources);

	auto frame_cpu_end = std::chrono::high_resolution_clock::now();
	frame_cpu_avg = frame_cpu_avg * 0.95 + (frame_cpu_end - frame_cpu_start).count() * 1e-6 * 0.05;
}

VkResult Renderer::allocate_descriptor_set(const VkDescriptorSetLayout& descriptor_set_layout, VkDescriptorSet& descriptor_set)
{
	VkDescriptorSetAllocateInfo descriptor_set_ai = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptor_set_ai.descriptorPool = descriptor_pool;
	descriptor_set_ai.descriptorSetCount = 1;
	descriptor_set_ai.pSetLayouts = &descriptor_set_layout;

	return vkAllocateDescriptorSets(device->context->device, &descriptor_set_ai, &descriptor_set);
}

void Renderer::update_uniform_buffer(Vulkan::FrameResources& frame_resources)
{
	Frame* frame = (Frame*) frame_resources.custom;
	assert(frame != nullptr);

	static auto start_time = std::chrono::high_resolution_clock::now();
	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

	glm::vec3 camera_position = { 0.0f, 10.0f, 5.0f };

	UniformBufferObject ubo = {};
	// ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f) * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
	ubo.model = glm::mat4(1.0f);
	ubo.view = glm::lookAt(camera_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.projection = glm::perspective(glm::radians(45.0f), (float)device->wsi->swapchain_extent.width / (float)device->wsi->swapchain_extent.height, 0.001f, 100.0f);
	ubo.projection[1][1] *= -1;

	void* data;
	vkMapMemory(device->context->device, frame->ubo_buffer->device_memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device->context->device, frame->ubo_buffer->device_memory);
}

void Renderer::cleanup()
{
	vkQueueWaitIdle(device->context->graphics_queue);
	destroy_ubo_buffers();
	destroy_descriptor_pool();

	vkDestroyDescriptorSetLayout(device->context->device, descriptor_set_layout, nullptr);
	descriptor_set_layout = VK_NULL_HANDLE;

	vkDestroyPipeline(device->context->device, graphics_pipeline, nullptr);
	graphics_pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(device->context->device, pipeline_layout, nullptr);
	pipeline_layout = VK_NULL_HANDLE;
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
	VkVertexInputBindingDescription vertex_binding_descriptions[2];
	// Mesh vertex bindings
	vertex_binding_descriptions[0] = {};
	vertex_binding_descriptions[0].binding = 0;
	vertex_binding_descriptions[0].stride = sizeof(Vertex);
	vertex_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	// Instance transform bindings (per instance position, scale and rotation)
	vertex_binding_descriptions[1] = {};
	vertex_binding_descriptions[1].binding = 1;
	vertex_binding_descriptions[1].stride = sizeof(Transform);
	vertex_binding_descriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

	vertex_input_ci.vertexBindingDescriptionCount = ARRAYSIZE(vertex_binding_descriptions);
	vertex_input_ci.pVertexBindingDescriptions = vertex_binding_descriptions;

	// An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data 
	// originating from a binding description. We have 3 attributes, position, normal and color,
	// so we need 3 attribute description structs
	// We also have 1 additional attribute, instance position
	VkVertexInputAttributeDescription vertex_input_attribute_descriptions[6];
	// Per-vertex attributes
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

	// Per-instance attributes
	// Instance position
	vertex_input_attribute_descriptions[3].binding = 1;
	vertex_input_attribute_descriptions[3].location = 3;
	vertex_input_attribute_descriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attribute_descriptions[3].offset = offsetof(Transform, position);
	// Instance scale
	vertex_input_attribute_descriptions[4].binding = 1;
	vertex_input_attribute_descriptions[4].location = 4;
	vertex_input_attribute_descriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attribute_descriptions[4].offset = offsetof(Transform, scale);
	// Instance rotation
	vertex_input_attribute_descriptions[5].binding = 1;
	vertex_input_attribute_descriptions[5].location = 5;
	vertex_input_attribute_descriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertex_input_attribute_descriptions[5].offset = offsetof(Transform, rotation);

	vertex_input_ci.vertexAttributeDescriptionCount = ARRAYSIZE(vertex_input_attribute_descriptions);
	vertex_input_ci.pVertexAttributeDescriptions = vertex_input_attribute_descriptions;

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_ci.primitiveRestartEnable = VK_FALSE;

	// View Uniform Buffer
	VkDescriptorSetLayoutBinding view_ubo_layout_binding = {};
	view_ubo_layout_binding.binding = 0;
	view_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	view_ubo_layout_binding.descriptorCount = 1;
	view_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo descriptor_layout_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptor_layout_ci.bindingCount = 1;
	descriptor_layout_ci.pBindings = &view_ubo_layout_binding;

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

	// Depth Buffer
	VkPipelineDepthStencilStateCreateInfo depth_stencil_ci = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depth_stencil_ci.depthTestEnable = VK_TRUE;
	depth_stencil_ci.depthWriteEnable = VK_TRUE;
	depth_stencil_ci.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil_ci.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_ci.stencilTestEnable = VK_FALSE;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling_ci = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling_ci.rasterizationSamples = device->context->msaa_samples;
	if (device->context->gpu_enabled_features.sampleRateShading == VK_TRUE)
	{
		multisampling_ci.sampleShadingEnable = VK_TRUE;
		// min fraction for sample shading; closer to one is smooth
		multisampling_ci.minSampleShading = 0.2f;
	}
	else
	{
		multisampling_ci.sampleShadingEnable = VK_FALSE;
	}
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
	VkDescriptorSetLayout descriptor_set_layouts[] = { descriptor_set_layout };
	VkPipelineLayoutCreateInfo pipeline_layout_ci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipeline_layout_ci.setLayoutCount = ARRAYSIZE(descriptor_set_layouts);
	pipeline_layout_ci.pSetLayouts = descriptor_set_layouts;

	result = vkCreatePipelineLayout(device->context->device, &pipeline_layout_ci, nullptr, &pipeline_layout);
	assert(result == VK_SUCCESS);

	VkGraphicsPipelineCreateInfo pipeline_ci = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipeline_ci.stageCount = ARRAYSIZE(shader_stages);
	pipeline_ci.pStages = shader_stages;
	pipeline_ci.pVertexInputState = &vertex_input_ci;
	pipeline_ci.pInputAssemblyState = &input_assembly_ci;
	pipeline_ci.pViewportState = &viewport_ci;
	pipeline_ci.pRasterizationState = &rasterizer_ci;
	pipeline_ci.pDepthStencilState = &depth_stencil_ci;
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

// Descriptor Pool helpers
void Renderer::create_descriptor_pool()
{
	VkDescriptorPoolSize pool_size = {};
	pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_size.descriptorCount = 2 * Vulkan::MAX_FRAMES_IN_FLIGHT;

	VkDescriptorPoolCreateInfo pool_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_ci.poolSizeCount = 1;
	pool_ci.pPoolSizes = &pool_size;
	pool_ci.maxSets = 2 * Vulkan::MAX_FRAMES_IN_FLIGHT;

	VkResult result = vkCreateDescriptorPool(device->context->device, &pool_ci, nullptr, &descriptor_pool);
	assert(result == VK_SUCCESS);
}

void Renderer::destroy_descriptor_pool()
{
	if (descriptor_pool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(device->context->device, descriptor_pool, nullptr);
		descriptor_pool = VK_NULL_HANDLE;
	}
}

// UBO Buffers Helpers
void Renderer::create_ubo_buffers()
{
	for (uint32_t i = 0; i < Vulkan::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		frames[i].ubo_buffer = new Vulkan::Buffer(
			device->context->device,
			device->context->gpu,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(UniformBufferObject));
	}
}

void Renderer::destroy_ubo_buffers()
{

	for (uint32_t i = 0; i < Vulkan::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		frames[i].ubo_buffer->destroy(device->context->device);
	}
}

} // namespace Renderer
