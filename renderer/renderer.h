#pragma once

#include "../application/platform.h"
#include "../vulkan/backend.h"
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
	glm::vec2 tex_coord;
};

struct DebugLine
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
};

struct ViewUniformBufferObject
{
	glm::mat4 view;
	glm::mat4 projection;
};

struct Frame
{
	uint32_t descriptor_set_count = 0;
	uint32_t descriptor_set_cursor = 0;
	VkDescriptorSet descriptor_sets[256];
	Vulkan::Buffer* view_ubo_buffer = nullptr;
	Vulkan::Buffer* debug_vertex_buffer = nullptr;
};

struct Pipeline
{
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

	uint32_t descriptor_set_layout_count = 0;
	VkDescriptorSetLayout descriptor_set_layouts[16];

	uint32_t descriptor_set_offset = 0;
};

struct Renderer
{
	Renderer(Platform* platform);

	void init();
	void cleanup();

	void create_pipelines();
	Pipeline create_pipeline(const char* vertex_shader_path, const char* fragment_shader_path, VkPipelineVertexInputStateCreateInfo vertex_input_ci, VkPipelineInputAssemblyStateCreateInfo input_assembly_ci,
		uint32_t descriptor_count, VkDescriptorSetLayoutCreateInfo* descriptor_layout_cis, VkPipelineViewportStateCreateInfo viewport_ci, VkPipelineRasterizationStateCreateInfo rasterizer_ci,
		VkPipelineDepthStencilStateCreateInfo depth_stencil_ci, VkPipelineMultisampleStateCreateInfo multisampling_ci, VkPipelineColorBlendStateCreateInfo color_blend_ci,
		VkPipelineDynamicStateCreateInfo dynamic_state_ci, uint32_t push_constant_range_count, VkPushConstantRange* push_constant_ranges);

	void render_frame(float delta_time);
	void prepare_uniform_buffers();
	void update_uniform_buffers(Vulkan::FrameResources& frame_resources);

	void prepare_debug_vertex_buffers();

	// Platform
	Platform* platform;
	// Vulkan Backend
	Vulkan::Backend* backend;

	Pipeline static_pipeline = {};
	Pipeline dynamic_pipeline = {};
	Pipeline debug_pipeline = {};

	Frame frames[Vulkan::MAX_FRAMES_IN_FLIGHT];
	bool wait_on_semaphores = false;

	double frame_cpu_avg = 0;

	uint32_t meshes_count = 0;
	Mesh* meshes;

	// Player info
	Transform player_transform;
	glm::vec3 player_direction = glm::vec3(0.0f, 0.0f, 0.0f);
	float player_speed = 2.0f;

	// Dynamic Entities
	//
	uint32_t dynamic_entity_count = 0;
	Entity* dynamic_entities;

	// Static Entities
	// 
	uint32_t static_entity_count = 0;
	StaticEntity* static_entitites;
	// TODO: Increase this limit if necessary
	uint32_t static_transform_count = 1024;
	Transform* static_transforms;

	Vulkan::Image* texture_image;
	VkSampler texture_sampler;

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
