#include "renderer.h"
#include "../vulkan/shaders.h"
#include <cassert>
#include <stdio.h>
#include <algorithm>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../application/stb_image.h"
#include "../resources/resources.h"
#include "../renderer/types.h"

#include "../extern/imgui/imgui.h"

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

	for (uint32_t i = 0; i < cpu_frame_buffer_size; ++i)
	{
		cpu_frame_buffer[i] = 0.0f;
	}

	ImGui::CreateContext();

	// ImGUI Initialization
	// Color scheme
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
	// Dimensions
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)backend->wsi->swapchain_extent.width, (float)backend->wsi->swapchain_extent.height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	// Create ImGui Vulkan Resources
	unsigned char* font_data;
	int tex_width, tex_height;
	io.Fonts->GetTexDataAsRGBA32(&font_data, &tex_width, &tex_height);
	VkDeviceSize upload_size = tex_width * tex_height * 4 * sizeof(char);

	imgui_font = new Vulkan::Image(
		backend->device->context->device,
		backend->device->context->gpu,
		{ (uint32_t)tex_width, (uint32_t)tex_height },
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	Vulkan::Buffer* font_staging_buffer = new Vulkan::Buffer(
		backend->device->context->device,
		backend->device->context->gpu,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		upload_size);

	void* mapped;
	vkMapMemory(backend->device->context->device, font_staging_buffer->device_memory, 0, font_staging_buffer->size, 0, &mapped);
	memcpy(mapped, font_data, upload_size);
	vkUnmapMemory(backend->device->context->device, font_staging_buffer->device_memory);

	backend->device->upload_buffer_to_image(font_staging_buffer->buffer, imgui_font->image, VK_FORMAT_R8G8_UNORM, tex_width, tex_height, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	font_staging_buffer->destroy(backend->device->context->device);

	VkSamplerCreateInfo sampler_ci = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	sampler_ci.magFilter = VK_FILTER_LINEAR;
	sampler_ci.minFilter = VK_FILTER_LINEAR;
	sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VkResult result = vkCreateSampler(backend->device->context->device, &sampler_ci, nullptr, &imgui_font_sampler);
	assert(result == VK_SUCCESS);

	// Descriptor Set Layout
	VkDescriptorSetLayoutBinding imgui_set_layout_bindings[1];
	imgui_set_layout_bindings[0] = {};
	imgui_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imgui_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	imgui_set_layout_bindings[0].binding = 0;
	imgui_set_layout_bindings[0].descriptorCount = 1;
	imgui_set_layout_bindings[0].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo set_layout_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	set_layout_ci.bindingCount = ARRAYSIZE(imgui_set_layout_bindings);
	set_layout_ci.pBindings = imgui_set_layout_bindings;
	result = vkCreateDescriptorSetLayout(backend->device->context->device, &set_layout_ci, nullptr, &imgui_descriptor_set_layout);
	assert(result == VK_SUCCESS);

	create_pipelines();
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
		Entity& entity = game_state->entities[e];
		entity.node_offset = absolute_node_count;
		const Resources::Model& model = game_state->assets_info->models[entity.model_id];

		for (uint32_t n = 0; n < model.node_count; ++n)
		{
			// Aligned offset
			glm::mat4* model_matrix = (glm::mat4*)(((uint64_t)dynamic_ubo.model + (absolute_node_count * dynamic_alignment)));

			// NOTE: Temporary solution
			// For dynamic entities we do not apply the game_state->transforms beforehand
			if (e < 3)
			{
				*model_matrix = glm::translate(glm::mat4(1.0f), model.nodes[n].translation);
				*model_matrix = glm::scale(*model_matrix, model.nodes[n].scale);
				*model_matrix = *model_matrix * glm::toMat4(model.nodes[n].rotation);
			}
			else
			{
				*model_matrix = glm::translate(glm::mat4(1.0f), model.nodes[n].translation);
				*model_matrix = glm::translate(*model_matrix, game_state->transforms[e].position);
				*model_matrix = glm::scale(*model_matrix, model.nodes[n].scale);
				*model_matrix = glm::scale(*model_matrix, game_state->transforms[e].scale);
				*model_matrix = *model_matrix * glm::toMat4(model.nodes[n].rotation) * glm::toMat4(game_state->transforms[e].rotation);
			}

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
	static uint32_t frame_index = 0;
	static uint32_t sampling_index = 0;

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

		result = allocate_descriptor_set(imgui_descriptor_set_layout, frame->imgui_descriptor_set);
		assert(result == VK_SUCCESS);

		VkDescriptorImageInfo font_info = {};
		font_info.sampler = imgui_font_sampler;
		font_info.imageView = imgui_font->image_view;
		font_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet font_descriptor_writes[1];
		font_descriptor_writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		font_descriptor_writes[0].dstSet = frame->imgui_descriptor_set;
		font_descriptor_writes[0].dstBinding = 0;
		font_descriptor_writes[0].dstArrayElement = 0;
		font_descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		font_descriptor_writes[0].descriptorCount = 1;
		font_descriptor_writes[0].pImageInfo = &font_info;

		vkUpdateDescriptorSets(backend->device->context->device, ARRAYSIZE(font_descriptor_writes), font_descriptor_writes, 0, nullptr);
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

	for (uint32_t i = game_state->player_head_id; i < game_state->player_body_part_count; ++i)
	{
		glm::mat4 body_model = glm::mat4(1.0f);
		Game::State::BodyPart& body = game_state->body_parts[i];
		glm::vec3 view_position = body.position + (body.direction * game_state->player_speed * delta_time);
		body_model = glm::translate(body_model, view_position);
		body_model *= body.orientation;

		game_state->player_matrices[i] = body_model;
	}

	for (uint32_t x = game_state->player_head_id; x < game_state->player_body_part_count; ++x)
	{
		vkCmdPushConstants(frame_resources.command_buffer, dynamic_pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &game_state->player_matrices[x]);

		// Render the player
		Entity& entity = game_state->entities[x];
		Resources::Model model = game_state->assets_info->models[entity.model_id];
		for (uint32_t n_id = 0; n_id < model.node_count; ++n_id)
		{
			uint32_t dynamic_offset = (entity.node_offset + n_id) * static_cast<uint32_t>(dynamic_alignment);
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
	}

	vkCmdBindPipeline(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_pipeline.pipeline);

	vkCmdBindDescriptorSets(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_pipeline.pipeline_layout, 0, 1, &frame->view_descriptor_set, 0, nullptr);

	vkCmdSetViewport(frame_resources.command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(frame_resources.command_buffer, 0, 1, &scissor);

	// Render all other entities
	// TODO: Move static entities to the top of the entities table, since they are static and won't grow
	// during a level
	for (uint32_t e_id = game_state->player_tail_id + 2; e_id < game_state->entity_count; ++e_id)
	{
		Entity& entity = game_state->entities[e_id];
		Resources::Model model = game_state->assets_info->models[entity.model_id];
		for (uint32_t n_id = 0; n_id < model.node_count; ++n_id)
		{
			uint32_t dynamic_offset = (entity.node_offset + n_id) * static_cast<uint32_t>(dynamic_alignment);
			Resources::Node node = model.nodes[n_id];
			vkCmdBindDescriptorSets(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_pipeline.pipeline_layout, 1, 1, &node_descriptor_set, 1, &dynamic_offset);
			Resources::Mesh mesh = game_state->assets_info->meshes[node.mesh_id];
			for (uint32_t p_id = 0; p_id < mesh.primitive_count; ++p_id)
			{
				Resources::Primitive primitive = mesh.primitives[p_id];
				Resources::Material material = game_state->assets_info->materials[primitive.material_id];
				vkCmdPushConstants(frame_resources.command_buffer, static_pipeline.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Resources::Material::PBRMetallicRoughness), &material.pbr_metallic_roughness);
				vkCmdDrawIndexed(frame_resources.command_buffer, primitive.index_count, 1, primitive.index_offset, 0, 0);
			}
		}
	}

	// Debug Draw
	if (game_state->show_grid)
	{
		vkCmdBindPipeline(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_pipeline.pipeline);

		vkCmdSetViewport(frame_resources.command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(frame_resources.command_buffer, 0, 1, &scissor);
		vkCmdSetLineWidth(frame_resources.command_buffer, 1.0f);
		vkCmdBindVertexBuffers(frame_resources.command_buffer, 0, 1, &frame->debug_vertex_buffer->buffer, offsets);
		vkCmdDraw(frame_resources.command_buffer, frame->debug_line_count, 1, 0, 0);
	}

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)backend->wsi->swapchain_extent.width, (float)backend->wsi->swapchain_extent.height);
	io.DeltaTime = delta_time;

	imgui_new_frame(frame_resources, game_state);
	imgui_update_buffers(frame_resources);
	imgui_draw_frame(frame_resources);

	backend->device->end_draw_frame(frame_resources);

	auto frame_cpu_end = std::chrono::high_resolution_clock::now();
	auto frame_time = (frame_cpu_end - frame_cpu_start).count();
	frame_cpu_avg = frame_cpu_avg * 0.95 +  frame_time * 1e-6 * 0.05;

	if (frame_cpu_avg < frame_cpu_min)
	{
		frame_cpu_min = frame_cpu_avg;
	}
	else if (frame_cpu_avg > frame_cpu_max)
	{
		frame_cpu_max = frame_cpu_avg;
	}

	if (sampling_index % 30 == 0)
	{
		cpu_frame_buffer[frame_index] = (float)frame_cpu_avg;
		gpu_frame_buffer[frame_index] = (float)backend->device->frame_gpu_avg;

		frame_index = (frame_index + 1) % gpu_frame_buffer_size;
		sampling_index = 0;
	}

	sampling_index++;
}

// ImGUI-Specific
void Renderer::imgui_new_frame(Vulkan::FrameResources& frame_resources, const Game::State* game_state)
{
	ImGui::NewFrame();
	ImGui::Begin("Stats");
	ImGui::SetWindowPos({ 0, 0 });
	ImGui::SetWindowSize({ 500, 1000 });

	// Init ImGui windows and elements
	ImVec4 clear_color = ImColor(144, 144, 154);

	char cpu_title[64];
	sprintf(cpu_title, "CPU - min: %.4fms max: %.4fms avg: %.4fms", frame_cpu_min, frame_cpu_max, frame_cpu_avg);
	ImGui::PlotLines("", cpu_frame_buffer, cpu_frame_buffer_size, 0, cpu_title, 0.0f, 2.0f, ImVec2(400.0f, 100.0f));
	char gpu_title[64];
	sprintf(gpu_title, "GPU - min: %.4fms max: %.4fms avg: %.4fms", backend->device->frame_gpu_min, backend->device->frame_gpu_max, backend->device->frame_gpu_avg);
	ImGui::PlotLines("", gpu_frame_buffer, gpu_frame_buffer_size, 0, gpu_title, 0.0f, 2.0f, ImVec2(400.0f, 100.0f));

	float camera_position[] = {
		game_state->current_camera->position.x,
		game_state->current_camera->position.y,
		game_state->current_camera->position.z,
	};

	if (game_state->current_camera->type == CameraType::LookAt)
	{
		ImGui::TextUnformatted("Game Camera");
	}
	else
	{
		ImGui::TextUnformatted("Debug Camera");
	}

	ImGui::InputFloat3("Camera Position", camera_position, 4);

	char move_count[16];
	sprintf(move_count, "Move count: %d", game_state->player_move_count);
	ImGui::TextUnformatted(move_count);

	ImGui::End();

	// Render to generate draw buffers
	ImGui::Render();
}

void Renderer::imgui_update_buffers(Vulkan::FrameResources& frame_resources)
{
	Frame* frame = (Frame*) frame_resources.custom;
	assert(frame);

	ImDrawData* im_draw_data = ImGui::GetDrawData();

	VkDeviceSize vertex_buffer_size = im_draw_data->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize index_buffer_size = im_draw_data->TotalIdxCount * sizeof(ImDrawIdx);

	if (vertex_buffer_size == 0 || index_buffer_size == 0)
	{
		return;
	}

	// Update buffers only if vertex or index count has changed
	// Vertex buffer
	if (!frame->imgui_vertex_buffer || frame->imgui_vertex_buffer->buffer == VK_NULL_HANDLE || frame->imgui_vertex_count != im_draw_data->TotalVtxCount)
	{
		if (frame->imgui_vertex_buffer)
		{
			frame->imgui_vertex_buffer->unmap(backend->device->context->device);
			frame->imgui_vertex_buffer->destroy(backend->device->context->device);
		}

		frame->imgui_vertex_buffer = new Vulkan::Buffer(
			backend->device->context->device,
			backend->device->context->gpu,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			vertex_buffer_size,
			VK_SHARING_MODE_EXCLUSIVE,
			true
		);

		frame->imgui_vertex_count = im_draw_data->TotalVtxCount;
		frame->imgui_vertex_buffer->unmap(backend->device->context->device);
		frame->imgui_vertex_buffer->map(backend->device->context->device);
	}

	// Index buffer
	if (!frame->imgui_index_buffer || frame->imgui_index_buffer->buffer == VK_NULL_HANDLE || frame->imgui_index_count != im_draw_data->TotalVtxCount)
	{
		if (frame->imgui_index_buffer)
		{
			frame->imgui_index_buffer->unmap(backend->device->context->device);
			frame->imgui_index_buffer->destroy(backend->device->context->device);
		}

		frame->imgui_index_buffer = new Vulkan::Buffer(
			backend->device->context->device,
			backend->device->context->gpu,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			index_buffer_size,
			VK_SHARING_MODE_EXCLUSIVE,
			true
		);

		frame->imgui_index_count = im_draw_data->TotalVtxCount;
		frame->imgui_index_buffer->unmap(backend->device->context->device);
		frame->imgui_index_buffer->map(backend->device->context->device);
	}

	// Upload data
	ImDrawVert* vtx_dst = (ImDrawVert*)frame->imgui_vertex_buffer->mapped;
	ImDrawIdx* idx_dst = (ImDrawIdx*)frame->imgui_index_buffer->mapped;

	for (int n = 0; n < im_draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = im_draw_data->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	frame->imgui_vertex_buffer->flush(backend->device->context->device);
	frame->imgui_index_buffer->flush(backend->device->context->device);
}

void Renderer::imgui_draw_frame(Vulkan::FrameResources& frame_resources)
{
	Frame* frame = (Frame*) frame_resources.custom;
	assert(frame);

	ImGuiIO& io = ImGui::GetIO();

	vkCmdBindDescriptorSets(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, imgui_pipeline.pipeline_layout, 0, 1, &frame->imgui_descriptor_set, 0, nullptr);
	vkCmdBindPipeline(frame_resources.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, imgui_pipeline.pipeline);

	VkViewport viewport = {};
	viewport.width = io.DisplaySize.x;
	viewport.height = io.DisplaySize.y;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(frame_resources.command_buffer, 0, 1, &viewport);

	// UI Translate and Scale via push commands
	imgui_push_const_block.translate = glm::vec2(-1.0f);
	imgui_push_const_block.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
	vkCmdPushConstants(frame_resources.command_buffer, imgui_pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UIPushConstantBlock), &imgui_push_const_block);

	// Render commands
	ImDrawData* im_draw_data = ImGui::GetDrawData();
	int32_t vertex_offset = 0;
	int32_t index_offset = 0;

	if (im_draw_data->CmdListsCount > 0)
	{
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(frame_resources.command_buffer, 0, 1, &frame->imgui_vertex_buffer->buffer, offsets);
		vkCmdBindIndexBuffer(frame_resources.command_buffer, frame->imgui_index_buffer->buffer, 0, VK_INDEX_TYPE_UINT16);

		for (int32_t i = 0; i < im_draw_data->CmdListsCount; i++)
		{
			const ImDrawList* cmd_list = im_draw_data->CmdLists[i];
			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
				VkRect2D scissorRect;
				scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
				scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
				scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
				scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
				vkCmdSetScissor(frame_resources.command_buffer, 0, 1, &scissorRect);
				vkCmdDrawIndexed(frame_resources.command_buffer, pcmd->ElemCount, 1, index_offset, vertex_offset, 0);
				index_offset += pcmd->ElemCount;
			}
			vertex_offset += cmd_list->VtxBuffer.Size;
		}
	}
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
		ubo.projection = glm::perspective(glm::radians(45.0f), (float)backend->device->wsi->swapchain_extent.width / (float)backend->device->wsi->swapchain_extent.height, 0.001f, 1000.0f);
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
	ubo.projection = glm::perspective(glm::radians(45.0f), (float)backend->device->wsi->swapchain_extent.width / (float)backend->device->wsi->swapchain_extent.height, 0.001f, 1000.0f);
	ubo.projection[1][1] *= -1;
	ubo.camera_position = game_state->current_camera->position;

	void* data;
	vkMapMemory(backend->device->context->device, frame->view_ubo_buffer->device_memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(backend->device->context->device, frame->view_ubo_buffer->device_memory);
}

void Renderer::prepare_debug_vertex_buffers()
{
	glm::vec4 line_color_dark = glm::vec4(0.4f, 0.4f, 0.4f, 0.8f);
	glm::vec4 line_color_light = glm::vec4(0.6f, 0.6f, 0.6f, 0.5f);
	glm::vec4 red = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	glm::vec4 green = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
	glm::vec4 blue = glm::vec4(-.0f, 0.0f, 1.0f, 1.0f);

	float line_space = 0.6f;

	for (uint32_t i = 0; i < ARRAYSIZE(frames); ++i)
	{
		frames[i].debug_line_count = 0;

		for (int32_t d = -10; d < 10; ++d)
		{
			frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(-20.0f, 0.0f, d * line_space + (line_space * 0.5f)), line_color_dark };
			frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(20.0f, 0.0f, d * line_space + (line_space * 0.5f)), line_color_dark };
			frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(d * line_space + (line_space * 0.5f), 0.0f, -20.0f), line_color_dark };
			frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(d * line_space + (line_space * 0.5f), 0.0f, 20.0f), line_color_dark };

			/*
			frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(-20.0f, 0.0f, d * line_space * 0.5f), line_color_light };
			frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(20.0f, 0.0f, d * line_space * 0.5f), line_color_light };
			frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(d * line_space * 0.5f, 0.0f, -20.0f), line_color_light };
			frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(d * line_space * 0.5f, 0.0f, 20.0f), line_color_light };
			*/
		}

		// Origin
		// X-Axis
		frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(0.0f, 0.0f, 0.0f), red };
		frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(3.0f, 0.0f, 0.0f), red };

		// Z-Axis
		frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(0.0f, 0.0f, 0.0f), blue };
		frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(0.0f, 0.0f, 3.0f), blue };

		// Y-Axis
		frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(0.0f, 0.0f, 0.0f), green };
		frames[i].debug_lines[frames[i].debug_line_count++] = { glm::vec3(0.0f, 3.0f, 0.0f), green };

		vkMapMemory(backend->device->context->device, frames[i].debug_vertex_buffer->device_memory, 0, sizeof(DebugLine) * frames[i].debug_line_count, 0, &frames[i].debug_vertex_buffer->mapped);
		memcpy(frames[i].debug_vertex_buffer->mapped, &frames[i].debug_lines, sizeof(DebugLine) * frames[i].debug_line_count);
		vkUnmapMemory(backend->device->context->device, frames[i].debug_vertex_buffer->device_memory);
	}
}

void Renderer::cleanup()
{
	vkQueueWaitIdle(backend->device->context->graphics_queue);

	ImGui::DestroyContext();

	vkDestroySampler(backend->device->context->device, imgui_font_sampler, nullptr);
	imgui_font->destroy(backend->device->context->device);

	destroy_ubo_buffers();
	destroy_descriptor_pool();

	vkDestroyDescriptorSetLayout(backend->device->context->device, imgui_descriptor_set_layout, nullptr);

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

	vkDestroyPipeline(backend->device->context->device, imgui_pipeline.pipeline, nullptr);
	imgui_pipeline.pipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(backend->device->context->device, imgui_pipeline.pipeline_layout, nullptr);
	imgui_pipeline.pipeline_layout = VK_NULL_HANDLE;

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

	// Pipeline Fixed Functions
	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertex_input_ci = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	// A vertex binding describes at which rate to load data from memory throughout the vertices. 
	// It specifies the number of bytes between data entries and whether to move to the next 
	// data entry after each vertex or after each instance.
	VkVertexInputBindingDescription vertex_binding_descriptions[1];
	// Mesh vertex bindings
	vertex_binding_descriptions[0] = {};
	vertex_binding_descriptions[0].binding = 0;
	vertex_binding_descriptions[0].stride = sizeof(Resources::Vertex);
	vertex_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertex_input_ci.vertexBindingDescriptionCount = ARRAYSIZE(vertex_binding_descriptions);
	vertex_input_ci.pVertexBindingDescriptions = vertex_binding_descriptions;

	// An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data 
	// originating from a binding description. We have 4 attributes, position, normal, and tex_coord_0
	// so we need 3 attribute description structs
	VkVertexInputAttributeDescription vertex_input_attribute_descriptions[3];
	// Per-vertex attributes
	// Position
	vertex_input_attribute_descriptions[0].binding = 0;
	vertex_input_attribute_descriptions[0].location = 0;
	vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attribute_descriptions[0].offset = offsetof(Resources::Vertex, position);
	// Normal
	vertex_input_attribute_descriptions[1].binding = 0;
	vertex_input_attribute_descriptions[1].location = 1;
	vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attribute_descriptions[1].offset = offsetof(Resources::Vertex, normal);
	// Tex Coord 0
	vertex_input_attribute_descriptions[2].binding = 0;
	vertex_input_attribute_descriptions[2].location = 2;
	vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_input_attribute_descriptions[2].offset = offsetof(Resources::Vertex, tex_coord_0);

	vertex_input_ci.vertexAttributeDescriptionCount = ARRAYSIZE(vertex_input_attribute_descriptions);
	vertex_input_ci.pVertexAttributeDescriptions = vertex_input_attribute_descriptions;

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

	// Pipeline 1: Graphics pipeline for dynamic entities
	VkPushConstantRange player_matrix = {};
	player_matrix.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	player_matrix.size = sizeof(glm::mat4);
	player_matrix.offset = 0;

	VkPushConstantRange material_params = {};
	material_params.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	material_params.size = sizeof(Resources::Material::PBRMetallicRoughness);
	material_params.offset = sizeof(glm::mat4);

	VkPushConstantRange dynamic_pipeline_push_constant_ranges[] = { player_matrix, material_params };

	dynamic_pipeline = create_pipeline("../data/shaders/dynamic_entity.vert.spv", "../data/shaders/dynamic_entity.frag.spv", vertex_input_ci, input_assembly_ci, descriptor_set_layout_count, descriptor_set_layouts,
		viewport_ci, rasterizer_ci, depth_stencil_ci, multisampling_ci, color_blend_ci, dynamic_state_ci, ARRAYSIZE(dynamic_pipeline_push_constant_ranges), dynamic_pipeline_push_constant_ranges);

	// Pipeline 2: Graphics pipeline for static entities
	// VkPushConstantRange material_params = {};
	// material_params.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	// material_params.size = sizeof(Resources::Material::PBRMetallicRoughness);
	material_params.offset = 0;

	VkPushConstantRange static_pipeline_push_constant_ranges[] = { material_params };

	static_pipeline = create_pipeline("../data/shaders/static_entity.vert.spv", "../data/shaders/static_entity.frag.spv", vertex_input_ci, input_assembly_ci, descriptor_set_layout_count, descriptor_set_layouts,
		viewport_ci, rasterizer_ci, depth_stencil_ci, multisampling_ci, color_blend_ci, dynamic_state_ci, ARRAYSIZE(static_pipeline_push_constant_ranges), static_pipeline_push_constant_ranges);

	// Pipeline 3: Graphics pipeline for ImGui
	// Rasterizer
	VkPipelineRasterizationStateCreateInfo imgui_rasterizer_ci = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	imgui_rasterizer_ci.polygonMode = VK_POLYGON_MODE_FILL;
	imgui_rasterizer_ci.lineWidth = 1.0f;
	imgui_rasterizer_ci.cullMode = VK_CULL_MODE_NONE;
	imgui_rasterizer_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	// Color Blend
	VkPipelineColorBlendAttachmentState imgui_color_blend_attachment_ci = {};
	imgui_color_blend_attachment_ci.blendEnable = VK_TRUE;
	imgui_color_blend_attachment_ci.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	imgui_color_blend_attachment_ci.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	imgui_color_blend_attachment_ci.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	imgui_color_blend_attachment_ci.colorBlendOp = VK_BLEND_OP_ADD;
	imgui_color_blend_attachment_ci.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	imgui_color_blend_attachment_ci.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	imgui_color_blend_attachment_ci.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo imgui_color_blend_ci = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	imgui_color_blend_ci.logicOpEnable = VK_FALSE;
	imgui_color_blend_ci.attachmentCount = 1;
	imgui_color_blend_ci.pAttachments = &imgui_color_blend_attachment_ci;

	// Depth Buffer
	VkPipelineDepthStencilStateCreateInfo imgui_depth_stencil_ci = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	imgui_depth_stencil_ci.depthTestEnable = VK_FALSE;
	imgui_depth_stencil_ci.depthWriteEnable = VK_FALSE;
	imgui_depth_stencil_ci.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	imgui_depth_stencil_ci.depthBoundsTestEnable = VK_FALSE;
	imgui_depth_stencil_ci.stencilTestEnable = VK_FALSE;

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo imgui_vertex_input_ci = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	// A vertex binding describes at which rate to load data from memory throughout the vertices. 
	// It specifies the number of bytes between data entries and whether to move to the next 
	// data entry after each vertex or after each instance.
	VkVertexInputBindingDescription imgui_vertex_binding_descriptions[1];
	// Mesh vertex bindings
	imgui_vertex_binding_descriptions[0] = {};
	imgui_vertex_binding_descriptions[0].binding = 0;
	imgui_vertex_binding_descriptions[0].stride = sizeof(ImDrawVert);
	imgui_vertex_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	imgui_vertex_input_ci.vertexBindingDescriptionCount = ARRAYSIZE(imgui_vertex_binding_descriptions);
	imgui_vertex_input_ci.pVertexBindingDescriptions = imgui_vertex_binding_descriptions;

	// An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data 
	// originating from a binding description. We have 3 attributes, position, uv and color
	// so we need 3 attribute description structs
	VkVertexInputAttributeDescription imgui_vertex_input_attribute_descriptions[3];
	// Per-vertex attributes
	// Position
	imgui_vertex_input_attribute_descriptions[0].binding = 0;
	imgui_vertex_input_attribute_descriptions[0].location = 0;
	imgui_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	imgui_vertex_input_attribute_descriptions[0].offset = offsetof(ImDrawVert, pos);
	// UV
	imgui_vertex_input_attribute_descriptions[1].binding = 0;
	imgui_vertex_input_attribute_descriptions[1].location = 1;
	imgui_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	imgui_vertex_input_attribute_descriptions[1].offset = offsetof(ImDrawVert, uv);
	// Color
	imgui_vertex_input_attribute_descriptions[2].binding = 0;
	imgui_vertex_input_attribute_descriptions[2].location = 2;
	imgui_vertex_input_attribute_descriptions[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	imgui_vertex_input_attribute_descriptions[2].offset = offsetof(ImDrawVert, col);

	imgui_vertex_input_ci.vertexAttributeDescriptionCount = ARRAYSIZE(imgui_vertex_input_attribute_descriptions);
	imgui_vertex_input_ci.pVertexAttributeDescriptions = imgui_vertex_input_attribute_descriptions;

	VkPushConstantRange ui_params = {};
	ui_params.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ui_params.size = sizeof(UIPushConstantBlock);
	ui_params.offset = 0;

	VkPushConstantRange imgui_pipeline_push_constant_ranges[] = { ui_params };

	imgui_pipeline = create_pipeline("../data/shaders/ui.vert.spv", "../data/shaders/ui.frag.spv", imgui_vertex_input_ci, input_assembly_ci, 1, &imgui_descriptor_set_layout,
		viewport_ci, imgui_rasterizer_ci, imgui_depth_stencil_ci, multisampling_ci, imgui_color_blend_ci, dynamic_state_ci, ARRAYSIZE(imgui_pipeline_push_constant_ranges), imgui_pipeline_push_constant_ranges);

	// Pipeline 4: Debug Draw pipeline for debug gizmos
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
	// originating from a binding description. We have 3 attributes, position, and color
	// so we need 2 attribute description structs
	VkVertexInputAttributeDescription debug_vertex_input_attribute_descriptions[2];
	// Per-vertex attributes
	// Position
	debug_vertex_input_attribute_descriptions[0].binding = 0;
	debug_vertex_input_attribute_descriptions[0].location = 0;
	debug_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	debug_vertex_input_attribute_descriptions[0].offset = offsetof(DebugLine, position);
	// Color
	debug_vertex_input_attribute_descriptions[1].binding = 0;
	debug_vertex_input_attribute_descriptions[1].location = 1;
	debug_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	debug_vertex_input_attribute_descriptions[1].offset = offsetof(DebugLine, color);

	debug_vertex_input_ci.vertexAttributeDescriptionCount = ARRAYSIZE(debug_vertex_input_attribute_descriptions);
	debug_vertex_input_ci.pVertexAttributeDescriptions = debug_vertex_input_attribute_descriptions;

	input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

	// Color Blend
	VkPipelineColorBlendAttachmentState debug_color_blend_attachment_ci = {};
	debug_color_blend_attachment_ci.blendEnable = VK_TRUE;
	debug_color_blend_attachment_ci.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	debug_color_blend_attachment_ci.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	debug_color_blend_attachment_ci.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	debug_color_blend_attachment_ci.colorBlendOp = VK_BLEND_OP_ADD;
	debug_color_blend_attachment_ci.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	debug_color_blend_attachment_ci.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	debug_color_blend_attachment_ci.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo debug_color_blend_ci = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	debug_color_blend_ci.logicOpEnable = VK_FALSE;
	debug_color_blend_ci.attachmentCount = 1;
	debug_color_blend_ci.pAttachments = &debug_color_blend_attachment_ci;

	// Depth Buffer
	VkPipelineDepthStencilStateCreateInfo debug_depth_stencil_ci = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	debug_depth_stencil_ci.depthTestEnable = VK_FALSE;
	debug_depth_stencil_ci.depthWriteEnable = VK_TRUE;
	debug_depth_stencil_ci.depthCompareOp = VK_COMPARE_OP_LESS;
	debug_depth_stencil_ci.depthBoundsTestEnable = VK_FALSE;
	debug_depth_stencil_ci.stencilTestEnable = VK_FALSE;

	debug_pipeline = create_pipeline("../data/shaders/debug_draw.vert.spv", "../data/shaders/debug_draw.frag.spv", debug_vertex_input_ci, input_assembly_ci, 1, descriptor_set_layouts,
		viewport_ci, rasterizer_ci, debug_depth_stencil_ci, multisampling_ci, debug_color_blend_ci, dynamic_state_ci, 0, nullptr);
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
		frames[i].imgui_descriptor_set = VK_NULL_HANDLE;
	}
}

void Renderer::destroy_ubo_buffers()
{
	for (uint32_t i = 0; i < Vulkan::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		frames[i].view_ubo_buffer->destroy(backend->device->context->device);
		frames[i].imgui_vertex_buffer->destroy(backend->device->context->device);
		frames[i].imgui_index_buffer->destroy(backend->device->context->device);
	}
}

} // namespace Renderer
