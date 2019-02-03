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

}; // struct Renderer

} // namespace Renderer
