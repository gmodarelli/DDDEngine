#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_

// TODO: Instead of failing pass 2 VkMemoryPropertyFlags params, required and optional
// and don't fail in case the optional flags are not supported
uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
	{
		VkMemoryType memoryType = memoryProperties.memoryTypes[i];
		if ((memoryTypeBits & (1 << i)) != 0 && (memoryType.propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	assert(!"No compatible memory found");
	return ~0u;
}

void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Buffer* outBuffer)
{
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &outBuffer->Buffer));
	outBuffer->size = size;
	outBuffer->usage = usage;

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, outBuffer->Buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties);
	VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &outBuffer->DeviceMemory));
	
	// Tie the memory and the object together
	VK_CHECK(vkBindBufferMemory(device, outBuffer->Buffer, outBuffer->DeviceMemory, 0));
}

void DestroyBuffer(VkDevice device, Buffer* buffer)
{
	vkFreeMemory(device, buffer->DeviceMemory, nullptr);
	vkDestroyBuffer(device, buffer->Buffer, nullptr);

	buffer->DeviceMemory = nullptr;
	buffer->Buffer = VK_NULL_HANDLE;
}

#endif // VULKAN_BUFFER_H_
