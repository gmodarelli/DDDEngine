#pragma once
#include "volk.h"
#include "Types.h"
#include "VulkanDebug.h"
#include "VulkanInstance.h"
#include <vector>

namespace ddd
{
	bool getQueueFamilyIndex(std::vector<VkQueueFamilyProperties> queueFamilyProperties, VkQueueFlagBits queueFlagBits, uint32_t* pFamilyIndex);
	VkCommandPool createCommandPool(VulkanDevice* device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	bool pickSuitableGPU(VulkanDevice* vulkanDevice, VkInstance instance, std::vector<const char*> desiredExtensions)
	{
		uint32_t physicalDeviceCount;
		VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
		DDD_ASSERT(physicalDeviceCount);
		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));

		for (const auto physicalDevice : physicalDevices)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(physicalDevice, &properties);

			if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) continue;

			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures(physicalDevice, &features);
			if (features.geometryShader != VK_TRUE) continue;

			// NOTE: We're only interested in the geometry shader feature
			VkPhysicalDeviceFeatures enabledFeatures = {};
			enabledFeatures.geometryShader = VK_TRUE;

			uint32_t extensionsCount;
			VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, nullptr));
			std::vector<VkExtensionProperties> extensions(extensionsCount);
			VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, extensions.data()));

			float extensionsSupported = true;
			for (const auto desiredExtension : desiredExtensions)
			{
				if (!IsExtensionSupported(extensions, desiredExtension))
				{
					extensionsSupported = false;
					break;
				}
			}

			if (!extensionsSupported) continue;

			vulkanDevice->physicalDevice = physicalDevice;
			vulkanDevice->features = features;
			vulkanDevice->enabledFeatures = enabledFeatures;
			vulkanDevice->properties = properties;

			for (const auto extension : extensions)
			{
				vulkanDevice->supportedExtensions.push_back(extension.extensionName);
			}

			return true;
		}

		return false;
	}

	bool createLogicalDevice(VulkanDevice* vulkanDevice, std::vector<const char*> enabledExtensions, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)
	{
		DDD_ASSERT(vulkanDevice->physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = {};
		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(vulkanDevice->physicalDevice, &queueFamilyCount, nullptr);
		DDD_ASSERT(queueFamilyCount > 0);
		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(vulkanDevice->physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

		const float defaultPriority = 1.0f;
		
		// Graphics queue
		if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
		{
			uint32_t familyIndex;
			bool result = getQueueFamilyIndex(queueFamilyProperties, VK_QUEUE_GRAPHICS_BIT, &familyIndex);
			DDD_ASSERT(result == true);
			VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			queueCreateInfo.queueFamilyIndex = familyIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &defaultPriority;
			queueCreateInfos.push_back(queueCreateInfo);
			vulkanDevice->queueFamilyIndices.graphics = familyIndex;
		}
		else
		{
			vulkanDevice->queueFamilyIndices.graphics = VK_NULL_HANDLE;
		}

		// Dedicated compute queue
		if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
		{
			uint32_t familyIndex;
			bool result = getQueueFamilyIndex(queueFamilyProperties, VK_QUEUE_COMPUTE_BIT, &familyIndex);
			DDD_ASSERT(result == true);
			if (familyIndex != vulkanDevice->queueFamilyIndices.graphics)
			{
				VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
				queueCreateInfo.queueFamilyIndex = familyIndex;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &defaultPriority;
				queueCreateInfos.push_back(queueCreateInfo);
				vulkanDevice->queueFamilyIndices.compute = familyIndex;
			}
		}
		else
		{
			vulkanDevice->queueFamilyIndices.compute = vulkanDevice->queueFamilyIndices.graphics;
		}

		// Dedicated transfer queue
		if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
		{
			uint32_t familyIndex;
			bool result = getQueueFamilyIndex(queueFamilyProperties, VK_QUEUE_TRANSFER_BIT, &familyIndex);
			DDD_ASSERT(result == true);
			if (familyIndex != vulkanDevice->queueFamilyIndices.graphics && familyIndex != vulkanDevice->queueFamilyIndices.compute)
			{
				VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
				queueCreateInfo.queueFamilyIndex = familyIndex;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &defaultPriority;
				queueCreateInfos.push_back(queueCreateInfo);
				vulkanDevice->queueFamilyIndices.compute = familyIndex;
			}
		}
		else
		{
			vulkanDevice->queueFamilyIndices.transfer = vulkanDevice->queueFamilyIndices.graphics;
		}

		VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
		createInfo.ppEnabledExtensionNames = static_cast<uint32_t>(enabledExtensions.size()) > 0 ? enabledExtensions.data() : nullptr;
		createInfo.pEnabledFeatures = &vulkanDevice->enabledFeatures;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		VK_CHECK(vkCreateDevice(vulkanDevice->physicalDevice, &createInfo, nullptr, &vulkanDevice->logicalDevice));
		DDD_ASSERT(vulkanDevice->logicalDevice);

		volkLoadDevice(vulkanDevice->logicalDevice);

		vulkanDevice->defaultGraphicsCommandPool = createCommandPool(vulkanDevice, vulkanDevice->queueFamilyIndices.graphics);

		return true;
	}

	void destroyVulkanDevice(const VulkanInstance* vulkanInstance, VulkanDevice* vulkanDevice)
	{
		DDD_ASSERT(vulkanDevice->logicalDevice != VK_NULL_HANDLE);
		if (vulkanDevice->defaultGraphicsCommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(vulkanDevice->logicalDevice, vulkanDevice->defaultGraphicsCommandPool, nullptr);
			vulkanDevice->defaultGraphicsCommandPool = VK_NULL_HANDLE;
		}

		vkDestroyDevice(vulkanDevice->logicalDevice, nullptr);
		vulkanDevice->logicalDevice = VK_NULL_HANDLE;
	}

	bool getQueueFamilyIndex(std::vector<VkQueueFamilyProperties> queueFamilyProperties, VkQueueFlagBits queueFlags, uint32_t* familyIndex)
	{
		if (queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i)
			{
				if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					*familyIndex = i;
					return true;
				}
			}
		}

		if (queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i)
			{
				if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					*familyIndex = i;
					return true;
				}
			}
		}

		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i)
		{
			if (queueFamilyProperties[i].queueFlags & queueFlags)
			{
				*familyIndex = i;
				return true;
			}
		}

		return false;
	}

	VkCommandPool createCommandPool(VulkanDevice* vulkanDevice, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
	{
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.queueFamilyIndex = queueFamilyIndex;
		createInfo.flags = createFlags;

		VkCommandPool commandPool;
		VK_CHECK(vkCreateCommandPool(vulkanDevice->logicalDevice, &createInfo, nullptr, &commandPool));

		return commandPool;
	}

	VkCommandBuffer createCommandBuffer(VulkanDevice* vulkanDevice, VkCommandBufferLevel level)
	{
		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = vulkanDevice->defaultGraphicsCommandPool;
		allocateInfo.level = level;
		allocateInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		VK_CHECK(vkAllocateCommandBuffers(vulkanDevice->logicalDevice, &allocateInfo, &commandBuffer));

		return commandBuffer;
	}
}
