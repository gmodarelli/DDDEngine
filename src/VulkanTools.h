#ifndef VULKAN_TOOLS_H_
#define VULKAN_TOOLS_H_

#define VK_CHECK(call) \
	do { \
		VkResult result = call; \
		assert(result == VK_SUCCESS); \
	} while (0);

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include "volk.h"

struct BufferImage {
	VkImage image;
	VkImageView imageView;
	VkDeviceMemory imageMemory;
};

#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"
#include "VulkanCommandBuffer.h"

#endif // VULKAN_TOOLS_H_
