#include "buffer.h"
#include <cassert>

namespace Vulkan
{

Buffer::Buffer(VkDevice device, VkPhysicalDevice gpu, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_property_flags, VkDeviceSize size, VkSharingMode sharing_mode)
{
	this->size = size;
	this->usage = usage_flags;

	// Create the buffer handle
	VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buffer_info.size = size;
	buffer_info.usage = usage_flags;
	buffer_info.sharingMode = sharing_mode;
	VkResult result = vkCreateBuffer(device, &buffer_info, nullptr, &buffer);
	assert(result == VK_SUCCESS);

	// Allocate the memory backing the buffer handle
	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

	VkMemoryAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = find_memory_type(gpu, memory_requirements.memoryTypeBits, memory_property_flags);
	result = vkAllocateMemory(device, &allocate_info, nullptr, &device_memory);
	assert(result == VK_SUCCESS);

	// Attach the memory to the buffer object
	result = vkBindBufferMemory(device, buffer, device_memory, 0);
	assert(result == VK_SUCCESS);
}

void Buffer::destroy(VkDevice device)
{
	vkFreeMemory(device, device_memory, nullptr);
	vkDestroyBuffer(device, buffer, nullptr);

	device_memory = VK_NULL_HANDLE;
	buffer = VK_NULL_HANDLE;
}

uint32_t Buffer::find_memory_type(VkPhysicalDevice gpu, uint32_t memory_type_bits, VkMemoryPropertyFlags properties)
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