#include "image.h"

namespace Vulkan
{

Image::Image(VkDevice device, VkPhysicalDevice gpu, VkExtent2D extent, VkFormat format, VkSampleCountFlagBits samples, VkImageAspectFlags aspect_flags, VkImageTiling tiling, VkImageUsageFlags usage_flags, VkMemoryPropertyFlags memory_property_flags, VkSharingMode sharing_mode)
{
	image_format = format;

	// Create the depth image
	VkImageCreateInfo image_ci = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	image_ci.imageType = VK_IMAGE_TYPE_2D;
	image_ci.extent.width = extent.width;
	image_ci.extent.height = extent.height;
	image_ci.extent.depth = 1;
	image_ci.mipLevels = 1;
	image_ci.arrayLayers = 1;
	image_ci.format = format;
	image_ci.tiling = tiling;
	image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_ci.usage = usage_flags;
	image_ci.samples = samples;
	image_ci.sharingMode = sharing_mode;

	VkResult result = vkCreateImage(device, &image_ci, nullptr, &image);
	assert(result == VK_SUCCESS);

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, image, &memory_requirements);

	VkMemoryAllocateInfo memory_ai = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memory_ai.allocationSize = memory_requirements.size;
	memory_ai.memoryTypeIndex = find_memory_type(gpu, memory_requirements.memoryTypeBits, memory_property_flags);

	result = vkAllocateMemory(device, &memory_ai, nullptr, &image_memory);
	assert(result == VK_SUCCESS);

	vkBindImageMemory(device, image, image_memory, 0);

	VkImageViewCreateInfo image_view_ci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	image_view_ci.image = image;
	image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_ci.format = format;
	image_view_ci.subresourceRange.aspectMask = aspect_flags;
	image_view_ci.subresourceRange.baseMipLevel = 0;
	image_view_ci.subresourceRange.levelCount = 1;
	image_view_ci.subresourceRange.baseArrayLayer = 0;
	image_view_ci.subresourceRange.layerCount = 1;

	result = vkCreateImageView(device, &image_view_ci, nullptr, &image_view);
	assert(result == VK_SUCCESS);

}

void Image::destroy(VkDevice device)
{
	vkFreeMemory(device, image_memory, nullptr);
	vkDestroyImage(device, image, nullptr);
	vkDestroyImageView(device, image_view, nullptr);

	image_memory = VK_NULL_HANDLE;
	image = VK_NULL_HANDLE;
	image_view = VK_NULL_HANDLE;
}

uint32_t Image::find_memory_type(VkPhysicalDevice gpu, uint32_t memory_type_bits, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(gpu, &memoryProperties);

	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
	{
		VkMemoryType memoryType = memoryProperties.memoryTypes[i];
		if ((memory_type_bits & (1 << i)) != 0 && (memoryType.propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	assert(!"No compatible memory found");
}

} // namespace Vulkan
