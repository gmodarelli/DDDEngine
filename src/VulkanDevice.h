#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include <assert.h>
#include <vector>
#include <stdio.h>

void SetupPhysicalDevice(VkInstance instance, VkPhysicalDevice* outPhysicalDevice, VkDevice* outDevice)
{
	// Query how many devices are present in the system
	uint32_t deviceCount;

	VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
	assert(deviceCount > 0 && "Could find any suitable device");
	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()));
	assert(physicalDevices.size() > 0);

	VkPhysicalDeviceFeatures desiredFeatures = {};
	desiredFeatures.geometryShader = VK_TRUE;
	desiredFeatures.shaderClipDistance = VK_TRUE;

	// Enumerate all physical devices and print out the details
	for (uint32_t i = 0; i < deviceCount; ++i)
	{
		VkPhysicalDeviceProperties deviceProperties;
		memset(&deviceProperties, 0, sizeof(deviceProperties));

		vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);

		printf("Driver Version: %d\n", deviceProperties.driverVersion);
		printf("Device Name: %s\n", deviceProperties.deviceName);
		printf("Device Type: %d\n", deviceProperties.deviceType);
		printf("API Version: %d.%d.%d\n", VK_VERSION_MAJOR(deviceProperties.apiVersion), VK_VERSION_MINOR(deviceProperties.apiVersion), VK_VERSION_PATCH(deviceProperties.apiVersion));

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

		*outPhysicalDevice = physicalDevices[i];
		break;
	}

	// Fill out the physical device memory properties
	// TODO: This will be needed for buffers
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(*outPhysicalDevice, &memoryProperties);

	uint32_t queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(*outPhysicalDevice, &queueCount, nullptr);
	assert(queueCount > 0 && "No queue families found");
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(*outPhysicalDevice, &queueCount, queueFamilyProperties.data());

	// Initialize the queue(s)
	VkDeviceQueueCreateInfo qci = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	// TODO: Enumerate all available queue and query for graphics, transfer and compute queue indices
	// for not we pick the first one
	qci.queueFamilyIndex = 0;
	qci.queueCount = 1;
	float queuePriorities[] = { 1.0f };
	qci.pQueuePriorities = queuePriorities;

	// Device extensions
	// TODO: Check if desired extensions are available
	const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo dci = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &qci;
	dci.enabledExtensionCount = ARRAYSIZE(deviceExtensions);
	dci.ppEnabledExtensionNames = ARRAYSIZE(deviceExtensions) > 0 ? deviceExtensions : nullptr;
	dci.pEnabledFeatures = &desiredFeatures;

	VK_CHECK(vkCreateDevice(*outPhysicalDevice, &dci, nullptr, outDevice));
	volkLoadDevice(*outDevice);
}

void DestroyDevice(VkDevice* outDevice)
{
	vkDestroyDevice(*outDevice, nullptr);
	*outDevice = nullptr;
}

#endif // VULKAN_DEVICE_H_