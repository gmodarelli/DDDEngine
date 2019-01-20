#pragma once

#include "vkr/utils.h"
#include "vkr/device.h"

namespace vkr
{
	struct VulkanSwapchain
	{
		VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

		VkSurfaceFormatKHR SurfaceFormat;
		VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;

		uint32_t ImageCount;
		VkImage* Images;
		VkImageView* ImageViews;

		VkExtent2D ImageExtent;

		vkr::VulkanDevice VulkanDevice;
		
		VulkanSwapchain(const vkr::VulkanDevice& vulkanDevice, VkSurfaceFormatKHR desiredFormat, VkPresentModeKHR desiredPresentMode, uint32_t windowWidth, uint32_t windowHeight)
		{
			VulkanDevice = vulkanDevice;

			VkSwapchainCreateInfoKHR swapchainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
			swapchainCreateInfo.imageArrayLayers = 1;
			swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapchainCreateInfo.clipped = VK_TRUE;
			swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

			if (VulkanDevice.GraphicsFamilyIndex != VulkanDevice.PresentFamilyIndex)
			{
				uint32_t queueFamilyIndices[] = { VulkanDevice.GraphicsFamilyIndex, VulkanDevice.PresentFamilyIndex };
				swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				swapchainCreateInfo.queueFamilyIndexCount = 2;
				swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else
			{
				swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}

			// Surface support
			VkBool32 surfaceSupported = VK_FALSE;
			VKR_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(VulkanDevice.PhysicalDevice, VulkanDevice.GraphicsFamilyIndex, VulkanDevice.Surface, &surfaceSupported), "Surface not supported");
			VKR_ASSERT(surfaceSupported == VK_TRUE && "The physical device does not support this surface");
			swapchainCreateInfo.surface = VulkanDevice.Surface;

			// Query for the physical device surface capabilities
			VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
			VKR_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanDevice.PhysicalDevice, VulkanDevice.Surface, &surfaceCapabilities), "Failed to fetch surface capabilities");

			swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;

			// Image Count (Trying to aqcuire minImageCount + 1 images)
			VKR_ASSERT(surfaceCapabilities.minImageCount >= 2 && "The physical device does not support double buffering");
			ImageCount = surfaceCapabilities.minImageCount + 1;
			if (surfaceCapabilities.maxImageCount > 0 && ImageCount > surfaceCapabilities.maxImageCount)
			{
				ImageCount = surfaceCapabilities.maxImageCount;
			}
			swapchainCreateInfo.minImageCount = ImageCount;

			// Image Format and Color Space
			SurfaceFormat = findBestFormat(desiredFormat);
			swapchainCreateInfo.imageFormat = SurfaceFormat.format;
			swapchainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;

			// Image Extent
			if (surfaceCapabilities.currentExtent.width != UINT32_MAX)
			{
				ImageExtent = surfaceCapabilities.currentExtent;
			}
			else
			{
				// TODO: Replace this with std::min/std::max
				ImageExtent.width = windowWidth >= surfaceCapabilities.minImageExtent.width && windowWidth <= surfaceCapabilities.maxImageExtent.width ? windowWidth : surfaceCapabilities.maxImageExtent.width;
				ImageExtent.height = windowHeight >= surfaceCapabilities.minImageExtent.height && windowHeight <= surfaceCapabilities.maxImageExtent.height ? windowHeight : surfaceCapabilities.maxImageExtent.height;
			}
			swapchainCreateInfo.imageExtent = ImageExtent;

			// Present Mode
			uint32_t presentModeCount = 0;
			VKR_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(VulkanDevice.PhysicalDevice, VulkanDevice.Surface, &presentModeCount, nullptr), "Failed to enumerate present modes");
			VKR_ASSERT(presentModeCount > 0 && "This physical device doesn't support any present mode");
			VkPresentModeKHR* presentModes = new VkPresentModeKHR[presentModeCount];
			VKR_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(VulkanDevice.PhysicalDevice, VulkanDevice.Surface, &presentModeCount, &presentModes[0]), "Failed to enumerate present modes");

			for (uint32_t i = 0; i < presentModeCount; ++i)
			{
				if (presentModes[i] == desiredPresentMode)
				{
					PresentMode = desiredPresentMode;
					break;
				}
			}
			swapchainCreateInfo.presentMode = PresentMode;

			VKR_CHECK(vkCreateSwapchainKHR(VulkanDevice.Device, &swapchainCreateInfo, nullptr, &Swapchain), "Failed to create the swapchain");

			// Create the images for double/triple buffering
			// NOTE: Although we specified a minImageCount when we've created the swapchain, the driver implementation is allowed to create
			// more than `minImageCount`. For this reason we have to query the image count
			uint32_t imageCount = 0;
			VKR_CHECK(vkGetSwapchainImagesKHR(VulkanDevice.Device, Swapchain, &imageCount, nullptr), "Failed to get the image count");
			VKR_ASSERT(imageCount >= ImageCount);
			ImageCount = imageCount;
			Images = new VkImage[ImageCount];
			VKR_CHECK(vkGetSwapchainImagesKHR(VulkanDevice.Device, Swapchain, &imageCount, &Images[0]), "Failed to get the images");

			ImageViews = new VkImageView[ImageCount];
			for (uint32_t i = 0; i < ImageCount; ++i)
			{
				VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
				imageViewCreateInfo.image = Images[i];
				imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageViewCreateInfo.format = SurfaceFormat.format;
				imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
				imageViewCreateInfo.subresourceRange.levelCount = 1;
				imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
				imageViewCreateInfo.subresourceRange.layerCount = 1;

				VKR_CHECK(vkCreateImageView(VulkanDevice.Device, &imageViewCreateInfo, nullptr, &ImageViews[i]), "Failed to create image view");
			}
		}

		~VulkanSwapchain()
		{

		}

		void Destroy()
		{
			if (ImageCount > 0)
			{
				for (uint32_t i = 0; i < ImageCount; ++i)
				{
					vkDestroyImageView(VulkanDevice.Device, ImageViews[i], nullptr);
				}

				delete[] ImageViews;
				delete[] Images;
			}

			if (Swapchain != VK_NULL_HANDLE)
			{
				vkDestroySwapchainKHR(VulkanDevice.Device, Swapchain, nullptr);
				Swapchain = VK_NULL_HANDLE;
			}
		}

	private:

		VkSurfaceFormatKHR findBestFormat(VkSurfaceFormatKHR desiredFormat)
		{
			VkSurfaceFormatKHR bestFormat = { VK_FORMAT_UNDEFINED };

			// Image Format and Color Space support
			uint32_t formatCount = 0;
			VKR_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(VulkanDevice.PhysicalDevice, VulkanDevice.Surface, &formatCount, nullptr), "Failed to fetch physical device surface formats");
			VKR_ASSERT(formatCount > 0 && "No formats supported");
			VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[formatCount];
			VKR_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(VulkanDevice.PhysicalDevice, VulkanDevice.Surface, &formatCount, &surfaceFormats[0]), "Failed to fetch physical device surface formats");

			if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				bestFormat = desiredFormat;
			}
			else
			{
				for (uint32_t i = 0; i < formatCount; ++i)
				{
					if (surfaceFormats[i].format == desiredFormat.format && surfaceFormats[i].colorSpace == desiredFormat.colorSpace)
					{
						bestFormat = desiredFormat;
						break;
					}
				}

				if (bestFormat.format == VK_FORMAT_UNDEFINED)
				{
					for (uint32_t i = 0; i < formatCount; ++i)
					{
						if (surfaceFormats[i].format == desiredFormat.format)
						{
							bestFormat.format = desiredFormat.format;
							bestFormat.colorSpace = surfaceFormats[i].colorSpace;
							break;
						}
					}
				}

				if (bestFormat.format == VK_FORMAT_UNDEFINED)
				{
					bestFormat.format = surfaceFormats[0].format;
					bestFormat.colorSpace = surfaceFormats[0].colorSpace;
				}
			}

			delete[] surfaceFormats;

			return bestFormat;
		}
	};
}