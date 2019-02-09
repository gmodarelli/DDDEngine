#pragma once

#include "volk.h"
#include <cassert>

#if _DEBUG
#define VULKAN_DEBUG_ENABLED
#endif

namespace Vulkan
{

struct Buffer
{
	Buffer(VkDevice device, VkPhysicalDevice gpu, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE);
	void destroy(VkDevice device);
	
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory device_memory = VK_NULL_HANDLE;
	VkDeviceSize size = 0;
	VkBufferUsageFlags usage;
	void* data;

private:

	uint32_t find_memory_type(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);
};

} // namespace Vulkan