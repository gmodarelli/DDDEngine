#pragma once

#include <ctype.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Renderer
{

struct Primitive
{
	uint32_t vertex_offset;
	uint32_t index_offset;
	uint32_t index_count;
};

struct Entity
{
	uint32_t primitive_id;
	char name[64];
};

struct StaticEntity
{
	uint32_t primitive_id;
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

typedef uint16_t Index;

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