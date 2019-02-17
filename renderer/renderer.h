#pragma once

#include "../vulkan/device.h"
#include "../vulkan/buffer.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Renderer
{

struct Mesh
{
	uint32_t index_offset;
	uint32_t index_count;
	uint32_t vertex_offset;
};

struct Entity
{
	uint32_t mesh_id;
};

struct StaticEntity
{
	uint32_t mesh_id;
	uint32_t count;
	VkDeviceSize transform_offset;
};

struct Transform
{
	glm::vec3 position;
	glm::vec3 scale;
	glm::quat rotation = glm::identity<glm::quat>();
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

struct Frame
{
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	Vulkan::Buffer* ubo_buffer = nullptr;
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

	Frame frames[Vulkan::MAX_FRAMES_IN_FLIGHT];

	double frame_cpu_avg = 0;

	uint32_t meshes_count = 0;
	Mesh* meshes;

	// Dynamic Entities
	uint32_t dynamic_entity_count = 0;
	Entity* dynamic_entities;

	// Static Entities
	// 
	uint32_t static_entity_count = 0;
	StaticEntity* static_entitites;
	// TODO: Increase this limit if necessary
	uint32_t static_transform_count = 1024;
	Transform* static_transforms;

	// Descriptor Pool helpers
	VkDescriptorPool descriptor_pool;
	VkResult allocate_descriptor_set(const VkDescriptorSetLayout& descriptor_set_layout, VkDescriptorSet& descriptor_set);
	void create_descriptor_pool();
	void destroy_descriptor_pool();

	// UBO Buffer helpers
	void create_ubo_buffers();
	void destroy_ubo_buffers();
}; // struct Renderer

} // namespace Renderer
