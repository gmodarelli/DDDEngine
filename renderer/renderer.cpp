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
#include "../resources/resources.h"
#include "../renderer/types.h"

namespace Renderer
{

Renderer::Renderer(Application::Platform* platform) : platform(platform)
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
		VkDeviceSize size = 1 * 1024 * 1024;
		frames[i].debug_vertex_buffer = new Vulkan::Buffer(
			backend->device->context->device,
			backend->device->context->gpu,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			size);
	}

	prepare_debug_vertex_buffers();
}

void Renderer::upload_buffers(const Game::State* game_state)
{
	const Resources::AssetsInfo* assets_info = game_state->assets_info;
	// Upload all vertices to the Vertex Buffer
	VkDeviceSize vertices_size = sizeof(assets_info->vertices[0]) * assets_info->vertex_offset;

	Vulkan::Buffer* vertex_staging_buffer = new Vulkan::Buffer(
		backend->device->context->device,
		backend->device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		vertices_size);

	void* vertices_data;
	vkMapMemory(backend->device->context->device, vertex_staging_buffer->device_memory, 0, vertex_staging_buffer->size, 0, &vertices_data);
	memcpy(vertices_data, assets_info->vertices, vertices_size);
	vkUnmapMemory(backend->device->context->device, vertex_staging_buffer->device_memory);

	backend->device->upload_vertex_buffer(vertex_staging_buffer);
	vertex_staging_buffer->destroy(backend->device->context->device);

	// Upload all indices to the Index Buffer
	VkDeviceSize indices_size = sizeof(assets_info->indices[0]) * assets_info->index_offset;

	Vulkan::Buffer* index_staging_buffer = new Vulkan::Buffer(
		backend->device->context->device,
		backend->device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		indices_size);

	void* indices_data;
	vkMapMemory(backend->device->context->device, index_staging_buffer->device_memory, 0, index_staging_buffer->size, 0, &indices_data);
	memcpy(indices_data, assets_info->indices, indices_size);
	vkUnmapMemory(backend->device->context->device, index_staging_buffer->device_memory);

	backend->device->upload_index_buffer(index_staging_buffer);
	index_staging_buffer->destroy(backend->device->context->device);
}

void Renderer::upload_dynamic_uniform_buffers(const Game::State* game_state)
{
	// NOTE: The game_state->entities table contains a list of entities,
	// where the index in the table represent the entity id.
	// An entity references a model by its model_id.
	// Parallel to the game_state->entities we have the 
	// game_state->transforms, which contains the transforms (position,
	// scale and rotation) of an entity.
	// The 2 tables are of the same length and a transform at index i in
	// the game_state->transforms table, applies to the entity at index i
	// in the game_state->entities table.
	// By convention entity 0 corresponds to the player.
	// 
	// An entity references a model (via model_id) inside the
	// assets_info->models table, and the models contains information
	// about how to draw it. Usually a model is composed of multiple
	// nodes, and a node contains information about the primitives to 
	// render and also contains its onw transform.
	//
	// To be able to render an entity correctly, we need to calculate
	// one transform per model's node, and store them sequentally inside
	// a Dynamic Uniform Buffer on the GPU.
	//
	// As an example, image we have 2 entities E1 and E2, that point to
	// two distinct model M1 and M2. M1 has 3 nodes M1N1, M1N2 and M1N3,
	// while M2 has 2, M2N1 and M2N2.
	// 
	// This is what the data will look like in memory
	// CPU:
	// entities:   [ E1, E2 ]
	// transforms: [ T1, T2 ]
	//
	// GPU:
	// ubo:            [ M1N1 * T1, M1N2 * T1, M1N3 * T1, M2N1 * T2, M2N2 * T2 ]
	//
	// Calculate how many descriptor sets we need to allocate
	// and build the ubos

	size_t min_ubo_alignment = backend->device->context->gpu_properties.limits.minUniformBufferOffsetAlignment;
	size_t nodes = 0;

	for (uint32_t e = 0; e < game_state->entity_count; ++e)
	{
		const Entity& entity = game_state->entities[e];
		const Resources::Model& model = game_state->assets_info->models[entity.model_id];
		nodes += model.node_count;
	}

	dynamic_alignment = sizeof(glm::mat4);
	if (min_ubo_alignment > 0)
	{
		dynamic_alignment = (dynamic_alignment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
	}
	size_t buffer_size = nodes * dynamic_alignment;
	NodeUbo dynamic_ubo;
	dynamic_ubo.model = (glm::mat4*)_aligned_malloc(buffer_size, dynamic_alignment);
	assert(dynamic_ubo.model);

	uint32_t absolute_node_count = 0;

	for (uint32_t e = 0; e < game_state->entity_count; ++e)
	{
		const Entity& entity = game_state->entities[e];
		const Resources::Model& model = game_state->assets_info->models[entity.model_id];

		for (uint32_t n = 0; n < model.node_count; ++n)
		{
			// Aligned offset
			glm::mat4* model_matrix = (glm::mat4*)(((uint64_t)dynamic_ubo.model + (absolute_node_count * dynamic_alignment)));
			*model_matrix = glm::translate(glm::mat4(1.0f), model.nodes[n].translation);
			*model_matrix = glm::translate(*model_matrix, game_state->transforms[e].position);
			*model_matrix = glm::scale(*model_matrix, model.nodes[n].scale);
			*model_matrix = glm::scale(*model_matrix, game_state->transforms[e].scale);
			*model_matrix = *model_matrix * glm::toMat4(model.nodes[n].rotation) * glm::toMat4(game_state->transforms[e].rotation);

			absolute_node_count++;
		}
	}

	Vulkan::Buffer* uniform_staging_buffer = new Vulkan::Buffer(
		backend->device->context->device,
		backend->device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		buffer_size);

	void* uniform_data;
	vkMapMemory(backend->device->context->device, uniform_staging_buffer->device_memory, 0, uniform_staging_buffer->size, 0, &uniform_data);
	memcpy(uniform_data, dynamic_ubo.model, buffer_size);
	vkUnmapMemory(backend->device->context->device, uniform_staging_buffer->device_memory);

	VkDeviceSize offset = backend->device->upload_uniform_buffer(uniform_staging_buffer);
	uniform_staging_buffer->destroy(backend->device->context->device);
	_aligned_free(dynamic_ubo.model);

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = backend->device->uniform_buffer->buffer;
	buffer_info.offset = 0;
	buffer_info.range = buffer_size;

	node_descriptor_set = VK_NULL_HANDLE;

	VkDescriptorSetAllocateInfo descriptor_set_ai = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptor_set_ai.descriptorPool = descriptor_pool;
	descriptor_set_ai.descriptorSetCount = 1;
	descriptor_set_ai.pSetLayouts = &descriptor_set_layouts[1];

	VkResult result =  vkAllocateDescriptorSets(backend->device->context->device, &descriptor_set_ai, &node_descriptor_set);
	assert(result == VK_SUCCESS);

	VkWriteDescriptorSet descriptor_writes[1];
	descriptor_writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptor_writes[0].dstSet = node_descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &buffer_info;

	vkUpdateDescriptorSets(backend->device->context->device, 1, descriptor_writes, 0, nullptr);

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

	if (frame->view_descriptor_set == VK_NULL_HANDLE)
	{
		VkResult result = allocate_descriptor_set(descriptor_set_layouts[0], frame->view_descriptor_set);
		assert(result == VK_SUCCESS);

		VkDescriptorBufferInfo buffer_info = {};
		buffer_info.buffer = frame->view_ubo_buffer->buffer;
		buffer_info.offset = 0;
		buffer_info.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet descriptor_writes[1];
		descriptor_writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		descriptor_writes[0].dstSet = frame->view_descriptor_set;
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[0].descriptorCount = 1;
		descriptor_writes[0].pBufferInfo = &buffer_info;

		vkUpdateDescriptorSets(backend->device->context->device, ARRAYSIZE(descriptor_writes), descriptor_writes, 0, nullptr);
	}

	VkViewport viewport = { 0, 0, (float)backend->device->wsi->swapchain_extent.width, (float)backend->device->wsi->swapchain_extent.height, 0.0f, 1.0f };
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = backend->device->wsi->swapchain_extent;

	VkDeviceSize offsets[] = { 0 };
	update_uniform_buffers(game_state, frame_resources);

	vkCmdBindDescriptorSets(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dynamic_pipeline.pipeline_layout, 0, 1, &frame->view_descriptor_set, 0, nullptr);

	vkCmdBindPipeline(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dynamic_pipeline.pipeline);

	vkCmdSetViewport(frame_resources.command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(frame_resources.command_buffer, 0, 1, &scissor);

	VkBuffer vertex_buffers[] = { backend->device->vertex_buffer->buffer };

	// Bind point 0: Mesh vertex buffer
	vkCmdBindVertexBuffers(frame_resources.command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(frame_resources.command_buffer, backend->device->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT16);

	glm::mat4 model_matrix(1.0f);
	glm::vec3 transform_v3;
	transform_v3.x = game_state->transforms[game_state->player_entity_id].position.x;
	transform_v3.y = game_state->transforms[game_state->player_entity_id].position.y;
	transform_v3.z = game_state->transforms[game_state->player_entity_id].position.z;
	model_matrix = glm::translate(model_matrix, transform_v3);
	vkCmdPushConstants(frame_resources.command_buffer, dynamic_pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model_matrix);

	// Render the player
	Resources::Model model = game_state->assets_info->models[game_state->entities[game_state->player_entity_id].model_id];
	for (uint32_t n_id = 0; n_id < model.node_count; ++n_id)
	{
		uint32_t dynamic_offset = n_id * static_cast<uint32_t>(dynamic_alignment);
		Resources::Node node = model.nodes[n_id];
		vkCmdBindDescriptorSets(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dynamic_pipeline.pipeline_layout, 1, 1, &node_descriptor_set, 1, &dynamic_offset);
		Resources::Mesh mesh = game_state->assets_info->meshes[node.mesh_id];
		for (uint32_t p_id = 0; p_id < mesh.primitive_count; ++p_id)
		{
			Resources::Primitive primitive = mesh.primitives[p_id];
			Resources::Material material = game_state->assets_info->materials[primitive.material_id];
			vkCmdPushConstants(frame_resources.command_buffer, dynamic_pipeline.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(Resources::Material::PBRMetallicRoughness), &material.pbr_metallic_roughness);
			vkCmdDrawIndexed(frame_resources.command_buffer, primitive.index_count, 1, primitive.index_offset, 0, 0);
		}
	}

	// Debug Draw
	vkCmdBindPipeline(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_pipeline.pipeline);

	vkCmdSetViewport(frame_resources.command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(frame_resources.command_buffer, 0, 1, &scissor);
	vkCmdSetLineWidth(frame_resources.command_buffer, 1.0f);
	vkCmdBindVertexBuffers(frame_resources.command_buffer, 0, 1, &frame->debug_vertex_buffer->buffer, offsets);
	vkCmdDraw(frame_resources.command_buffer, 6, 1, 0, 0);

	backend->device->end_draw_frame(frame_resources);


	auto frame_cpu_end = std::chrono::high_resolution_clock::now();
	frame_cpu_avg = frame_cpu_avg * 0.95 + (frame_cpu_end - frame_cpu_start).count() * 1e-6 * 0.05;
}

VkResult Renderer::allocate_descriptor_set(const VkDescriptorSetLayout& descriptor_set_layout, VkDescriptorSet& descriptor_set, uint32_t descriptor_set_count)
{
	VkDescriptorSetAllocateInfo descriptor_set_ai = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptor_set_ai.descriptorPool = descriptor_pool;
	descriptor_set_ai.descriptorSetCount = descriptor_set_count;
	descriptor_set_ai.pSetLayouts = &descriptor_set_layout;

	return vkAllocateDescriptorSets(backend->device->context->device, &descriptor_set_ai, &descriptor_set);
}

void Renderer::prepare_uniform_buffers()
{
	for (uint32_t i = 0; i < ARRAYSIZE(frames); ++i)
	{
		ViewUniformBufferObject ubo = {};
		ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.projection = glm::perspective(glm::radians(45.0f), (float)backend->device->wsi->swapchain_extent.width / (float)backend->device->wsi->swapchain_extent.height, 0.001f, 100.0f);
		ubo.projection[1][1] *= -1;
		ubo.camera_position = glm::vec3(0.0f, 0.0f, 0.0f);

		void* data;
		vkMapMemory(backend->device->context->device, frames[i].view_ubo_buffer->device_memory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(backend->device->context->device, frames[i].view_ubo_buffer->device_memory);
	}
}

void Renderer::update_uniform_buffers(Game::State* game_state, Vulkan::FrameResources& frame_resources)
{
	Frame* frame = (Frame*) frame_resources.custom;
	assert(frame != nullptr);

	// TODO: Skip updating if nothing has changed.
	// This is here now because on resize the aspect ration can change, and we need to
	// recreate the projection matrix accordingly
	ViewUniformBufferObject ubo = {};
	ubo.view = game_state->current_camera->view;
	ubo.projection = glm::perspective(glm::radians(45.0f), (float)backend->device->wsi->swapchain_extent.width / (float)backend->device->wsi->swapchain_extent.height, 0.001f, 100.0f);
	ubo.projection[1][1] *= -1;
	ubo.camera_position = game_state->current_camera->position;

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
			// X-Axis
			glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
			glm::vec3(3.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),

			// Z-Axis
			glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
			glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),

			// Y-Axis
			glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
		};

		vkMapMemory(backend->device->context->device, frames[i].debug_vertex_buffer->device_memory, 0, sizeof(DebugLine) * ARRAYSIZE(debug_lines), 0, &frames[i].debug_vertex_buffer->data);
		memcpy(frames[i].debug_vertex_buffer->data, &debug_lines, sizeof(DebugLine) * ARRAYSIZE(debug_lines));
		vkUnmapMemory(backend->device->context->device, frames[i].debug_vertex_buffer->device_memory);
	}
}

void Renderer::cleanup()
{
	vkQueueWaitIdle(backend->device->context->graphics_queue);

	destroy_ubo_buffers();
	destroy_descriptor_pool();

	// TODO: Move this its onw cleanup function
	for (uint32_t i = 0; i < Vulkan::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		frames[i].debug_vertex_buffer->destroy(backend->device->context->device);
	}

	for (uint32_t i = 0; i < descriptor_set_layout_count; ++i)
	{
		vkDestroyDescriptorSetLayout(backend->device->context->device, descriptor_set_layouts[i], nullptr);
		descriptor_set_layouts[i] = VK_NULL_HANDLE;
	}

	vkDestroyPipeline(backend->device->context->device, static_pipeline.pipeline, nullptr);
	static_pipeline.pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(backend->device->context->device, static_pipeline.pipeline_layout, nullptr);
	static_pipeline.pipeline_layout = VK_NULL_HANDLE;

	vkDestroyPipeline(backend->device->context->device, dynamic_pipeline.pipeline, nullptr);
	dynamic_pipeline.pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(backend->device->context->device, dynamic_pipeline.pipeline_layout, nullptr);
	dynamic_pipeline.pipeline_layout = VK_NULL_HANDLE;

	vkDestroyPipeline(backend->device->context->device, debug_pipeline.pipeline, nullptr);
	debug_pipeline.pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(backend->device->context->device, debug_pipeline.pipeline_layout, nullptr);
	debug_pipeline.pipeline_layout = VK_NULL_HANDLE;

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


	// Pipeline 1: Graphics pipeline for dynamic entities
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
	dynamic_vertex_binding_descriptions[0].stride = sizeof(Resources::Vertex);
	dynamic_vertex_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	dynamic_vertex_input_ci.vertexBindingDescriptionCount = ARRAYSIZE(dynamic_vertex_binding_descriptions);
	dynamic_vertex_input_ci.pVertexBindingDescriptions = dynamic_vertex_binding_descriptions;

	// An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data 
	// originating from a binding description. We have 4 attributes, position, normal, and tex_coord_0
	// so we need 3 attribute description structs
	VkVertexInputAttributeDescription dynamic_vertex_input_attribute_descriptions[3];
	// Per-vertex attributes
	// Position
	dynamic_vertex_input_attribute_descriptions[0].binding = 0;
	dynamic_vertex_input_attribute_descriptions[0].location = 0;
	dynamic_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	dynamic_vertex_input_attribute_descriptions[0].offset = offsetof(Resources::Vertex, position);
	// Normal
	dynamic_vertex_input_attribute_descriptions[1].binding = 0;
	dynamic_vertex_input_attribute_descriptions[1].location = 1;
	dynamic_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	dynamic_vertex_input_attribute_descriptions[1].offset = offsetof(Resources::Vertex, normal);
	// Tex Coord 0
	dynamic_vertex_input_attribute_descriptions[2].binding = 0;
	dynamic_vertex_input_attribute_descriptions[2].location = 2;
	dynamic_vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	dynamic_vertex_input_attribute_descriptions[2].offset = offsetof(Resources::Vertex, tex_coord_0);

	dynamic_vertex_input_ci.vertexAttributeDescriptionCount = ARRAYSIZE(dynamic_vertex_input_attribute_descriptions);
	dynamic_vertex_input_ci.pVertexAttributeDescriptions = dynamic_vertex_input_attribute_descriptions;

	VkPushConstantRange player_matrix = {};
	player_matrix.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	player_matrix.size = sizeof(glm::mat4);
	player_matrix.offset = 0;

	VkPushConstantRange material_params = {};
	material_params.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	material_params.size = sizeof(Resources::Material::PBRMetallicRoughness);
	material_params.offset = sizeof(glm::mat4);

	VkPushConstantRange push_constant_ranges[] = { player_matrix, material_params };

	// View Descriptor Set Layout
	// TODO: Add more info about these DescriptorSet Layout Bindings
	VkDescriptorSetLayoutBinding view_descriptor_set_layout_bindings[1];
	// View Uniform Buffer Object: Camera info (view, projection, position)
	view_descriptor_set_layout_bindings[0] = {};
	view_descriptor_set_layout_bindings[0].binding = 0;
	view_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	view_descriptor_set_layout_bindings[0].descriptorCount = 1;
	view_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	view_descriptor_set_layout_bindings[0].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo view_descriptor_set_layout_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	view_descriptor_set_layout_ci.bindingCount = ARRAYSIZE(view_descriptor_set_layout_bindings);
	view_descriptor_set_layout_ci.pBindings = view_descriptor_set_layout_bindings;

	VkResult result = vkCreateDescriptorSetLayout(backend->device->context->device, &view_descriptor_set_layout_ci, nullptr, &descriptor_set_layouts[descriptor_set_layout_offset++]);
	assert(result == VK_SUCCESS);
	descriptor_set_layout_count++;

	// Node Descriptor Set
	VkDescriptorSetLayoutBinding node_descriptor_set_layout_bindings[1];
	// Node Uniform Buffer Object: translation, scale, rotation
	node_descriptor_set_layout_bindings[0] = {};
	node_descriptor_set_layout_bindings[0].binding = 0;
	node_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	node_descriptor_set_layout_bindings[0].descriptorCount = 1;
	node_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	node_descriptor_set_layout_bindings[0].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo node_descriptor_set_layout_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	node_descriptor_set_layout_ci.bindingCount = ARRAYSIZE(node_descriptor_set_layout_bindings);
	node_descriptor_set_layout_ci.pBindings = node_descriptor_set_layout_bindings;

	result = vkCreateDescriptorSetLayout(backend->device->context->device, &node_descriptor_set_layout_ci, nullptr, &descriptor_set_layouts[descriptor_set_layout_offset++]);
	assert(result == VK_SUCCESS);
	descriptor_set_layout_count++;

	dynamic_pipeline = create_pipeline("../data/shaders/dynamic_entity.vert.spv", "../data/shaders/dynamic_entity.frag.spv", dynamic_vertex_input_ci, input_assembly_ci, descriptor_set_layout_count, descriptor_set_layouts,
		viewport_ci, rasterizer_ci, depth_stencil_ci, multisampling_ci, color_blend_ci, dynamic_state_ci, ARRAYSIZE(push_constant_ranges), push_constant_ranges);

	// Pipeline 2: Debug Draw pipeline for debug gizmos
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

	input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

	// Depth Buffer
	VkPipelineDepthStencilStateCreateInfo debug_depth_stencil_ci = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	debug_depth_stencil_ci.depthTestEnable = VK_FALSE;
	debug_depth_stencil_ci.depthWriteEnable = VK_TRUE;
	debug_depth_stencil_ci.depthCompareOp = VK_COMPARE_OP_LESS;
	debug_depth_stencil_ci.depthBoundsTestEnable = VK_FALSE;
	debug_depth_stencil_ci.stencilTestEnable = VK_FALSE;

	debug_pipeline = create_pipeline("../data/shaders/debug_draw.vert.spv", "../data/shaders/debug_draw.frag.spv", debug_vertex_input_ci, input_assembly_ci, 1, descriptor_set_layouts,
		viewport_ci, rasterizer_ci, debug_depth_stencil_ci, multisampling_ci, color_blend_ci, dynamic_state_ci, 0, nullptr);
}

Pipeline Renderer::create_pipeline(const char* vertex_shader_path, const char* fragment_shader_path, VkPipelineVertexInputStateCreateInfo vertex_input_ci, VkPipelineInputAssemblyStateCreateInfo input_assembly_ci,
								   uint32_t descriptor_set_layout_count, VkDescriptorSetLayout* descriptor_set_layouts, VkPipelineViewportStateCreateInfo viewport_ci, VkPipelineRasterizationStateCreateInfo rasterizer_ci,
								   VkPipelineDepthStencilStateCreateInfo depth_stencil_ci, VkPipelineMultisampleStateCreateInfo multisampling_ci, VkPipelineColorBlendStateCreateInfo color_blend_ci,
								   VkPipelineDynamicStateCreateInfo dynamic_state_ci, uint32_t push_constant_range_count, VkPushConstantRange* push_constant_ranges)
{
	Pipeline graphics_pipeline = {};

	VkPipelineShaderStageCreateInfo shader_stages[2];
	shader_stages[0] = Vulkan::Shader::load_shader(backend->device->context->device, backend->device->context->gpu, vertex_shader_path, VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = Vulkan::Shader::load_shader(backend->device->context->device, backend->device->context->gpu, fragment_shader_path, VK_SHADER_STAGE_FRAGMENT_BIT);

	// Pipeline Layout
	VkPipelineLayoutCreateInfo pipeline_layout_ci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipeline_layout_ci.setLayoutCount = descriptor_set_layout_count;
	pipeline_layout_ci.pSetLayouts = descriptor_set_layouts;

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
	pool_sizes[0].descriptorCount = 3;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	pool_sizes[1].descriptorCount = 256;

	VkDescriptorPoolCreateInfo pool_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_ci.poolSizeCount = ARRAYSIZE(pool_sizes);
	pool_ci.pPoolSizes = pool_sizes;
	pool_ci.maxSets = 256;

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

		frames[i].view_descriptor_set = VK_NULL_HANDLE;
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
