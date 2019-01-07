#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_

uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
	{
		VkMemoryType memoryType = memoryProperties.memoryTypes[i];
		if ((typeFilter & (1 << i)) && (memoryType.propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	assert(!"Couldn't find the appropriate memory type");
}

void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Buffer* outBuffer)
{
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &outBuffer->Buffer));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, outBuffer->Buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties);

	VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &outBuffer->DeviceMemory));
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
