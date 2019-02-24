#include "camera.h"

namespace Renderer
{

Camera::Camera(CameraType type, glm::vec3 position, glm::vec3 front, glm::vec3 up) : type(type), position(position), front(front), up(up)
{
	if (type == CameraType::Fly)
	{
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	}

	update_view();
}

void Camera::update_view()
{
	if (type == CameraType::Fly)
		view = glm::lookAt(position, position + front, up);

	if (type == CameraType::LookAt)
		view = glm::lookAt(position, front, up);
}

}