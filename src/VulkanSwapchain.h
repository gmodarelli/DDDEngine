#pragma once

#include "volk.h"
#include "Types.h"
#include "VulkanDebug.h"

namespace ddd
{
	uint32_t chooseImageCount(VkSurfaceCapabilitiesKHR surfaceCapabilities);
	VkExtent2D chooseExtent2D(VkSurfaceCapabilitiesKHR surfaceCapabilities, uint32_t windowWidth, uint32_t windowHeight);
	VkSurfaceFormatKHR chooseSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR presentationSurface, VkSurfaceFormatKHR desiredFormat);
	bool getSwapchainImages(VulkanSwapchain* swapchain, VulkanDevice* device);

	bool createSurface(VulkanSwapchain* swapchain, VulkanInstance* instance, VulkanDevice* device, VkPresentModeKHR desiredPresentMode, HINSTANCE hinstance, HWND windowHandle)
	{
		VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
		createInfo.hinstance = hinstance;
		createInfo.hwnd = windowHandle;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VK_CHECK(vkCreateWin32SurfaceKHR(instance->instance, &createInfo, nullptr, &surface));
		DDD_ASSERT(surface);

		VkBool32 supported = false;
		VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device->physicalDevice, device->queueFamilyIndices.graphics, surface, &supported));
		if (supported != VK_TRUE) return false;

		uint32_t presentModeCount = 0;
		VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device->physicalDevice, surface, &presentModeCount, nullptr));
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device->physicalDevice, surface, &presentModeCount, presentModes.data()));
		DDD_ASSERT(presentModeCount > 0);

		for (const auto presentMode : presentModes) {
			if (presentMode == desiredPresentMode) {
				swapchain->presentMode = desiredPresentMode;
				break;
			}
		}

		if (!swapchain->presentMode != desiredPresentMode)
		{
			swapchain->presentMode = VK_PRESENT_MODE_FIFO_KHR;
		}

		swapchain->presentationSurface = surface;
		return true;
	}

	bool createSwapchain(VulkanSwapchain* swapchain, VulkanDevice* vulkanDevice, uint32_t width, uint32_t height)
	{
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanDevice->physicalDevice, swapchain->presentationSurface, &swapchain->surfaceCapabilities));

		swapchain->imageCount = chooseImageCount(swapchain->surfaceCapabilities);
		swapchain->extent = chooseExtent2D(swapchain->surfaceCapabilities, width, height);

		VkImageUsageFlags desiredUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		bool supported = desiredUsage == (desiredUsage & swapchain->surfaceCapabilities.supportedUsageFlags);
		assert(supported == true);

		VkSurfaceTransformFlagBitsKHR surfaceTransform;
		VkSurfaceTransformFlagBitsKHR desiredTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		if (swapchain->surfaceCapabilities.supportedTransforms & desiredTransform) {
			surfaceTransform = desiredTransform;
		}
		else {
			surfaceTransform = swapchain->surfaceCapabilities.currentTransform;
		}

		VkSurfaceFormatKHR surfaceFormat = chooseSwapchainFormat(vulkanDevice->physicalDevice, swapchain->presentationSurface, { VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR });

		VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		createInfo.surface = swapchain->presentationSurface;
		createInfo.minImageCount = swapchain->imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = swapchain->extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.imageUsage = desiredUsage;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.preTransform = surfaceTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = swapchain->presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VK_CHECK(vkCreateSwapchainKHR(vulkanDevice->logicalDevice, &createInfo, nullptr, &swapchain->swapchain));

		DDD_ASSERT(getSwapchainImages(swapchain, vulkanDevice));

		return true;
	}

	void destroySwapchain(VulkanSwapchain* swapchain, VkInstance instance, VkDevice device)
	{
		DDD_ASSERT(swapchain->swapchain != VK_NULL_HANDLE);
		vkDestroySwapchainKHR(device, swapchain->swapchain, nullptr);
		swapchain->swapchain = VK_NULL_HANDLE;

		assert(swapchain->presentationSurface != VK_NULL_HANDLE);
		vkDestroySurfaceKHR(instance, swapchain->presentationSurface, nullptr);
		swapchain->presentationSurface = VK_NULL_HANDLE;
	}

	uint32_t chooseImageCount(VkSurfaceCapabilitiesKHR surfaceCapabilities)
	{
		uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
		if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
			imageCount = surfaceCapabilities.maxImageCount;
		}

		return imageCount;
	}

	VkExtent2D chooseExtent2D(VkSurfaceCapabilitiesKHR surfaceCapabilities, uint32_t windowWidth, uint32_t windowHeight)
	{
		if (surfaceCapabilities.currentExtent.width != 0xFFFFFFFF) {
			return surfaceCapabilities.currentExtent;
		}
		else {
			VkExtent2D extent;
			extent.width = windowWidth >= surfaceCapabilities.minImageExtent.width && windowWidth <= surfaceCapabilities.maxImageExtent.width ? windowWidth : surfaceCapabilities.maxImageExtent.width;
			extent.height = windowHeight >= surfaceCapabilities.minImageExtent.height && windowHeight <= surfaceCapabilities.maxImageExtent.height ? windowHeight : surfaceCapabilities.maxImageExtent.height;

			return extent;
		}
	}

	VkSurfaceFormatKHR chooseSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR presentationSurface, VkSurfaceFormatKHR desiredFormat)
	{
		uint32_t formatsCount;
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, presentationSurface, &formatsCount, nullptr));
		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatsCount);
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, presentationSurface, &formatsCount, surfaceFormats.data()));

		if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
			return desiredFormat;
		}

		for (const auto surfaceFormat : surfaceFormats) {
			if (surfaceFormat.format == desiredFormat.format && surfaceFormat.colorSpace == desiredFormat.colorSpace) {
				return desiredFormat;
			}
		}

		for (const auto surfaceFormat : surfaceFormats) {
			if (surfaceFormat.format == desiredFormat.format) {
				return { desiredFormat.format, surfaceFormat.colorSpace };
			}
		}

		return surfaceFormats[0];
	}

	bool getSwapchainImages(VulkanSwapchain* swapchain, VulkanDevice* device)
	{
		VK_CHECK(vkGetSwapchainImagesKHR(device->logicalDevice, swapchain->swapchain, &swapchain->imageCount, nullptr));
		swapchain->images.resize(swapchain->imageCount);
		VK_CHECK(vkGetSwapchainImagesKHR(device->logicalDevice, swapchain->swapchain, &swapchain->imageCount, swapchain->images.data()));

		return true;
	}
}
