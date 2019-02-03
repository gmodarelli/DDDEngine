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

	void render();

private:
	// WSI
	Vulkan::WSI* wsi;

	VkPipeline graphics_pipeline = VK_NULL_HANDLE;
	VkRenderPass render_pass = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

	VkFramebuffer* framebuffers = nullptr;
	uint32_t framebuffer_count = 0;
	void destroy_framebuffers();

	VkCommandPool command_pool = VK_NULL_HANDLE;
	VkCommandBuffer* command_buffers = nullptr;
	uint32_t command_buffer_count = 0;
	void free_command_buffers();

}; // struct Renderer

} // namespace Renderer
