#ifndef VULKAN_TYPES_H_
#define VULKAN_TYPES_H_

#include "volk.h"
#include <vector>

struct Buffer
{
	VkBuffer Buffer;
	VkDeviceMemory DeviceMemory;
	VkDeviceSize size;
	VkBufferUsageFlags usage;
	void* data;
};

struct Descriptor
{
	VkDescriptorSetLayout DescriptorSetLayout;
	VkDescriptorPool DescriptorPool;
	uint32_t DescriptorSetCount;
	std::vector<VkDescriptorSet> DescriptorSets;
};

struct Pipeline
{
	VkPipeline Pipeline;
	VkPipelineLayout PipelineLayout;
};

struct SyncObjects
{
	std::vector<VkSemaphore> ImageAvailableSemaphores;
	std::vector<VkSemaphore> RenderFinishedSemaphores;
	std::vector<VkFence> InFlightFences;
};

#endif // VULKAN_TYPES_H_