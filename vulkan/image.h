#pragma once

#include "volk.h"
#include <cassert>

#if _DEBUG
#define VULKAN_DEBUG_ENABLED
#endif

namespace Vulkan
{

struct Image
{
	Image(VkDevice device, VkPhysicalDevice gpu, VkExtent2D extent, VkFormat format, VkImageAspectFlags aspect_flags,
		  VkImageTiling tiling, VkImageUsageFlags usage_flags, VkMemoryPropertyFlags memory_property_flags, VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE);

	void destroy(VkDevice device);
	
	VkImage image = VK_NULL_HANDLE;
	VkImageView image_view = VK_NULL_HANDLE;
	VkDeviceMemory image_memory = VK_NULL_HANDLE;
	VkFormat image_format = VK_FORMAT_UNDEFINED;

private:
	uint32_t find_memory_type(VkPhysicalDevice gpu, uint32_t memory_type_bits, VkMemoryPropertyFlags properties);
};

} // namespace Vulkan
