#pragma once

#include "../application/platform.h"
#include "../vulkan/backend.h"
#include "../vulkan/buffer.h"
#include "../game/state.h"
#include "../resources/resources.h"

#include "types.h"

namespace Renderer
{

struct Frame
{
	VkDescriptorSet view_descriptor_set;
	VkDescriptorSet imgui_descriptor_set;

	Vulkan::Buffer* view_ubo_buffer = nullptr;
	Vulkan::Buffer* debug_vertex_buffer = nullptr;

	Vulkan::Buffer* imgui_vertex_buffer = nullptr;
	Vulkan::Buffer* imgui_index_buffer = nullptr;
	uint32_t imgui_vertex_count = 0;
	uint32_t imgui_index_count = 0;

	DebugLine debug_lines[1024];
	uint32_t debug_line_count;
};

struct Pipeline
{
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
};

struct Renderer
{
	Renderer(Application::Platform* platform);

	void init();
	void cleanup();

	void upload_buffers(const Game::State* game_state);
	void upload_dynamic_uniform_buffers(const Game::State* game_state);

	void create_pipelines();
	Pipeline create_pipeline(const char* vertex_shader_path, const char* fragment_shader_path, VkPipelineVertexInputStateCreateInfo vertex_input_ci, VkPipelineInputAssemblyStateCreateInfo input_assembly_ci,
		uint32_t descriptor_set_layout_count, VkDescriptorSetLayout* descriptor_set_layouts, VkPipelineViewportStateCreateInfo viewport_ci, VkPipelineRasterizationStateCreateInfo rasterizer_ci,
		VkPipelineDepthStencilStateCreateInfo depth_stencil_ci, VkPipelineMultisampleStateCreateInfo multisampling_ci, VkPipelineColorBlendStateCreateInfo color_blend_ci,
		VkPipelineDynamicStateCreateInfo dynamic_state_ci, uint32_t push_constant_range_count, VkPushConstantRange* push_constant_ranges);

	void render_frame(Game::State* game_state, float delta_time);
	void prepare_uniform_buffers();
	void update_uniform_buffers(Game::State* game_state, Vulkan::FrameResources& frame_resources);

	void prepare_debug_vertex_buffers();

	// Platform
	Application::Platform* platform;
	// Vulkan Backend
	Vulkan::Backend* backend;

	// TODO: Should we store these together with the pipelines?
	size_t dynamic_alignment;
	uint32_t descriptor_set_layout_count = 0;
	uint32_t descriptor_set_layout_offset = 0;
	VkDescriptorSetLayout descriptor_set_layouts[16];

	Pipeline static_pipeline = {};
	Pipeline dynamic_pipeline = {};
	Pipeline debug_pipeline = {};
	Pipeline imgui_pipeline = {};

	// Node Descriptor Sets Resources
	VkDescriptorSet node_descriptor_set;

	Frame frames[Vulkan::MAX_FRAMES_IN_FLIGHT];
	bool wait_on_semaphores = false;

	double frame_cpu_avg = 0;
	double frame_cpu_max = -1.0f;
	double frame_cpu_min = 100.0f;

	const static size_t cpu_frame_buffer_size = 128;
	float cpu_frame_buffer[cpu_frame_buffer_size];
	const static size_t gpu_frame_buffer_size = 128;
	float gpu_frame_buffer[gpu_frame_buffer_size];

	// Descriptor Pool helpers
	VkDescriptorPool descriptor_pool;
	VkResult allocate_descriptor_set(const VkDescriptorSetLayout& descriptor_set_layout, VkDescriptorSet& descriptor_set, uint32_t descriptor_set_count = 1);
	void create_descriptor_pool();
	void destroy_descriptor_pool();

	// UBO Buffer helpers
	void create_ubo_buffers();
	void destroy_ubo_buffers();

	// ImGUI
	Vulkan::Image* imgui_font;
	VkSampler imgui_font_sampler;
	VkDescriptorSetLayout imgui_descriptor_set_layout;
	UIPushConstantBlock imgui_push_const_block;
	void imgui_new_frame(Vulkan::FrameResources& frame_resources, const Game::State* game_state);
	void imgui_update_buffers(Vulkan::FrameResources& frame_resources);
	void imgui_draw_frame(Vulkan::FrameResources& frame_resources);
}; // struct Renderer

} // namespace Renderer
