#include "buffer.h"
#include <cassert>

namespace Vulkan
{

Buffer::Buffer(VkDevice device, VkPhysicalDevice gpu, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_property_flags, VkDeviceSize size, VkSharingMode sharing_mode, bool align)
{
	if (align)
	{
		this->size = ((size - 1) / 256 + 1) * 256;
	}
	{
		this->size = size;
	}
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

	if (align)
	{
		assert(256 == memory_requirements.alignment);
	}

	VkMemoryAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = find_memory_type(gpu, memory_requirements.memoryTypeBits, memory_property_flags);
	result = vkAllocateMemory(device, &allocate_info, nullptr, &device_memory);
	assert(result == VK_SUCCESS);

	// Attach the memory to the buffer object
	result = vkBindBufferMemory(device, buffer, device_memory, 0);
	assert(result == VK_SUCCESS);
}

VkResult Buffer::map(VkDevice device, VkDeviceSize size, VkDeviceSize offset)
{
	return vkMapMemory(device, device_memory, offset, size, 0, &mapped);
}

void Buffer::unmap(VkDevice device)
{
	if (mapped)
	{
		vkUnmapMemory(device, device_memory);
		mapped = nullptr;
	}
}

VkResult Buffer::flush(VkDevice device, VkDeviceSize size, VkDeviceSize offset)
{
	VkMappedMemoryRange mapped_range = {};
	mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mapped_range.memory = device_memory;
	mapped_range.offset = offset;
	mapped_range.size = size;
	return vkFlushMappedMemoryRanges(device, 1, &mapped_range);
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