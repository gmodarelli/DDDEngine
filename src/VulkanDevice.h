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

void SetupPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, VkQueueFlags requiredQueues, VkPhysicalDevice* outPhysicalDevice, VkDevice* outDevice, QueueFamilyIndices* outQueueFamilyIndices)
{
	// Query how many devices are present in the system
	uint32_t deviceCount;

	VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
	R_ASSERT(deviceCount > 0 && "Could find any suitable device");
	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()));
	R_ASSERT(physicalDevices.size() > 0);

	VkPhysicalDeviceFeatures requiredFeatures = {};
	requiredFeatures.geometryShader = VK_TRUE;
	requiredFeatures.shaderClipDistance = VK_TRUE;

	std::vector<VkPhysicalDeviceProperties> physicalDevicesProperties;

	// Enumerate all physical devices and print out the details
	for (uint32_t i = 0; i < deviceCount; ++i)
	{
		VkPhysicalDeviceProperties deviceProperties;
		memset(&deviceProperties, 0, sizeof(deviceProperties));

		vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);

		physicalDevicesProperties.push_back(deviceProperties);

		if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			printf("The device is not a discrete GPU so we don't pick it.\n");
			continue;
		}

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(physicalDevices[i], &features);

		if (features.geometryShader != VK_TRUE || features.shaderClipDistance != VK_TRUE) {
			printf("The device does not support Geometry Shaders or Clip Distance so we don't pick it.\n");
			continue;
		}

		if (*outPhysicalDevice == VK_NULL_HANDLE)
			*outPhysicalDevice = physicalDevices[i];
	}

	LogPhysicalDeviceProperties(&physicalDevicesProperties);

	// Initialize the queue(s)
	// TODO: This queue-related code can be simplified and moved out of this function.
	// TODO: Add support for finding a Compute-capable queue.
	// TODO: Figure out if we need to take more than one queue per queue family index

	uint32_t queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(*outPhysicalDevice, &queueCount, nullptr);
	R_ASSERT(queueCount > 0 && "No queue families found");
	std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(*outPhysicalDevice, &queueCount, queueFamilies.data());

	// Try to find a queue for graphics/present and a separate one for tranfser
	if (requiredQueues & VK_QUEUE_GRAPHICS_BIT && requiredQueues & VK_QUEUE_TRANSFER_BIT)
	{
		for (uint32_t i = 0; i < queueCount; ++i)
		{
			if (outQueueFamilyIndices->GraphicsFamilyIndex == -1 && queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				outQueueFamilyIndices->GraphicsFamilyIndex = i;

			if (outQueueFamilyIndices->TransferFamilyIndex == -1 && queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
				outQueueFamilyIndices->TransferFamilyIndex = i;

			if (outQueueFamilyIndices->PresentFamilyIndex == -1)
			{
				VkBool32 presentSupport = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(*outPhysicalDevice, i, surface, &presentSupport);
				if (queueFamilies[i].queueCount > 0 && presentSupport)
					outQueueFamilyIndices->PresentFamilyIndex = i;
			}

			if (outQueueFamilyIndices->GraphicsFamilyIndex >= 0 && outQueueFamilyIndices->PresentFamilyIndex >= 0 && outQueueFamilyIndices->TransferFamilyIndex >= 0)
				break;
		}
	}
	// Try to find a Queue for Graphics and for presenting to the surface
	else if (requiredQueues & VK_QUEUE_GRAPHICS_BIT)
	{
		for (uint32_t i = 0; i < queueCount; ++i)
		{
			if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				outQueueFamilyIndices->GraphicsFamilyIndex = i;

			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(*outPhysicalDevice, i, surface, &presentSupport);
			if (queueFamilies[i].queueCount > 0 && presentSupport)
				outQueueFamilyIndices->PresentFamilyIndex = i;

			if (outQueueFamilyIndices->GraphicsFamilyIndex >= 0 && outQueueFamilyIndices->PresentFamilyIndex >= 0)
				break;
		}
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	float queuePriorities[] = { 1.0f };

	if (requiredQueues & VK_QUEUE_GRAPHICS_BIT)
	{
		R_ASSERT(outQueueFamilyIndices->GraphicsFamilyIndex >= 0 && "Could not find a graphics capable queue");
		R_ASSERT(outQueueFamilyIndices->PresentFamilyIndex >= 0 && "Could not find a present capable queue");

		VkDeviceQueueCreateInfo graphicsQueueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		graphicsQueueInfo.queueFamilyIndex = outQueueFamilyIndices->GraphicsFamilyIndex;
		graphicsQueueInfo.queueCount = 1;
		graphicsQueueInfo.pQueuePriorities = queuePriorities;
		queueCreateInfos.push_back(graphicsQueueInfo);

		if (outQueueFamilyIndices->PresentFamilyIndex != outQueueFamilyIndices->GraphicsFamilyIndex)
		{
			VkDeviceQueueCreateInfo presentQueueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			presentQueueInfo.queueFamilyIndex = outQueueFamilyIndices->PresentFamilyIndex;
			presentQueueInfo.queueCount = 1;
			presentQueueInfo.pQueuePriorities = queuePriorities;
			queueCreateInfos.push_back(presentQueueInfo);
		}
	}

	if (requiredQueues & VK_QUEUE_TRANSFER_BIT)
	{
		R_ASSERT(outQueueFamilyIndices->TransferFamilyIndex >= 0 && "Could not find a transfer capable queue");

		if (outQueueFamilyIndices->TransferFamilyIndex != outQueueFamilyIndices->GraphicsFamilyIndex)
		{
			VkDeviceQueueCreateInfo transferQueueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			transferQueueInfo.queueFamilyIndex = outQueueFamilyIndices->TransferFamilyIndex;
			transferQueueInfo.queueCount = 1;
			transferQueueInfo.pQueuePriorities = queuePriorities;
			queueCreateInfos.push_back(transferQueueInfo);
		}
	}

	// Device extensions
	std::vector<const char*> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	uint32_t extensionCount = 0;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(*outPhysicalDevice, nullptr, &extensionCount, nullptr));
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(*outPhysicalDevice, nullptr, &extensionCount, availableExtensions.data()));

	LogDeviceExtensions(&availableExtensions);

	VkBool32 supported = CheckExtensionsSupport(&requiredExtensions, &availableExtensions);
	R_ASSERT(supported == VK_TRUE && "Required instance extensions not supported!");

	VkDeviceCreateInfo dci = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	dci.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	dci.pQueueCreateInfos = queueCreateInfos.data();
	dci.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	dci.ppEnabledExtensionNames = requiredExtensions.size() > 0 ? requiredExtensions.data() : nullptr;
	dci.pEnabledFeatures = &requiredFeatures;

	VK_CHECK(vkCreateDevice(*outPhysicalDevice, &dci, nullptr, outDevice));
	volkLoadDevice(*outDevice);
}

void DestroyDevice(VkDevice* outDevice)
{
	vkDestroyDevice(*outDevice, nullptr);
	*outDevice = nullptr;
}

#endif // VULKAN_DEVICE_H_