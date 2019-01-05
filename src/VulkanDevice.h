#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include <vector>
#include <stdio.h>

void SetupPhysicalDevice(VkInstance instance, VkPhysicalDevice* outPhysicalDevice, VkDevice* outDevice, uint32_t* outQueueFamilyIndex)
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

	uint32_t queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(*outPhysicalDevice, &queueCount, nullptr);
	R_ASSERT(queueCount > 0 && "No queue families found");
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(*outPhysicalDevice, &queueCount, queueFamilyProperties.data());

	// Initialize the queue(s)
	*outQueueFamilyIndex = 0;
	VkDeviceQueueCreateInfo qci = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	// TODO: Enumerate all available queue and query for graphics, transfer and compute queue indices
	// for not we pick the first one
	qci.queueFamilyIndex = *outQueueFamilyIndex;
	qci.queueCount = 1;
	float queuePriorities[] = { 1.0f };
	qci.pQueuePriorities = queuePriorities;

	// Device extensions
	std::vector<const char*> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	uint32_t extensionCount = 0;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(*outPhysicalDevice, nullptr, &extensionCount, nullptr));
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(*outPhysicalDevice, nullptr, &extensionCount, availableExtensions.data()));

	LogDeviceExtensions(&availableExtensions);

	VkBool32 supported = CheckExtensionsSupport(&requiredExtensions, &availableExtensions);
	R_ASSERT(supported == VK_TRUE && L"Required instance extensions not supported!");

	VkDeviceCreateInfo dci = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &qci;
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