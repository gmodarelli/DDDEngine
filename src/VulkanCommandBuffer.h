#ifndef VULKAN_COMMAND_BUFFER_H_
#define VULKAN_COMMAND_BUFFER_H_

void SetupCommandBuffer(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkCommandPool* outCommandPool, VkCommandBuffer* outCommandBuffer)
{
	// Get the device queue to submit commands to
	VkQueue presentQueue;
	vkGetDeviceQueue(device, queueFamilyIndex, 0, &presentQueue);

	// Create the command pool
	VkCommandPoolCreateInfo cpci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	cpci.queueFamilyIndex = queueFamilyIndex;

	VK_CHECK(vkCreateCommandPool(device, &cpci, nullptr, outCommandPool));

	VkCommandBufferAllocateInfo cbai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	cbai.commandPool = *outCommandPool;
	cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cbai.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(device, &cbai, outCommandBuffer));
}

void DestroyCommandBuffer(VkDevice device, VkCommandPool* commandPool, VkCommandBuffer* commandBuffer)
{
	vkFreeCommandBuffers(device, *commandPool, 1, commandBuffer);
	vkDestroyCommandPool(device, *commandPool, nullptr);

	*commandBuffer = VK_NULL_HANDLE;
	*commandPool = VK_NULL_HANDLE;
}
#endif // VULKAN_COMMAND_BUFFER_H_ 
