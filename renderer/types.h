#pragma once

#include <ctype.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Renderer
{

struct Entity
{
	char name[64];
	uint32_t model_id;
};

struct Transform
{
	glm::vec3 position;
	glm::vec3 scale;
	glm::quat rotation = glm::identity<glm::quat>();
};

struct AlignedTransform
{
	glm::vec4 position;
	glm::vec4 scale;
	glm::quat rotation = glm::identity<glm::quat>();
};

struct NodeUboTransform
{
	AlignedTransform* transforms;
};

struct NodeUbo
{
	glm::mat4* model;
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