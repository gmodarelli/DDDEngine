#pragma once

#include "../vulkan/wsi.h"

namespace Renderer
{

const int MAX_FRAMES_IN_FLIGHT = 2;

struct Renderer
{
	Renderer(Vulkan::WSI* wsi);

	void init();
	void cleanup();

	void create_render_pass();
	void create_graphics_pipeline();
	void create_framebuffers();
	void create_command_pool();
	void create_command_buffers();
	void create_sync_objects();
	void record_commands();

	void render_frame();

private:
	// WSI
	Vulkan::WSI* wsi;

	void recreate_frame_resources();
	void destroy_frame_resources();

	VkRenderPass render_pass = VK_NULL_HANDLE;
	void destroy_renderpass();

	VkPipeline graphics_pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

	VkFramebuffer* framebuffers = nullptr;
	uint32_t framebuffer_count = 0;
	void destroy_framebuffers();

	VkViewport viewport;

	VkCommandPool command_pool = VK_NULL_HANDLE;
	VkCommandBuffer* command_buffers = nullptr;
	uint32_t command_buffer_count = 0;
	void free_command_buffers();
	void destroy_command_pool();

	// Sync objects
	VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
	void destroy_sync_objects();

	size_t current_frame = 0;

	bool is_recreating_frame_resources = false;

}; // struct Renderer

} // namespace Renderer
