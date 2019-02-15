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

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

struct Renderer
{
	Renderer(Vulkan::Device* device);

	void init();
	void cleanup();

	void create_graphics_pipeline();
	void render_frame();
	void update_uniform_buffer(Vulkan::FrameResources& frame_resources);

	// Device
	Vulkan::Device* device;

	VkPipeline graphics_pipeline = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

	double frame_cpu_avg = 0;

	// TODO: Merge these two buffers and move them to the Device struct
	Vulkan::Buffer* vertex_buffer;
	Vulkan::Buffer* index_buffer;

	// Scene related stuff?
	uint32_t vertex_count = 0;
	uint32_t index_count = 0;
	Vertex* vertices;
	uint16_t* indices;

}; // struct Renderer

} // namespace Renderer
