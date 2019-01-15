#ifndef VULKAN_TYPES_H_
#define VULKAN_TYPES_H_

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "volk.h"
#include <vector>

struct QueueFamilyIndices
{
	int32_t GraphicsFamilyIndex = -1;
	int32_t PresentFamilyIndex  = -1;
	int32_t TransferFamilyIndex = -1;
	int32_t ComputeFamilyIndex  = -1;
};

struct Buffer
{
	VkBuffer Buffer;
	VkDeviceMemory DeviceMemory;
	VkDeviceSize size;
	VkBufferUsageFlags usage;
	void* data;
};

struct BufferImage
{
	VkImage Image;
	VkImageView ImageView;
	VkDeviceMemory ImageMemory;
};

struct Command
{
	VkCommandPool CommandPool = VK_NULL_HANDLE;
	uint32_t CommandBufferCount = 0;
	std::vector<VkCommandBuffer> CommandBuffers;
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