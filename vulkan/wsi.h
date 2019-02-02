#pragma once

#include "volk.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Vulkan
{

struct WSI
{
	WSI(int width, int height);
	VkSurfaceKHR create_surface(VkInstance instance);

	int get_width();
	int get_height();

	bool alive();
	void cleanup();

private:
	int width;
	int height;
	GLFWwindow* window = NULL;

}; // struct WSI

} // namespace Vulkan