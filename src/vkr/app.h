#pragma once

#define ENABLE_VULKAN_DEBUG_CALLBACK
#include "../VulkanTools.h"

namespace vkr
{
	struct App
	{
		vkr::VulkanDevice* device;
		vkr::VulkanSwapchain* swapchain;

		VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
		// This format include stencil as well VK_FORMAT_D16_UNORM_S8_UINT
		VkFormat depthFormat = VK_FORMAT_D16_UNORM;
		VkRenderPass renderPass;

		std::vector<VkFramebuffer> framebuffers;
		struct DepthBuffer
		{
			VkImage Image;
			VkImageView ImageView;
			VkDeviceMemory ImageMemory;
		} depthBuffer;

		uint32_t width;
		uint32_t height;

		App(WindowParameters windowParams)
		{
			this->width = width;
			this->height = height;

			VkQueueFlags requiredQueues = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
			device = new vkr::VulkanDevice(windowParams, requiredQueues);
		}

		~App()
		{
			if (depthBuffer.Image != VK_NULL_HANDLE)
			{
				vkDestroyImageView(device->Device, depthBuffer.ImageView, nullptr);
				vkDestroyImage(device->Device, depthBuffer.Image, nullptr);
				vkFreeMemory(device->Device, depthBuffer.ImageMemory, nullptr);

				depthBuffer.Image = VK_NULL_HANDLE;
				depthBuffer.ImageView = VK_NULL_HANDLE;
				depthBuffer.ImageMemory = { 0 };
			}

			for (uint32_t i = 0; i < framebuffers.size(); ++i)
			{
				vkDestroyFramebuffer(device->Device, framebuffers[i], nullptr);
			}

			if (renderPass != VK_NULL_HANDLE)
			{
				vkDestroyRenderPass(device->Device, renderPass, nullptr);
				renderPass = VK_NULL_HANDLE;
			}

			swapchain->Destroy();
			device->destroy();
		}

		void prepare()
		{
			VkSurfaceFormatKHR desiredFormat { VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
			VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

			swapchain = new vkr::VulkanSwapchain(*device, desiredFormat, desiredPresentMode, width, height);

			prepareRenderPass();
			prepareDepthBuffer();
			prepareFrameBuffers();
		}

	private:

		void prepareRenderPass()
		{
			VkAttachmentDescription pass[2] = {};

			// 0 - Color screen buffer
			pass[0].format = colorFormat;
			pass[0].samples = VK_SAMPLE_COUNT_1_BIT;
			pass[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			pass[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			pass[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			pass[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			pass[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			pass[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			
			// 1 - Depth buffer
			pass[1].format = depthFormat;
			pass[1].samples = VK_SAMPLE_COUNT_1_BIT;
			pass[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			pass[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			pass[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			pass[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			pass[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			pass[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef = {};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// Create one main subpass for the render pass
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			// Create the main render pass
			VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
			renderPassInfo.attachmentCount = 2;
			renderPassInfo.pAttachments = pass;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;

			VKR_CHECK(vkCreateRenderPass(device->Device, &renderPassInfo, nullptr, &renderPass), "Failed to create the render pass");
		}

		void prepareDepthBuffer()
		{
			// Create a depth image
			VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = depthFormat;  
			imageInfo.extent = { swapchain->ImageExtent.width, swapchain->ImageExtent.height, 1 };
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.queueFamilyIndexCount = 0;
			imageInfo.pQueueFamilyIndices = nullptr;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			VKR_CHECK(vkCreateImage(device->Device, &imageInfo, nullptr, &depthBuffer.Image), "Failed to create depth buffer image");

			// Query for the memory requirements of the depth buffer
			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(device->Device, depthBuffer.Image, &memoryRequirements);

			// Allocate memory for the depth buffer
			VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
			allocateInfo.allocationSize = memoryRequirements.size;
			allocateInfo.memoryTypeIndex = vkr::findMemoryType(device->PhysicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			depthBuffer.ImageMemory = { 0 };
			VKR_CHECK(vkAllocateMemory(device->Device, &allocateInfo, nullptr, &depthBuffer.ImageMemory), "Failed to allocate memory for the depth buffer");
			VKR_CHECK(vkBindImageMemory(device->Device, depthBuffer.Image, depthBuffer.ImageMemory, 0), "Faild to bind memory to the depth buffer");

			// Create the depth image view
			VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			VkImageViewCreateInfo imageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			imageViewInfo.image = depthBuffer.Image;
			imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewInfo.format = depthFormat;
			imageViewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
			imageViewInfo.subresourceRange.aspectMask = aspectMask;
			imageViewInfo.subresourceRange.baseMipLevel = 0;
			imageViewInfo.subresourceRange.levelCount = 1;
			imageViewInfo.subresourceRange.baseArrayLayer = 0;
			imageViewInfo.subresourceRange.layerCount = 1;

			VKR_CHECK(vkCreateImageView(device->Device, &imageViewInfo, nullptr, &depthBuffer.ImageView), "Failed to create depth image view");
		}

		void prepareFrameBuffers()
		{
			framebuffers.resize(swapchain->ImageCount);
			for (uint32_t i = 0; i < swapchain->ImageCount; ++i)
			{
				VkImageView frameBufferAttachments[2] = { 0 };
				frameBufferAttachments[0] = swapchain->ImageViews[i];
				frameBufferAttachments[1] = depthBuffer.ImageView;

				VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
				createInfo.renderPass = renderPass;
				createInfo.attachmentCount = 2;
				createInfo.pAttachments = frameBufferAttachments;
				createInfo.width = swapchain->ImageExtent.width;
				createInfo.height = swapchain->ImageExtent.height;
				createInfo.layers = 1;

				VK_CHECK(vkCreateFramebuffer(device->Device, &createInfo, nullptr, &framebuffers[i]));
			}
		}
	};

}
