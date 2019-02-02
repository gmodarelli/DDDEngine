#include "wsi.h"
#include <stdio.h>
#include <cassert>

namespace Vulkan
{

WSI::WSI(int width, int height) : width(width), height(height)
{
	glfwInit();

	// GLFW was designed to work with OpenGL so we need to tell it not to
	// create an OpenGL context, otherwise we won't be able to create a 
	// VkSwapchainKHR later.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	const char* error;
	glfwGetError(&error);
	if (error)
	{
		printf("[WSI]: %s\n", error);
		assert(!"Failed to initialize GLFW");
	}


	// Disable window resizing for now.
	// TODO: Remove this line once we can handle window resize
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwGetError(&error);
	if (error)
	{
		printf("[WSI]: %s\n", error);
		assert(!"Failed to initialize GLFW");
	}

	window = glfwCreateWindow(width, height, "A Fante", nullptr, nullptr);
	if (window == NULL)
	{
		glfwGetError(&error);
		if (error)
		{
			printf("[WSI]: %s\n", error);
			assert(false);
		}
	}
}

int WSI::get_width() { return width; }
int WSI::get_height() { return height; }

bool WSI::alive()
{
	glfwPollEvents();
	return !glfwWindowShouldClose(window);
}

void WSI::cleanup()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

VkSurfaceKHR WSI::create_surface(VkInstance instance)
{
	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
	assert(vkCreateWin32SurfaceKHR != nullptr);

	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfaceCreateInfo.hwnd = glfwGetWin32Window(window);
	surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

	VkSurfaceKHR surface;
	VkResult result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
	assert(result == VK_SUCCESS);

	return surface;
}

} // namespace Vulkan