#include <assert.h>
#include "Vulkan.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>


int main()
{
	Renderer::Vulkan::Initialize();
	VkApplicationInfo appInfo = Renderer::Vulkan::createApplicationInfo("Vulkan Renderer", 0, 0, 1);

	std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	VkInstance instance = Renderer::Vulkan::createInstance(appInfo, instanceExtensions);

	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	Renderer::Vulkan::DiscreteGPU gpu = Renderer::Vulkan::findDiscreteGPU(instance, deviceExtensions);

	Renderer::Vulkan::Queue graphicsQueue;
	Renderer::Vulkan::Queue computeQueue;
	VkDevice device = Renderer::Vulkan::createDevice(gpu, deviceExtensions, graphicsQueue, computeQueue);

	assert(glfwInit());

	GLFWwindow* window = glfwCreateWindow(1024, 768, "Renderer", 0, 0);
	assert(window);

	HINSTANCE hinstance = GetModuleHandleA(0);
	assert(hinstance);
	HWND windowHandle = glfwGetWin32Window(window);
	assert(windowHandle);

	VkSurfaceKHR presentationSurface = Renderer::Vulkan::createSurface(instance, gpu, graphicsQueue, hinstance, windowHandle);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	Renderer::Vulkan::destroyDevice(device);
	Renderer::Vulkan::destroyInstance(instance);
}