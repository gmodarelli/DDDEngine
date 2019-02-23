#pragma once

#include <ctype.h>
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
	size_t transform_offset;
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

}