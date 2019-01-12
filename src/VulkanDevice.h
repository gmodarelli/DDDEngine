#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include <vector>
#include <stdio.h>

VkQueue GetQueue(VkDevice device, uint32_t queueFamilyIndex)
{
	VkQueue queue;
	vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

	return queue;
}

VkQueryPool CreateQueryPool(VkDevice device, uint32_t queryCount)
{
	VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
	createInfo.queryCount = queryCount;
	createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	VkQueryPool queryPool;
	VK_CHECK(vkCreateQueryPool(device, &createInfo, nullptr, &queryPool));

	return queryPool;
}

void DestroyDevice(VkDevice* outDevice)
{
	vkDestroyDevice(*outDevice, nullptr);
	*outDevice = VK_NULL_HANDLE;
}

void DestroyQueryPool(VkDevice device, VkQueryPool& queryPool)
{
	vkDestroyQueryPool(device, queryPool, nullptr);
	queryPool = VK_NULL_HANDLE;
}

#endif // VULKAN_DEVICE_H_