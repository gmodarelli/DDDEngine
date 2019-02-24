#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Renderer
{

enum CameraType
{
	LookAt,
	Fly
};

struct Camera
{
	Camera(CameraType camera_type, glm::vec3 position, glm::vec3 front, glm::vec3 up);
	void update_view();

	CameraType type;

	float speed = 1.0f;
	float sensitivity = 0.6f;
	float yaw = -90.0f;
	float pitch = 0.0f;

	glm::vec3 position = { 0.0f, 0.0f, 3.0f };
	glm::vec3 front = { 0.0f, 0.0f, -1.0f };
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::mat4 view;
};

} // namespace Renderer
