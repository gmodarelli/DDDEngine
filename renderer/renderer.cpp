#include "renderer.h"
#include "../vulkan/shaders.h"
#include <cassert>
#include <stdio.h>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../application/stb_image.h"

namespace Renderer
{

Renderer::Renderer(Platform* platform) : platform(platform)
{
}

void Renderer::init()
{
	// Initialize the Backend
	backend = new Vulkan::Backend();
	backend->wsi = new Vulkan::WSI(platform);
	backend->wsi->init();

	backend->device = new Vulkan::Device(backend->wsi, backend->wsi->context);
	backend->device->init();

	create_descriptor_pool();
	create_ubo_buffers();
	prepare_uniform_buffers();
	create_pipelines();

	// Init frame resources
	for (uint32_t i = 0; i < ARRAYSIZE(frames); ++i)
	{
		for (uint32_t ds = 0; ds < ARRAYSIZE(frames[i].descriptor_sets); ++ds)
		{
			frames[i].descriptor_sets[ds] = VK_NULL_HANDLE;
		}

		frames[i].descriptor_set_count = 1;
		frames[i].descriptor_set_cursor = 1;

		VkDeviceSize size = 1 * 1024 * 1024;
		frames[i].debug_vertex_buffer = new Vulkan::Buffer(
			backend->device->context->device,
			backend->device->context->gpu,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			size);
	}

	prepare_debug_vertex_buffers();
	
	// NOTE: All the following code will be replaced with a scene loader.
	// The hardcoded meshes will be loaded from asset files

	// Static objects
	static_transforms = new Transform[static_transform_count];
	static_entity_count = 2;
	static_entitites = new StaticEntity[static_entity_count];

	// Dynamic objects
	dynamic_entity_count = 1;
	dynamic_entities = new Entity[dynamic_entity_count];

	// Meshes
	meshes_count = 2;
	meshes = new Mesh[meshes_count];
	uint32_t mesh_id = 0;

	// NOTE: A cube mesh
	// We use this mesh to draw obstacles in the level.
	// Since obstacles don't move, we render them as static instances
	Vertex cube_vertices[] = {
		// front
		{ { -0.3f, -0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, {0.3f, 0.3f, 0.3f}, {1.0f, 0.0f} },
		{ { 0.3f, -0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, {0.3f, 0.3f, 0.3f}, {0.0f, 0.0f} },
		{ {	0.3f, 0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, {0.3f, 0.3f, 0.3f}, {0.0f, 1.0f} },
		{ {	-0.3f, 0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f}, {1.0f, 1.0f} },
		// back
		{ {	-0.3f, -0.3f, -0.3f }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f}, {1.0f, 0.0f} },
		{ {	 0.3f, -0.3f, -0.3f }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f}, {0.0f, 0.0f} },
		{ { 0.3f,  0.3f, -0.3f }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f}, {0.0f, 1.0f} },
		{ {	-0.3f,  0.3f, -0.3f }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f}, {1.0f, 1.0f} }
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
		backend->device->context->device,
		backend->device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		vertices_size);

	void* cube_vertex_data;
	vkMapMemory(backend->device->context->device, vertex_staging_buffer->device_memory, 0, vertex_staging_buffer->size, 0, &cube_vertex_data);
	memcpy(cube_vertex_data, cube_vertices, (size_t)vertex_staging_buffer->size);
	vkUnmapMemory(backend->device->context->device, vertex_staging_buffer->device_memory);

	VkDeviceSize indices_size = sizeof(cube_indices[0]) * ARRAYSIZE(cube_indices);

	Vulkan::Buffer* index_staging_buffer = new Vulkan::Buffer(
		backend->device->context->device,
		backend->device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		indices_size);

	void* cube_index_data;
	vkMapMemory(backend->device->context->device, index_staging_buffer->device_memory, 0, index_staging_buffer->size, 0, &cube_index_data);
	memcpy(cube_index_data, cube_indices, (size_t)index_staging_buffer->size);
	vkUnmapMemory(backend->device->context->device, index_staging_buffer->device_memory);

	uint32_t vertex_offset = (uint32_t)backend->device->upload_vertex_buffer(vertex_staging_buffer);
	uint32_t index_offset = (uint32_t)backend->device->upload_index_buffer(index_staging_buffer);

	meshes[mesh_id].index_offset = index_offset / sizeof(uint16_t);
	meshes[mesh_id].index_count = ARRAYSIZE(cube_indices);
	meshes[mesh_id].vertex_offset = vertex_offset / sizeof(Vertex);

	vertex_staging_buffer->destroy(backend->device->context->device);
	index_staging_buffer->destroy(backend->device->context->device);

	mesh_id++;

	// NOTE: A plane mesh
	Vertex plane_vertices[] = {
		{ { -0.3f, -0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.36f, 0.03f }, {1.0f, 0.0f} },
		{ { 0.3f, -0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.36f, 0.03f }, {0.0f, 0.0f} },
		{ {	0.3f, 0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.36f, 0.03f }, {0.0f, 1.0f} },
		{ {	-0.3f, 0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.36f, 0.03f }, {1.0f, 1.0f} },
	};

	uint16_t plane_indices[] = {
		0, 1, 2,
		2, 3, 0,
	};

	vertices_size = sizeof(plane_vertices[0]) * ARRAYSIZE(plane_vertices);

	vertex_staging_buffer = new Vulkan::Buffer(
		backend->device->context->device,
		backend->device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		vertices_size);

	void* plane_vertex_data;
	vkMapMemory(backend->device->context->device, vertex_staging_buffer->device_memory, 0, vertex_staging_buffer->size, 0, &plane_vertex_data);
	memcpy(plane_vertex_data, plane_vertices, (size_t)vertex_staging_buffer->size);
	vkUnmapMemory(backend->device->context->device, vertex_staging_buffer->device_memory);

	indices_size = sizeof(plane_indices[0]) * ARRAYSIZE(plane_indices);

	index_staging_buffer = new Vulkan::Buffer(
		backend->device->context->device,
		backend->device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		indices_size);

	void* plane_index_data;
	vkMapMemory(backend->device->context->device, index_staging_buffer->device_memory, 0, index_staging_buffer->size, 0, &plane_index_data);
	memcpy(plane_index_data, plane_indices, (size_t)index_staging_buffer->size);
	vkUnmapMemory(backend->device->context->device, index_staging_buffer->device_memory);

	vertex_offset = (uint32_t)backend->device->upload_vertex_buffer(vertex_staging_buffer);
	index_offset = (uint32_t)backend->device->upload_index_buffer(index_staging_buffer);

	meshes[mesh_id].index_offset = index_offset / sizeof(uint16_t);
	meshes[mesh_id].index_count = ARRAYSIZE(plane_indices);
	meshes[mesh_id].vertex_offset = vertex_offset / sizeof(Vertex);

	vertex_staging_buffer->destroy(backend->device->context->device);
	index_staging_buffer->destroy(backend->device->context->device);

	mesh_id++;

	// Static Entitites

	// A makeshift board for the game
	// Build a surrounding wall
	glm::vec3 scale(1.0f, 1.0f, 1.0f);
	float distance = 0.6f;
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
				static_transforms[transform_index++] = { glm::vec3(r * distance + offset_x, 0.6f, c * distance + offset_z), scale };
			}
			else if (r == 0 || r == board_width - 1)
			{
				static_transforms[transform_index++] = { glm::vec3(r * distance + offset_x, 0.6f, c * distance + offset_z), scale };
			}
		}
	}

	// Upload the transforms for the walls
	VkDeviceSize instances_size = sizeof(static_transforms[0]) * transform_index;

	Vulkan::Buffer* instance_staging_buffer = new Vulkan::Buffer(
		backend->device->context->device,
		backend->device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		instances_size);

	void* wall_instance_data;
	vkMapMemory(backend->device->context->device, instance_staging_buffer->device_memory, 0, instance_staging_buffer->size, 0, &wall_instance_data);
	memcpy(wall_instance_data, static_transforms, (size_t)instance_staging_buffer->size);
	vkUnmapMemory(backend->device->context->device, instance_staging_buffer->device_memory);

	static_entitites[0] = {};
	static_entitites[0].count = transform_index;
	static_entitites[0].mesh_id = 0;
	static_entitites[0].transform_offset = (uint32_t)backend->device->upload_instance_buffer(instance_staging_buffer);

	instance_staging_buffer->destroy(backend->device->context->device);

	// Generate the transforms for the ground
	static_transforms[transform_index] = { glm::vec3(distance * 0.5f, 0.0f, distance * -1.5f), glm::vec3(board_width, 1.0f, board_height) , glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)) };

	// Upload the transforms for the grounnd
	instances_size = sizeof(static_transforms[0]) * 1;

	instance_staging_buffer = new Vulkan::Buffer(
		backend->device->context->device,
		backend->device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		instances_size);

	void* ground_instance_data;
	vkMapMemory(backend->device->context->device, instance_staging_buffer->device_memory, 0, instance_staging_buffer->size, 0, &ground_instance_data);
	memcpy(ground_instance_data, &static_transforms[transform_index], (size_t)instance_staging_buffer->size);
	vkUnmapMemory(backend->device->context->device, instance_staging_buffer->device_memory);

	static_entitites[1] = {};
	static_entitites[1].count = transform_index;
	static_entitites[1].mesh_id = 1;
	static_entitites[1].transform_offset = (uint32_t)backend->device->upload_instance_buffer(instance_staging_buffer);

	instance_staging_buffer->destroy(backend->device->context->device);
	delete instance_staging_buffer;

	dynamic_entities[0].mesh_id = 0;

	// Load chequered texture
	int tex_width;
	int tex_height;
	int tex_channels;
	stbi_uc* pixels = stbi_load("../data/textures/debug.png", &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
	VkDeviceSize texture_size = tex_width * tex_height * 4;

	if (!pixels)
		assert(!"Failed to load texture");

	Vulkan::Buffer* texture_staging_buffer = new Vulkan::Buffer(
		backend->device->context->device,
		backend->device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		texture_size);

	void* texture_data;
	vkMapMemory(backend->device->context->device, texture_staging_buffer->device_memory, 0, texture_staging_buffer->size, 0, &texture_data);
	memcpy(texture_data, pixels, (size_t)texture_staging_buffer->size);
	vkUnmapMemory(backend->device->context->device, texture_staging_buffer->device_memory);

	stbi_image_free(pixels);

	VkExtent2D texture_extent = { (uint32_t)tex_width, (uint32_t)tex_height };
	VkFormat color_format = backend->device->wsi->surface_format.format;
	texture_image = new Vulkan::Image(backend->device->context->device, backend->device->context->gpu, texture_extent, color_format, VK_SAMPLE_COUNT_1_BIT,
									 VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_TILING_OPTIMAL,
								 	 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
									 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// NOTE: The upload_buffer_to_image function takes care of the necessary transitions between
	// image layouts and queues
	backend->device->upload_buffer_to_image(texture_staging_buffer->buffer, texture_image->image, color_format, texture_extent.width, texture_extent.height, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	texture_staging_buffer->destroy(backend->device->context->device);

	VkSamplerCreateInfo sampler_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	if (backend->device->context->gpu_enabled_features.samplerAnisotropy == VK_TRUE)
	{
		sampler_info.anisotropyEnable = VK_TRUE;
		sampler_info.maxAnisotropy = 16;
	}
	else
	{
		sampler_info.anisotropyEnable = VK_FALSE;
		sampler_info.maxAnisotropy = 1;
	}
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	VkResult result = vkCreateSampler(backend->device->context->device, &sampler_info, nullptr, &texture_sampler);
	assert(result == VK_SUCCESS);
}

void Renderer::render_frame(Game::State* game_state, float delta_time)
{
	auto frame_cpu_start = std::chrono::high_resolution_clock::now();

	Vulkan::FrameResources& frame_resources = backend->device->begin_draw_frame();
	Frame* frame = (Frame*) frame_resources.custom;
	
	if (frame == nullptr)
	{
		frame = &frames[backend->device->frame_index];
		frame_resources.custom = frame;
	}

	if (frame->descriptor_set_count > 0)
	{
		if (frame->descriptor_sets[0] == VK_NULL_HANDLE)
		{
			VkResult result = allocate_descriptor_set(static_pipeline.descriptor_set_layouts[0], frame->descriptor_sets[0]);
			assert(result == VK_SUCCESS);

			VkDescriptorBufferInfo buffer_info = {};
			buffer_info.buffer = frame->view_ubo_buffer->buffer;
			buffer_info.offset = 0;
			buffer_info.range = VK_WHOLE_SIZE;

			VkDescriptorImageInfo image_info = {};
			image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_info.imageView = texture_image->image_view;
			image_info.sampler = texture_sampler;

			VkWriteDescriptorSet descriptor_writes[2];
			descriptor_writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptor_writes[0].dstSet = frame->descriptor_sets[0];
			descriptor_writes[0].dstBinding = 0;
			descriptor_writes[0].dstArrayElement = 0;
			descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor_writes[0].descriptorCount = 1;
			descriptor_writes[0].pBufferInfo = &buffer_info;

			descriptor_writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptor_writes[1].dstSet = frame->descriptor_sets[0];
			descriptor_writes[1].dstBinding = 1;
			descriptor_writes[1].dstArrayElement = 0;
			descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptor_writes[1].descriptorCount = 1;
			descriptor_writes[1].pImageInfo = &image_info;

			vkUpdateDescriptorSets(backend->device->context->device, ARRAYSIZE(descriptor_writes), descriptor_writes, 0, nullptr);
		}
	}

	VkViewport viewport = { 0, 0, (float)backend->device->wsi->swapchain_extent.width, (float)backend->device->wsi->swapchain_extent.height, 0.0f, 1.0f };
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = backend->device->wsi->swapchain_extent;

	VkDeviceSize offsets[] = { 0 };

	update_uniform_buffers(frame_resources);
	vkCmdBindDescriptorSets(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_pipeline.pipeline_layout, 0, frame->descriptor_set_count, frame->descriptor_sets, 0, nullptr);

	// Static Pipeline
	vkCmdBindPipeline(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_pipeline.pipeline);

	vkCmdSetViewport(frame_resources.command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(frame_resources.command_buffer, 0, 1, &scissor);

	// vkCmdBindDescriptorSets(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_pipeline.pipeline_layout, 0, frame->descriptor_set_count, frame->descriptor_sets, 0, nullptr);

	VkBuffer vertex_buffers[] = { backend->device->vertex_buffer->buffer };
	VkBuffer instance_buffers[] = { backend->device->instance_buffer->buffer };

	// Bind point 0: Mesh vertex buffer
	vkCmdBindVertexBuffers(frame_resources.command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(frame_resources.command_buffer, backend->device->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT16);

	for (uint32_t i = 0; i < static_entity_count; ++i)
	{
		VkDeviceSize offsets[] = { static_entitites[i].transform_offset };
		// Bind point 1: Instance data buffer
		vkCmdBindVertexBuffers(frame_resources.command_buffer, 1, 1, instance_buffers, offsets);
		// Draw all instances at once
		Mesh& mesh = meshes[static_entitites[i].mesh_id];
		vkCmdDrawIndexed(frame_resources.command_buffer, mesh.index_count, static_entitites[i].count, mesh.index_offset, mesh.vertex_offset, 0);
	}

	vkCmdBindPipeline(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dynamic_pipeline.pipeline);

	vkCmdSetViewport(frame_resources.command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(frame_resources.command_buffer, 0, 1, &scissor);

	glm::mat4 model(1.0f);
	model = glm::translate(model, game_state->player_transform.position);
	vkCmdPushConstants(frame_resources.command_buffer, dynamic_pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);

	// Bind point 0: Mesh vertex buffer
	vkCmdBindVertexBuffers(frame_resources.command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(frame_resources.command_buffer, backend->device->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT16);

	for (uint32_t i = 0; i < dynamic_entity_count; ++i)
	{
		Mesh& mesh = meshes[dynamic_entities[i].mesh_id];
		vkCmdDrawIndexed(frame_resources.command_buffer, mesh.index_count, 1, mesh.index_offset, mesh.vertex_offset, 0);
	}

	// Debug Draw
	vkCmdBindPipeline(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_pipeline.pipeline);

	vkCmdSetViewport(frame_resources.command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(frame_resources.command_buffer, 0, 1, &scissor);
	vkCmdSetLineWidth(frame_resources.command_buffer, 1.0f);
	vkCmdBindVertexBuffers(frame_resources.command_buffer, 0, 1, &frame->debug_vertex_buffer->buffer, offsets);
	vkCmdDraw(frame_resources.command_buffer, 12, 1, 0, 0);

	backend->device->end_draw_frame(frame_resources);

	auto frame_cpu_end = std::chrono::high_resolution_clock::now();
	frame_cpu_avg = frame_cpu_avg * 0.95 + (frame_cpu_end - frame_cpu_start).count() * 1e-6 * 0.05;
}

VkResult Renderer::allocate_descriptor_set(const VkDescriptorSetLayout& descriptor_set_layout, VkDescriptorSet& descriptor_set)
{
	VkDescriptorSetAllocateInfo descriptor_set_ai = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptor_set_ai.descriptorPool = descriptor_pool;
	descriptor_set_ai.descriptorSetCount = 1;
	descriptor_set_ai.pSetLayouts = &descriptor_set_layout;

	return vkAllocateDescriptorSets(backend->device->context->device, &descriptor_set_ai, &descriptor_set);
}

void Renderer::prepare_uniform_buffers()
{
	for (uint32_t i = 0; i < ARRAYSIZE(frames); ++i)
	{
		glm::vec3 camera_position = { 0.0f, 15.0f, 5.0f };

		ViewUniformBufferObject ubo = {};
		ubo.view = glm::lookAt(camera_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.projection = glm::perspective(glm::radians(45.0f), (float)backend->device->wsi->swapchain_extent.width / (float)backend->device->wsi->swapchain_extent.height, 0.001f, 100.0f);
		ubo.projection[1][1] *= -1;

		void* data;
		vkMapMemory(backend->device->context->device, frames[i].view_ubo_buffer->device_memory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(backend->device->context->device, frames[i].view_ubo_buffer->device_memory);
	}
}

void Renderer::update_uniform_buffers(Vulkan::FrameResources& frame_resources)
{
	Frame* frame = (Frame*) frame_resources.custom;
	assert(frame != nullptr);

	// TODO: Skip updating if nothing has changed.
	// This is here now because on resize the aspect ration can change, and we need to
	// recreate the projection matrix accordingly
	glm::vec3 camera_position = { 0.0f, 15.0f, 5.0f };

	ViewUniformBufferObject ubo = {};
	ubo.view = glm::lookAt(camera_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.projection = glm::perspective(glm::radians(45.0f), (float)backend->device->wsi->swapchain_extent.width / (float)backend->device->wsi->swapchain_extent.height, 0.001f, 100.0f);
	ubo.projection[1][1] *= -1;

	void* data;
	vkMapMemory(backend->device->context->device, frame->view_ubo_buffer->device_memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(backend->device->context->device, frame->view_ubo_buffer->device_memory);
}

void Renderer::prepare_debug_vertex_buffers()
{
	for (uint32_t i = 0; i < ARRAYSIZE(frames); ++i)
	{
		DebugLine debug_lines[] = {
			glm::vec3(-10.0f, 0.0f, -0.3f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(10.0f, 0.0f, -0.3f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),

			glm::vec3(-10.0f, 0.0f, 0.3f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(10.0f, 0.0f, 0.3f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),

			glm::vec3(-10.0f, 0.0f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(10.0f, 0.0f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),

			glm::vec3(-0.3f, 0.0f, -10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(-0.3f, 0.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),

			glm::vec3(0.3f, 0.0f, -10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(0.3f, 0.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),

			glm::vec3(0.9f, 0.0f, -10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(0.9f, 0.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
		};

		vkMapMemory(backend->device->context->device, frames[i].debug_vertex_buffer->device_memory, 0, sizeof(DebugLine) * ARRAYSIZE(debug_lines), 0, &frames[i].debug_vertex_buffer->data);
		memcpy(frames[i].debug_vertex_buffer->data, &debug_lines, sizeof(DebugLine) * ARRAYSIZE(debug_lines));
		vkUnmapMemory(backend->device->context->device, frames[i].debug_vertex_buffer->device_memory);
	}
}

void Renderer::cleanup()
{
	vkQueueWaitIdle(backend->device->context->graphics_queue);
	vkDestroySampler(backend->device->context->device, texture_sampler, nullptr);
	destroy_ubo_buffers();
	destroy_descriptor_pool();

	// TODO: Move this its onw cleanup function
	for (uint32_t i = 0; i < Vulkan::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		frames[i].debug_vertex_buffer->destroy(backend->device->context->device);
	}

	vkDestroyDescriptorSetLayout(backend->device->context->device, static_pipeline.descriptor_set_layouts[0], nullptr);
	static_pipeline.descriptor_set_layouts[0] = VK_NULL_HANDLE;

	vkDestroyPipeline(backend->device->context->device, static_pipeline.pipeline, nullptr);
	static_pipeline.pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(backend->device->context->device, static_pipeline.pipeline_layout, nullptr);
	static_pipeline.pipeline_layout = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(backend->device->context->device, dynamic_pipeline.descriptor_set_layouts[0], nullptr);
	dynamic_pipeline.descriptor_set_layouts[0] = VK_NULL_HANDLE;

	vkDestroyPipeline(backend->device->context->device, dynamic_pipeline.pipeline, nullptr);
	dynamic_pipeline.pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(backend->device->context->device, dynamic_pipeline.pipeline_layout, nullptr);
	dynamic_pipeline.pipeline_layout = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(backend->device->context->device, debug_pipeline.descriptor_set_layouts[0], nullptr);
	debug_pipeline.descriptor_set_layouts[0] = VK_NULL_HANDLE;

	vkDestroyPipeline(backend->device->context->device, debug_pipeline.pipeline, nullptr);
	debug_pipeline.pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(backend->device->context->device, debug_pipeline.pipeline_layout, nullptr);
	debug_pipeline.pipeline_layout = VK_NULL_HANDLE;

	texture_image->destroy(backend->device->context->device);

	backend->device->cleanup();
	backend->wsi->cleanup();
}

void Renderer::create_pipelines()
{
	// Shared resources.
	// The static and dynamic pipelines share some of the resources (like the view uniform buffer)
	// and some stages, so we create them upfront

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_ci.primitiveRestartEnable = VK_FALSE;

	// Viewport and Scissor
	VkViewport viewport = { 0, 0, (float)backend->device->wsi->swapchain_extent.width, (float)backend->device->wsi->swapchain_extent.height, 0.0f, 1.0f };
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = backend->device->wsi->swapchain_extent;
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
	multisampling_ci.rasterizationSamples = backend->device->context->msaa_samples;
	if (backend->device->context->gpu_enabled_features.sampleRateShading == VK_TRUE)
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

	VkPipelineColorBlendStateCreateInfo color_blend_ci = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	color_blend_ci.logicOpEnable = VK_FALSE;
	color_blend_ci.attachmentCount = 1;
	color_blend_ci.pAttachments = &color_blend_attachment_ci;

	// Dynamic States (state that can be changed without having to recreate the pipeline)
	VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH };
	VkPipelineDynamicStateCreateInfo dynamic_state_ci = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamic_state_ci.dynamicStateCount = ARRAYSIZE(dynamic_states);
	dynamic_state_ci.pDynamicStates = dynamic_states;

	// Pipelines creation
	// Pipeline 1: Graphics pipeline for static entities
	// Pipeline Fixed Functions
	// Vertex Input
	VkPipelineVertexInputStateCreateInfo static_vertex_input_ci = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	// A vertex binding describes at which rate to load data from memory throughout the vertices. 
	// It specifies the number of bytes between data entries and whether to move to the next 
	// data entry after each vertex or after each instance.
	VkVertexInputBindingDescription static_vertex_binding_descriptions[2];
	// Mesh vertex bindings
	static_vertex_binding_descriptions[0] = {};
	static_vertex_binding_descriptions[0].binding = 0;
	static_vertex_binding_descriptions[0].stride = sizeof(Vertex);
	static_vertex_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	// Instance transform bindings (per instance position, scale and rotation)
	static_vertex_binding_descriptions[1] = {};
	static_vertex_binding_descriptions[1].binding = 1;
	static_vertex_binding_descriptions[1].stride = sizeof(Transform);
	static_vertex_binding_descriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

	static_vertex_input_ci.vertexBindingDescriptionCount = ARRAYSIZE(static_vertex_binding_descriptions);
	static_vertex_input_ci.pVertexBindingDescriptions = static_vertex_binding_descriptions;

	// An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data 
	// originating from a binding description. We have 4 attributes, position, normal, color, and tex_coord
	// so we need 4 attribute description structs
	// We also have 3 additional attribute, instance position, scale and rotation
	VkVertexInputAttributeDescription static_vertex_input_attribute_descriptions[7];
	// Per-vertex attributes
	// Position
	static_vertex_input_attribute_descriptions[0].binding = 0;
	static_vertex_input_attribute_descriptions[0].location = 0;
	static_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	static_vertex_input_attribute_descriptions[0].offset = offsetof(Vertex, position);
	// Normal
	static_vertex_input_attribute_descriptions[1].binding = 0;
	static_vertex_input_attribute_descriptions[1].location = 1;
	static_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	static_vertex_input_attribute_descriptions[1].offset = offsetof(Vertex, normal);
	// Color
	static_vertex_input_attribute_descriptions[2].binding = 0;
	static_vertex_input_attribute_descriptions[2].location = 2;
	static_vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	static_vertex_input_attribute_descriptions[2].offset = offsetof(Vertex, color);
	// Tex Coord
	static_vertex_input_attribute_descriptions[3].binding = 0;
	static_vertex_input_attribute_descriptions[3].location = 3;
	static_vertex_input_attribute_descriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	static_vertex_input_attribute_descriptions[3].offset = offsetof(Vertex, tex_coord);

	// Per-instance attributes
	// Instance position
	static_vertex_input_attribute_descriptions[4].binding = 1;
	static_vertex_input_attribute_descriptions[4].location = 4;
	static_vertex_input_attribute_descriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
	static_vertex_input_attribute_descriptions[4].offset = offsetof(Transform, position);
	// Instance scale
	static_vertex_input_attribute_descriptions[5].binding = 1;
	static_vertex_input_attribute_descriptions[5].location = 5;
	static_vertex_input_attribute_descriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
	static_vertex_input_attribute_descriptions[5].offset = offsetof(Transform, scale);
	// Instance rotation
	static_vertex_input_attribute_descriptions[6].binding = 1;
	static_vertex_input_attribute_descriptions[6].location = 6;
	static_vertex_input_attribute_descriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	static_vertex_input_attribute_descriptions[6].offset = offsetof(Transform, rotation);

	static_vertex_input_ci.vertexAttributeDescriptionCount = ARRAYSIZE(static_vertex_input_attribute_descriptions);
	static_vertex_input_ci.pVertexAttributeDescriptions = static_vertex_input_attribute_descriptions;

	// View Uniform Buffer
	VkDescriptorSetLayoutBinding view_ubo_layout_binding = {};
	view_ubo_layout_binding.binding = 0;
	view_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	view_ubo_layout_binding.descriptorCount = 1;
	view_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// Texture sampler
	VkDescriptorSetLayoutBinding sampler_layout_binding = {};
	sampler_layout_binding.binding = 1;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.pImmutableSamplers = nullptr;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding bindings[] = { view_ubo_layout_binding, sampler_layout_binding };

	VkDescriptorSetLayoutCreateInfo static_descriptor_layout_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	static_descriptor_layout_ci.bindingCount = ARRAYSIZE(bindings);
	static_descriptor_layout_ci.pBindings = bindings;
	VkDescriptorSetLayoutCreateInfo static_descriptor_layout_cis[] = { static_descriptor_layout_ci };

	static_pipeline = create_pipeline("../data/shaders/static_entity.vert.spv", "../data/shaders/static_entity.frag.spv", static_vertex_input_ci, input_assembly_ci, ARRAYSIZE(static_descriptor_layout_cis), static_descriptor_layout_cis,
		viewport_ci, rasterizer_ci, depth_stencil_ci, multisampling_ci, color_blend_ci, dynamic_state_ci, 0, nullptr);

	// Pipeline 2: Graphics pipeline for dynamic entities
	// Pipeline Fixed Functions
	// Vertex Input
	VkPipelineVertexInputStateCreateInfo dynamic_vertex_input_ci = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	// A vertex binding describes at which rate to load data from memory throughout the vertices. 
	// It specifies the number of bytes between data entries and whether to move to the next 
	// data entry after each vertex or after each instance.
	VkVertexInputBindingDescription dynamic_vertex_binding_descriptions[1];
	// Mesh vertex bindings
	dynamic_vertex_binding_descriptions[0] = {};
	dynamic_vertex_binding_descriptions[0].binding = 0;
	dynamic_vertex_binding_descriptions[0].stride = sizeof(Vertex);
	dynamic_vertex_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	dynamic_vertex_input_ci.vertexBindingDescriptionCount = ARRAYSIZE(dynamic_vertex_binding_descriptions);
	dynamic_vertex_input_ci.pVertexBindingDescriptions = dynamic_vertex_binding_descriptions;

	// An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data 
	// originating from a binding description. We have 4 attributes, position, normal, color and tex_coord
	// so we need 4 attribute description structs
	VkVertexInputAttributeDescription dynamic_vertex_input_attribute_descriptions[4];
	// Per-vertex attributes
	// Position
	dynamic_vertex_input_attribute_descriptions[0].binding = 0;
	dynamic_vertex_input_attribute_descriptions[0].location = 0;
	dynamic_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	dynamic_vertex_input_attribute_descriptions[0].offset = offsetof(Vertex, position);
	// Normal
	dynamic_vertex_input_attribute_descriptions[1].binding = 0;
	dynamic_vertex_input_attribute_descriptions[1].location = 1;
	dynamic_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	dynamic_vertex_input_attribute_descriptions[1].offset = offsetof(Vertex, normal);
	// Color
	dynamic_vertex_input_attribute_descriptions[2].binding = 0;
	dynamic_vertex_input_attribute_descriptions[2].location = 2;
	dynamic_vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	dynamic_vertex_input_attribute_descriptions[2].offset = offsetof(Vertex, color);
	// Tex Coord
	dynamic_vertex_input_attribute_descriptions[3].binding = 0;
	dynamic_vertex_input_attribute_descriptions[3].location = 3;
	dynamic_vertex_input_attribute_descriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	dynamic_vertex_input_attribute_descriptions[3].offset = offsetof(Vertex, tex_coord);

	dynamic_vertex_input_ci.vertexAttributeDescriptionCount = ARRAYSIZE(dynamic_vertex_input_attribute_descriptions);
	dynamic_vertex_input_ci.pVertexAttributeDescriptions = dynamic_vertex_input_attribute_descriptions;

	VkDescriptorSetLayoutBinding dynamic_bindings[] = { view_ubo_layout_binding, sampler_layout_binding };

	VkDescriptorSetLayoutCreateInfo dynamic_descriptor_layout_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	dynamic_descriptor_layout_ci.bindingCount = ARRAYSIZE(dynamic_bindings);
	dynamic_descriptor_layout_ci.pBindings = dynamic_bindings;
	VkDescriptorSetLayoutCreateInfo dynamic_descriptor_layout_cis[] = { dynamic_descriptor_layout_ci };

	VkPushConstantRange player_matrix;
	player_matrix.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	player_matrix.size = sizeof(glm::mat4);
	player_matrix.offset = 0;

	VkPushConstantRange push_constant_ranges[] = { player_matrix };

	dynamic_pipeline = create_pipeline("../data/shaders/dynamic_entity.vert.spv", "../data/shaders/dynamic_entity.frag.spv", dynamic_vertex_input_ci, input_assembly_ci, ARRAYSIZE(dynamic_descriptor_layout_cis), dynamic_descriptor_layout_cis,
		viewport_ci, rasterizer_ci, depth_stencil_ci, multisampling_ci, color_blend_ci, dynamic_state_ci, ARRAYSIZE(push_constant_ranges), push_constant_ranges);

	// Pipeline 3: Debug Draw pipeline for debug gizmos
	// Pipeline Fixed Functions
	// Vertex Input
	VkPipelineVertexInputStateCreateInfo debug_vertex_input_ci = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	// A vertex binding describes at which rate to load data from memory throughout the vertices. 
	// It specifies the number of bytes between data entries and whether to move to the next 
	// data entry after each vertex or after each instance.
	VkVertexInputBindingDescription debug_vertex_binding_descriptions[1];
	// Mesh vertex bindings
	debug_vertex_binding_descriptions[0] = {};
	debug_vertex_binding_descriptions[0].binding = 0;
	debug_vertex_binding_descriptions[0].stride = sizeof(DebugLine);
	debug_vertex_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	debug_vertex_input_ci.vertexBindingDescriptionCount = ARRAYSIZE(debug_vertex_binding_descriptions);
	debug_vertex_input_ci.pVertexBindingDescriptions = debug_vertex_binding_descriptions;

	// An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data 
	// originating from a binding description. We have 3 attributes, position, normal and color
	// so we need 3 attribute description structs
	VkVertexInputAttributeDescription debug_vertex_input_attribute_descriptions[3];
	// Per-vertex attributes
	// Position
	debug_vertex_input_attribute_descriptions[0].binding = 0;
	debug_vertex_input_attribute_descriptions[0].location = 0;
	debug_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	debug_vertex_input_attribute_descriptions[0].offset = offsetof(DebugLine, position);
	// Normal
	debug_vertex_input_attribute_descriptions[1].binding = 0;
	debug_vertex_input_attribute_descriptions[1].location = 1;
	debug_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	debug_vertex_input_attribute_descriptions[1].offset = offsetof(DebugLine, normal);
	// Color
	debug_vertex_input_attribute_descriptions[2].binding = 0;
	debug_vertex_input_attribute_descriptions[2].location = 2;
	debug_vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	debug_vertex_input_attribute_descriptions[2].offset = offsetof(DebugLine, color);

	debug_vertex_input_ci.vertexAttributeDescriptionCount = ARRAYSIZE(debug_vertex_input_attribute_descriptions);
	debug_vertex_input_ci.pVertexAttributeDescriptions = debug_vertex_input_attribute_descriptions;

	VkDescriptorSetLayoutBinding debug_bindings[] = { view_ubo_layout_binding, sampler_layout_binding };

	VkDescriptorSetLayoutCreateInfo debug_descriptor_layout_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	debug_descriptor_layout_ci.bindingCount = ARRAYSIZE(debug_bindings);
	debug_descriptor_layout_ci.pBindings = debug_bindings;
	VkDescriptorSetLayoutCreateInfo debug_descriptor_layout_cis[] = { debug_descriptor_layout_ci };

	input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

	// Depth Buffer
	VkPipelineDepthStencilStateCreateInfo debug_depth_stencil_ci = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	debug_depth_stencil_ci.depthTestEnable = VK_FALSE;
	debug_depth_stencil_ci.depthWriteEnable = VK_TRUE;
	debug_depth_stencil_ci.depthCompareOp = VK_COMPARE_OP_LESS;
	debug_depth_stencil_ci.depthBoundsTestEnable = VK_FALSE;
	debug_depth_stencil_ci.stencilTestEnable = VK_FALSE;

	debug_pipeline = create_pipeline("../data/shaders/debug_draw.vert.spv", "../data/shaders/debug_draw.frag.spv", debug_vertex_input_ci, input_assembly_ci, ARRAYSIZE(debug_descriptor_layout_cis), debug_descriptor_layout_cis,
		viewport_ci, rasterizer_ci, debug_depth_stencil_ci, multisampling_ci, color_blend_ci, dynamic_state_ci, 0, nullptr);
}

Pipeline Renderer::create_pipeline(const char* vertex_shader_path, const char* fragment_shader_path, VkPipelineVertexInputStateCreateInfo vertex_input_ci, VkPipelineInputAssemblyStateCreateInfo input_assembly_ci,
								   uint32_t descriptor_count, VkDescriptorSetLayoutCreateInfo* descriptor_layout_cis, VkPipelineViewportStateCreateInfo viewport_ci, VkPipelineRasterizationStateCreateInfo rasterizer_ci,
								   VkPipelineDepthStencilStateCreateInfo depth_stencil_ci, VkPipelineMultisampleStateCreateInfo multisampling_ci, VkPipelineColorBlendStateCreateInfo color_blend_ci,
								   VkPipelineDynamicStateCreateInfo dynamic_state_ci, uint32_t push_constant_range_count, VkPushConstantRange* push_constant_ranges)
{
	Pipeline graphics_pipeline = {};

	VkPipelineShaderStageCreateInfo shader_stages[2];
	shader_stages[0] = Vulkan::Shader::load_shader(backend->device->context->device, backend->device->context->gpu, vertex_shader_path, VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = Vulkan::Shader::load_shader(backend->device->context->device, backend->device->context->gpu, fragment_shader_path, VK_SHADER_STAGE_FRAGMENT_BIT);

	graphics_pipeline.descriptor_set_layout_count = descriptor_count;
	for (uint32_t i = 0; i < descriptor_count; ++i)
	{
		VkResult result = vkCreateDescriptorSetLayout(backend->device->context->device, &descriptor_layout_cis[i], nullptr, &graphics_pipeline.descriptor_set_layouts[i]);
		assert(result == VK_SUCCESS);
	}

	// Pipeline Layout
	VkPipelineLayoutCreateInfo pipeline_layout_ci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipeline_layout_ci.setLayoutCount = graphics_pipeline.descriptor_set_layout_count;
	pipeline_layout_ci.pSetLayouts = graphics_pipeline.descriptor_set_layouts;

	if (push_constant_range_count > 0)
	{
		pipeline_layout_ci.pushConstantRangeCount = push_constant_range_count;
		pipeline_layout_ci.pPushConstantRanges = push_constant_ranges;
	}

	VkResult result = vkCreatePipelineLayout(backend->device->context->device, &pipeline_layout_ci, nullptr, &graphics_pipeline.pipeline_layout);
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
	pipeline_ci.layout = graphics_pipeline.pipeline_layout;
	pipeline_ci.renderPass = backend->device->render_pass;
	pipeline_ci.subpass = 0;

	result = vkCreateGraphicsPipelines(backend->device->context->device, nullptr, 1, &pipeline_ci, nullptr, &graphics_pipeline.pipeline);
	assert(result == VK_SUCCESS);

	for (uint32_t i = 0; i < ARRAYSIZE(shader_stages); ++i)
	{
		vkDestroyShaderModule(backend->device->context->device, shader_stages[i].module, nullptr);
	}

	return graphics_pipeline;
}

// Descriptor Pool helpers
void Renderer::create_descriptor_pool()
{
	VkDescriptorPoolSize pool_sizes[2];
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = 2 * Vulkan::MAX_FRAMES_IN_FLIGHT;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = 2 * Vulkan::MAX_FRAMES_IN_FLIGHT;

	VkDescriptorPoolCreateInfo pool_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_ci.poolSizeCount = ARRAYSIZE(pool_sizes);
	pool_ci.pPoolSizes = pool_sizes;
	pool_ci.maxSets = 2 * Vulkan::MAX_FRAMES_IN_FLIGHT;

	VkResult result = vkCreateDescriptorPool(backend->device->context->device, &pool_ci, nullptr, &descriptor_pool);
	assert(result == VK_SUCCESS);
}

void Renderer::destroy_descriptor_pool()
{
	if (descriptor_pool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(backend->device->context->device, descriptor_pool, nullptr);
		descriptor_pool = VK_NULL_HANDLE;
	}
}

// UBO Buffers Helpers
void Renderer::create_ubo_buffers()
{
	for (uint32_t i = 0; i < Vulkan::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		frames[i].view_ubo_buffer = new Vulkan::Buffer(
			backend->device->context->device,
			backend->device->context->gpu,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ViewUniformBufferObject));
	}
}

void Renderer::destroy_ubo_buffers()
{
	for (uint32_t i = 0; i < Vulkan::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		frames[i].view_ubo_buffer->destroy(backend->device->context->device);
	}
}

} // namespace Renderer
