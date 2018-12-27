#pragma once

#include "volk.h"
#include <vector>

namespace ddd
{
	struct VulkanInstance
	{
		VkInstance					instance;
		std::vector<const char*>	enabledExtensions;
		std::vector<const char*>	enabledLayers;
#ifdef _DEBUG
		VkDebugUtilsMessengerEXT	debugCallback;
#endif
	};

	struct VulkanDevice
	{
		VkPhysicalDevice			physicalDevice;
		VkPhysicalDeviceProperties	properties;
		VkPhysicalDeviceFeatures	features;
		VkPhysicalDeviceFeatures	enabledFeatures;
		VkDevice					logicalDevice;
		std::vector<const char*>	supportedExtensions;
		VkCommandPool				defaultGraphicsCommandPool = VK_NULL_HANDLE;

		struct
		{
			uint32_t graphics;
			uint32_t transfer;
			uint32_t compute;
		} queueFamilyIndices;
	};


	struct VulkanSwapchain
	{
		VkPresentModeKHR			presentMode;
		VkSurfaceKHR				presentationSurface;
		VkSurfaceCapabilitiesKHR	surfaceCapabilities;
		VkSwapchainKHR				swapchain;
		VkExtent2D					extent;
		uint32_t					imageCount;
		std::vector<VkImage>		images = {};
	};
}
