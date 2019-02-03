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
	void render();

private:
	// WSI
	Vulkan::WSI* wsi;

	VkPipeline graphics_pipeline = VK_NULL_HANDLE;
	VkRenderPass render_pass = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

}; // struct Renderer

} // namespace Renderer
