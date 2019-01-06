#ifndef VULKAN_SYNCHRONIZATION_H_
#define VULKAN_SYNCHRONIZATION_H_

void CreateSyncObjects(VkDevice device, size_t maxFramesInFlight, SyncObjects* outSyncObjects)
{
	outSyncObjects->ImageAvailableSemaphores.resize(maxFramesInFlight);
	outSyncObjects->RenderFinishedSemaphores.resize(maxFramesInFlight);
	outSyncObjects->InFlightFences.resize(maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < maxFramesInFlight; ++i)
	{
		VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &outSyncObjects->ImageAvailableSemaphores[i]));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &outSyncObjects->RenderFinishedSemaphores[i]));
		VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &outSyncObjects->InFlightFences[i]));
	}
}

void DestroySyncObjects(VkDevice device, SyncObjects* syncObjects)
{
	for (size_t i = 0; i < syncObjects->ImageAvailableSemaphores.size(); ++i)
	{
		vkDestroySemaphore(device, syncObjects->ImageAvailableSemaphores[i], nullptr);
	}

	for (size_t i = 0; i < syncObjects->RenderFinishedSemaphores.size(); ++i)
	{
		vkDestroySemaphore(device, syncObjects->RenderFinishedSemaphores[i], nullptr);
	}

	for (size_t i = 0; i < syncObjects->InFlightFences.size(); ++i)
	{
		vkDestroyFence(device, syncObjects->InFlightFences[i], nullptr);
	}

	syncObjects->ImageAvailableSemaphores.clear();
	syncObjects->RenderFinishedSemaphores.clear();
	syncObjects->InFlightFences.clear();
}

#endif // VULKAN_SYNCHRONIZATION_H_
