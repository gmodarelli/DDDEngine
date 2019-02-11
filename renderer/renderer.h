#pragma once

#include "../vulkan/device.h"
#include "../vulkan/buffer.h"
#include <glm/glm.hpp>

namespace Renderer
{

// const int MAX_FRAMES_IN_FLIGHT = 2;

struct Vertex
{
	glm::vec2 position;
	glm::vec3 color;
};

struct Renderer
{
	Renderer(Vulkan::Device* device);

	void init();
	void cleanup();

	void create_graphics_pipeline();
	void render_frame();

private:
	// Device
	Vulkan::Device* device;

	VkPipeline graphics_pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

	// Scene related stuff?
	uint32_t vertex_count = 0;
	Vulkan::Buffer* vertex_buffer;
	Vertex* vertices;
}; // struct Renderer

} // namespace Renderer
