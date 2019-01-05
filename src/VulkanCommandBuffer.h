#ifndef VULKAN_COMMAND_BUFFER_H_
#define VULKAN_COMMAND_BUFFER_H_

void SetupCommandBuffer(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, uint32_t commandBufferCount, Command* outCommand)
{
	R_ASSERT(commandBufferCount >= 0 && "You need to create at least one command buffer");
	// Get the device queue to submit commands to
	VkQueue presentQueue;
	vkGetDeviceQueue(device, queueFamilyIndex, 0, &presentQueue);

	// Create the command pool
	VkCommandPoolCreateInfo cpci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	cpci.queueFamilyIndex = queueFamilyIndex;

	VK_CHECK(vkCreateCommandPool(device, &cpci, nullptr, &outCommand->CommandPool));

	VkCommandBufferAllocateInfo cbai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	cbai.commandPool = outCommand->CommandPool;
	cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cbai.commandBufferCount = commandBufferCount;

	outCommand->CommandBuffers.resize(commandBufferCount);
	outCommand->CommandBufferCount = commandBufferCount;

	VK_CHECK(vkAllocateCommandBuffers(device, &cbai, outCommand->CommandBuffers.data()));
}

void DestroyCommandBuffer(VkDevice device, Command* command)
{
	vkFreeCommandBuffers(device, command->CommandPool, command->CommandBufferCount, command->CommandBuffers.data());
	vkDestroyCommandPool(device, command->CommandPool, nullptr);

	command->CommandPool = VK_NULL_HANDLE;
}
#endif // VULKAN_COMMAND_BUFFER_H_ 
