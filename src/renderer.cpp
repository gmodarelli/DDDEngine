#include <assert.h>
#include "Vulkan.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>

int main()
{
	Renderer::Vulkan::Initialize();
	VkApplicationInfo appInfo = Renderer::Vulkan::createApplicationInfo("Vulkan Renderer", 0, 0, 1);

	std::vector<const char*> instanceExtensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef _DEBUG
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
	};

	uint32_t glfwExtensionCount = 0;
	const char** glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
		instanceExtensions.push_back(glfwRequiredExtensions[i]);
#ifdef _DEBUG
		std::cout << "glfw requires extension " << glfwRequiredExtensions[i] << std::endl;
#endif
	}

	VkInstance instance = Renderer::Vulkan::createInstance(appInfo, instanceExtensions);

#ifdef _DEBUG
	VkDebugUtilsMessengerEXT debugCallback = Renderer::Vulkan::setupDebugCallback(instance, nullptr);
	assert(debugCallback);
#endif

	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	Renderer::Vulkan::DiscreteGPU gpu = Renderer::Vulkan::findDiscreteGPU(instance, deviceExtensions);

	Renderer::Vulkan::Queue graphicsQueue;
	Renderer::Vulkan::Queue computeQueue;
	VkDevice device = Renderer::Vulkan::createDevice(gpu, deviceExtensions, graphicsQueue, computeQueue);

	assert(glfwInit());
	// NOTE: If we don't tell glfw not to use any API it'll automatically associate its surface to OpenGL.
	// When creating a surface with glfwCreateWindowSurface you'll get a VK_ERROR_NATIVE_WINDOW_IN_USE_KHR error,
	// but if you use the vkCreateWin32SurfaceKHR function you won't get any validation error. Only when trying
	// to associate the surface to a swapchain (that happens when creating a swapchain) you'll get a 
	// VK_ERROR_VALIDATION_FAILED_EXT error and no validation layer messages
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	uint32_t windowWidth = 1024;
	uint32_t windowHeight = 768;
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Renderer", 0, 0);
	assert(window);

	HINSTANCE hinstance = GetModuleHandle(nullptr);
	assert(hinstance);
	HWND windowHandle = glfwGetWin32Window(window);
	assert(windowHandle);

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	VkSurfaceKHR presentationSurface = Renderer::Vulkan::createSurface(instance, gpu, graphicsQueue, hinstance, windowHandle, &presentMode);

	VkSwapchainKHR swapchain = Renderer::Vulkan::createSwapchain(device, gpu, presentationSurface, presentMode, windowWidth, windowHeight);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

#ifdef _DEBUG
	Renderer::Vulkan::destroyDebugCallback(instance, debugCallback, nullptr);
#endif

	Renderer::Vulkan::destroySwapchain(device, swapchain);
	Renderer::Vulkan::destroyDevice(device);
	Renderer::Vulkan::destroyInstance(instance);
}