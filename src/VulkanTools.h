#ifndef VULKAN_TOOLS_H_
#define VULKAN_TOOLS_H_

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined __linux__
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined __APPLE__
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include "volk.h"
#include <vector>

struct WindowParameters
{
#ifdef VK_USE_PLATFORM_WIN32_KHR
	HINSTANCE Hinstance;
	HWND HWnd;
#elif defined VK_USE_PLATFORM_XLIB_KHR
	Display* Dpy;
	Window Window;
#elif defined VK_USE_PLATFORM_XCB_KHR
	xcb_connection_t* Connection;
	xcb_window_t Window;
#endif
};

struct QueueFamilyIndices
{
	int32_t GraphicsFamilyIndex = -1;
	int32_t PresentFamilyIndex  = -1;
	int32_t TransferFamilyIndex = -1;
	int32_t ComputeFamilyIndex  = -1;
};

struct Buffer
{
	VkBuffer Buffer;
	VkDeviceMemory DeviceMemory;
};

struct BufferImage
{
	VkImage Image;
	VkImageView ImageView;
	VkDeviceMemory ImageMemory;
};

struct Command
{
	VkCommandPool CommandPool = VK_NULL_HANDLE;
	uint32_t CommandBufferCount = 0;
	std::vector<VkCommandBuffer> CommandBuffers;
};

struct Descriptor
{
	VkDescriptorSetLayout DescriptorSetLayout;
	VkDescriptorPool DescriptorPool;
	uint32_t DescriptorSetCount;
	std::vector<VkDescriptorSet> DescriptorSets;
};

struct Pipeline
{
	VkPipeline Pipeline;
	VkPipelineLayout PipelineLayout;
};

struct SyncObjects
{
	std::vector<VkSemaphore> ImageAvailableSemaphores;
	std::vector<VkSemaphore> RenderFinishedSemaphores;
	std::vector<VkFence> InFlightFences;
};

struct Vertex
{
	float x, y, z, w;
	float r, g, b, a;
};

VkBool32 IsExtensionSupported(const char* extensionName, const std::vector<VkExtensionProperties>* availableExtensions)
{
	for (const auto extension : *availableExtensions)
	{
		if (strstr(extension.extensionName, extensionName)) return VK_TRUE;
	}

	return VK_FALSE;
}

VkBool32 CheckExtensionsSupport(const std::vector<const char*>* extensionNames, const std::vector<VkExtensionProperties>* availableExtensions)
{
	for (const auto extensionName : *extensionNames)
	{
		if (IsExtensionSupported(extensionName, availableExtensions) != VK_TRUE)
		{
			printf("The required %s extension is not supported.\n", extensionName);
			return VK_FALSE;
		}
	}

	return VK_TRUE;
}

#include "VulkanDebug.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"
#include "VulkanCommandBuffer.h"
#include "VulkanVertexBuffer.h"
#include "VulkanShaderAndUniforms.h"
#include "VulkanDescriptors.h"
#include "VulkanPipeline.h"
#include "VulkanRenderLoop.h"
#include "VulkanSynchronization.h"

#endif // VULKAN_TOOLS_H_
