#pragma once

#include <ctype.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Resources
{

struct Node
{
	char name[64];
	uint32_t mesh_id;
	glm::vec3 translation = { 0.0f, 0.0f, 0.0f };
	glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
	glm::quat rotation = glm::identity<glm::quat>();
};

struct Model
{
	char name[64];
	uint32_t node_count = 0;
	Node nodes[8];
};

struct Primitive
{
	uint32_t vertex_offset;
	uint32_t index_offset;
	uint32_t index_count;
	uint32_t material_id;
};

struct Mesh
{
	char name[64];
	uint32_t primitive_count = 0;
	Primitive primitives[8];
};

enum MaterialAlphaMode
{
	Opaque,
	Mask,
	Blend
};

struct Material
{
	char name[64];
	uint32_t name_length = 0;
	MaterialAlphaMode alpha_mode;

	struct PBRMetallicRoughness
	{
		glm::vec4 base_color_factor = glm::vec4(1.0f);
		float metallic_factor = 0.0f;
		float roughness_factor = 0.0f;
	} pbr_metallic_roughness;
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 tex_coord_0;
};

typedef uint16_t Index16;
typedef uint16_t Index32;


const size_t vertex_buffer_size = 1024;
const size_t index_buffer_size = 2048;
const size_t uniform_buffer_size = 1024;

struct AssetsInfo
{
	size_t model_offset = 0;
	size_t material_offset = 0;
	size_t mesh_offset = 0;
	size_t vertex_offset = 0;
	size_t index_offset = 0;

	Model models[16];
	Material materials[16];
	Mesh meshes[32];
	Vertex vertices[vertex_buffer_size];
	Index16 indices[index_buffer_size];
};

} // namespace Resources
