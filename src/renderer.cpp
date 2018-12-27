#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>
#include "Types.h"
#include "VulkanDebug.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"

int main()
{
	VK_CHECK(volkInitialize());

	std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	ddd::VulkanInstance vulkanInstance = {};
	DDD_ASSERT(ddd::createInstance(&vulkanInstance, instanceExtensions));

	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	ddd::VulkanDevice vulkanDevice = {};
	DDD_ASSERT(ddd::pickSuitableGPU(&vulkanDevice, vulkanInstance.instance, deviceExtensions));
	DDD_ASSERT(ddd::createLogicalDevice(&vulkanDevice, deviceExtensions, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT));

	DDD_ASSERT(glfwInit());
	// NOTE: If we don't tell glfw not to use any API it'll automatically associate its surface to OpenGL.
	// When creating a surface with glfwCreateWindowSurface you'll get a VK_ERROR_NATIVE_WINDOW_IN_USE_KHR error,
	// but if you use the vkCreateWin32SurfaceKHR function you won't get any validation error. Only when trying
	// to associate the surface to a swapchain (that happens when creating a swapchain) you'll get a 
	// VK_ERROR_VALIDATION_FAILED_EXT error and no validation layer messages
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	uint32_t windowWidth = 1920;
	uint32_t windowHeight = 1080;
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Renderer", 0, 0);
	DDD_ASSERT(window);

	HINSTANCE hinstance = GetModuleHandle(nullptr);
	DDD_ASSERT(hinstance);
	HWND windowHandle = glfwGetWin32Window(window);
	DDD_ASSERT(windowHandle);

	ddd::VulkanSwapchain vulkanSwapchain = {};
	DDD_ASSERT(ddd::createSurface(&vulkanSwapchain, &vulkanInstance, &vulkanDevice, VK_PRESENT_MODE_MAILBOX_KHR, hinstance, windowHandle));
	DDD_ASSERT(ddd::createSwapchain(&vulkanSwapchain, &vulkanDevice, windowWidth, windowHeight));

	VkCommandBuffer commandBuffer = ddd::createCommandBuffer(&vulkanDevice, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	DDD_ASSERT(commandBuffer);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	ddd::destroySwapchain(&vulkanSwapchain, vulkanInstance.instance, vulkanDevice.logicalDevice);
	ddd::destroyVulkanDevice(&vulkanInstance, &vulkanDevice);
	ddd::destroyVulkanInstance(&vulkanInstance);
}