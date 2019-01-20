#pragma once

namespace vkr
{
	struct Buffer
	{
		VkBuffer Buffer;
		VkDeviceMemory DeviceMemory;
		VkDeviceSize size;
		VkBufferUsageFlags usage;
		void* data;
	};

	// TODO: Instead of failing pass 2 VkMemoryPropertyFlags params, required and optional
	// and don't fail in case the optional flags are not supported
	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
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

	VkResult createBuffer(vkr::VulkanDevice* device, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, void* data = nullptr)
	{
		// Create the buffer handle
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = usageFlags;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VKR_CHECK(vkCreateBuffer(device->Device, &bufferInfo, nullptr, buffer), "Failed to create the buffer");

		// Create the memory backing up the buffer handle
		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device->Device, *buffer, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = findMemoryType(device->PhysicalDevice, memoryRequirements.memoryTypeBits, memoryPropertyFlags);
		VKR_CHECK(vkAllocateMemory(device->Device, &allocateInfo, nullptr, memory), "Failed to allocate memory for the buffer");

		// If a pointer to the buffer data has been passed, map the buffer and copy the data over
		if (data != nullptr)
		{
			void *mapped;
			VKR_CHECK(vkMapMemory(device->Device, *memory, 0, size, 0, &mapped), "Failed to map memory");
			memcpy(mapped, data, size);
			// If host coherent hasn't been requested, do a manual flush to make writes visilble
			if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				VkMappedMemoryRange mappedRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
				mappedRange.memory = *memory;
				mappedRange.offset = 0;
				mappedRange.size = size;
				vkFlushMappedMemoryRanges(device->Device, 1, &mappedRange);
			}
			vkUnmapMemory(device->Device, *memory);
		}

		// Attach the memory to the buffer object
		VKR_CHECK(vkBindBufferMemory(device->Device, *buffer, *memory, 0), "Failed to bind memory to the buffer");

		return VK_SUCCESS;
	}

	void destroyBuffer(VkDevice device, Buffer* buffer)
	{
		vkFreeMemory(device, buffer->DeviceMemory, nullptr);
		vkDestroyBuffer(device, buffer->Buffer, nullptr);

		buffer->DeviceMemory = nullptr;
		buffer->Buffer = VK_NULL_HANDLE;
	}
}