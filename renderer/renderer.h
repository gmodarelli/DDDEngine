#pragma once

#include "../vulkan/device.h"
#include "../vulkan/buffer.h"
#include <glm/glm.hpp>

namespace Renderer
{

struct Mesh
{
	uint32_t index_offset;
	uint32_t index_count;
	uint32_t vertex_offset;
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
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

	uint32_t meshes_count = 0;
	Mesh* meshes;
}; // struct Renderer

} // namespace Renderer
