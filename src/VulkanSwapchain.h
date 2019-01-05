#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

void SetupSwapChain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t queueFamilyIndex,
	uint32_t* outWidth, uint32_t* outHeight, VkSwapchainKHR* outSwapChain,
	std::vector<VkImage>* outPresentImage, std::vector<VkImageView>* outPresentImageView)
{
	uint32_t imageCount = 0;
	VkFormat actualImageFormat;

	// SwapChain creation
	{
		VkSwapchainCreateInfoKHR scci = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };

		// Surface support
		{
			VkBool32 surfaceSupported = VK_FALSE;
			VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, &surfaceSupported));
			R_ASSERT(surfaceSupported == VK_TRUE && "The physical device does not have support for this surface");

			scci.surface = surface;
		}

		// Query for the physical device surface capabilities
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities));

		// Image count support (for double or triple buffering)
		{
			// We need at least 2 image for double buffering
			R_ASSERT(surfaceCapabilities.minImageCount >= 2 && "The surface does not support 2 images for double buffering");
			imageCount = surfaceCapabilities.maxImageCount >= surfaceCapabilities.minImageCount + 1 ? surfaceCapabilities.minImageCount + 1 : surfaceCapabilities.maxImageCount;

			scci.minImageCount = imageCount;
		}

		// Image Format and Color Space support
		{
			uint32_t formatCount;
			VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));
			R_ASSERT(formatCount > 0 && "No surface formats found for the physical device.");
			std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
			VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()));
			VkSurfaceFormatKHR desiredFormat { VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };

			scci.imageFormat = VK_FORMAT_UNDEFINED;

			if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				scci.imageFormat = desiredFormat.format;
				scci.imageColorSpace = desiredFormat.colorSpace;
			}
			else
			{
				for (uint32_t i = 0; i < formatCount; ++i)
				{
					if (surfaceFormats[i].format == desiredFormat.format && surfaceFormats[i].colorSpace == desiredFormat.colorSpace)
					{
						scci.imageFormat = desiredFormat.format;
						scci.imageColorSpace = desiredFormat.colorSpace;
						break;
					}
				}

				if (scci.imageFormat == VK_FORMAT_UNDEFINED)
				{
					for (uint32_t i = 0; i < formatCount; ++i)
					{
						if (surfaceFormats[i].format == desiredFormat.format)
						{
							scci.imageFormat = desiredFormat.format;
							scci.imageColorSpace = surfaceFormats[i].colorSpace;
							break;
						}
					}
				}

				if (scci.imageFormat == VK_FORMAT_UNDEFINED)
				{
					scci.imageFormat = surfaceFormats[0].format;
					scci.imageColorSpace = surfaceFormats[0].colorSpace;
				}
			}

			actualImageFormat = scci.imageFormat;
		}

		// TODO: We need to check if the currentExtent.width is 0xFFFFFFFF
		VkExtent2D surfaceResolution = surfaceCapabilities.currentExtent;
		*outWidth = surfaceResolution.width;
		*outHeight = surfaceResolution.height;
		scci.imageExtent = surfaceResolution;

		scci.imageArrayLayers = 1;
		scci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		// TODO: This depends on the number and indices of family queues
		scci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		scci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		// TODO: Do we need to query for this capability?
		scci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		// Present mode support
		{
			VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

			uint32_t presentModeCount = 0;
			VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
			R_ASSERT(presentModeCount > 0 && "This physical device does not support any present mode");
			std::vector<VkPresentModeKHR> presentModes(presentModeCount);
			VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));
			
			for (uint32_t i = 0; i < presentModeCount; ++i)
			{
				if (presentModes[i] == desiredPresentMode)
				{
					scci.presentMode = desiredPresentMode;
					break;
				}
			}

			if (scci.presentMode != desiredPresentMode)
			{
				scci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
			}
		}

		scci.clipped = VK_TRUE;
		scci.oldSwapchain = nullptr;

		VK_CHECK(vkCreateSwapchainKHR(device, &scci, nullptr, outSwapChain));
	}

	R_ASSERT(imageCount >= 2);

	// Create the images for the double/triple buffering
	outPresentImage->resize(imageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(device, *outSwapChain, &imageCount, outPresentImage->data()));

	// Create the image views for the double/triple buffering
	outPresentImageView->resize(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		VkImageViewCreateInfo ivci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ivci.format = actualImageFormat;
		ivci.components.r = VK_COMPONENT_SWIZZLE_R;
		ivci.components.g = VK_COMPONENT_SWIZZLE_G;
		ivci.components.b = VK_COMPONENT_SWIZZLE_B;
		ivci.components.a = VK_COMPONENT_SWIZZLE_A;
		ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ivci.subresourceRange.baseMipLevel = 0;
		ivci.subresourceRange.levelCount = 1;
		ivci.subresourceRange.baseArrayLayer = 0;
		ivci.subresourceRange.layerCount = 1;
		ivci.image = (*outPresentImage)[i];

		VK_CHECK(vkCreateImageView(device, &ivci, nullptr, &(*outPresentImageView)[i]));
	}
}

void DestroySwapChain(VkDevice device, VkSwapchainKHR* outSwapchain, std::vector<VkImage>* outPresentImage, std::vector<VkImageView>* outPresentImageView)
{
	for (size_t i = 0; i < outPresentImageView->size(); ++i)
	{
		vkDestroyImageView(device, (*outPresentImageView)[i], nullptr);
	}

	/*
	for (size_t i = 0; i < outPresentImage->size(); ++i)
	{
		vkDestroyImage(device, (*outPresentImage)[i], nullptr);
	}
	*/

	vkDestroySwapchainKHR(device, *outSwapchain, nullptr);
	*outSwapchain = nullptr;
}

#endif // VULKAN_SWAPCHAIN_H_ 
