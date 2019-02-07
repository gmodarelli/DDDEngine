#pragma once

#include "../vulkan/wsi.h"

namespace Renderer
{

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
	void create_semaphores();
	void record_commands();

	void render_frame();

private:
	// WSI
	Vulkan::WSI* wsi;

	VkPipeline graphics_pipeline = VK_NULL_HANDLE;
	VkRenderPass render_pass = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

	VkFramebuffer* framebuffers = nullptr;
	uint32_t framebuffer_count = 0;
	void destroy_framebuffers();

	VkViewport viewport;

	VkCommandPool command_pool = VK_NULL_HANDLE;
	VkCommandBuffer* command_buffers = nullptr;
	uint32_t command_buffer_count = 0;
	void free_command_buffers();

	// Sync objects
	VkSemaphore image_available_semaphore = VK_NULL_HANDLE;
	VkSemaphore render_finished_semaphore = VK_NULL_HANDLE;
	void destroy_semaphores();

}; // struct Renderer

} // namespace Renderer
