#ifndef VULKAN_RENDER_PASS_H_
#define VULKAN_RENDER_PASS_H_

void SetupRenderPass(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, std::vector<VkImageView>* presentImageViews,
	VkRenderPass* outRenderPass, std::vector<VkFramebuffer>* outFrameBuffers, BufferImage* outDepthBufferImage)
{
	{
		// Create a depth image
		VkImageCreateInfo ici = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		ici.imageType = VK_IMAGE_TYPE_2D;
		ici.format = VK_FORMAT_D16_UNORM;  // This format include stencil as well VK_FORMAT_D16_UNORM_S8_UINT
		ici.extent = { width, height, 1 };
		ici.mipLevels = 1;
		ici.arrayLayers = 1;
		ici.samples = VK_SAMPLE_COUNT_1_BIT;
		ici.tiling = VK_IMAGE_TILING_OPTIMAL;
		ici.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ici.queueFamilyIndexCount = 0;
		ici.pQueueFamilyIndices = nullptr;
		ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		
		VK_CHECK(vkCreateImage(device, &ici, nullptr, &outDepthBufferImage->image));

		// Query for the memory requirements of the depth buffer
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, outDepthBufferImage->image, &memoryRequirements);

		// Allocate memory for the depth buffer
		VkMemoryAllocateInfo imageAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		imageAllocateInfo.allocationSize = memoryRequirements.size;

		// Check the memory properties of the physical device
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		uint32_t memoryTypeBits = memoryRequirements.memoryTypeBits;
		for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
		{
			VkMemoryType memoryType = memoryProperties.memoryTypes[i];
			if (memoryTypeBits & 1)
			{
				if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
				{
					// save the intex
					imageAllocateInfo.memoryTypeIndex = i;
					break;
				}
			}
			memoryTypeBits = memoryTypeBits >> 1;
		}

		outDepthBufferImage->imageMemory = { 0 };
		VK_CHECK(vkAllocateMemory(device, &imageAllocateInfo, nullptr, &outDepthBufferImage->imageMemory));
		VK_CHECK(vkBindImageMemory(device, outDepthBufferImage->image, outDepthBufferImage->imageMemory, 0));

		// Create the depth image view
		VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		VkImageViewCreateInfo ivci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ivci.image = outDepthBufferImage->image;
		ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ivci.format = ici.format;
		ivci.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		ivci.subresourceRange.aspectMask = aspectMask;
		ivci.subresourceRange.baseMipLevel = 0;
		ivci.subresourceRange.levelCount = 1;
		ivci.subresourceRange.baseArrayLayer = 0;
		ivci.subresourceRange.layerCount = 1;

		VK_CHECK(vkCreateImageView(device, &ivci, nullptr, &outDepthBufferImage->imageView));
	}


	VkAttachmentDescription pass[2] = {};

	// 0 - Color screen buffer
	// TODO: Take this format as input
	pass[0].format = VK_FORMAT_B8G8R8A8_UNORM;
	pass[0].samples = VK_SAMPLE_COUNT_1_BIT;
	pass[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	pass[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	pass[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	pass[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	pass[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	pass[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference car = {};
	car.attachment = 0;
	car.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	// 1 - Depth buffer
	pass[1].format = VK_FORMAT_D16_UNORM;
	pass[1].samples = VK_SAMPLE_COUNT_1_BIT;
	pass[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	pass[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	pass[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	pass[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	pass[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	pass[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference dar = {};
	dar.attachment = 1;
	dar.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Create one main subpass for the render pass
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &car;
	subpass.pDepthStencilAttachment = &dar;

	// Create the main render pass
	VkRenderPassCreateInfo rpci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	rpci.attachmentCount = 2;
	rpci.pAttachments = pass;
	rpci.subpassCount = 1;
	rpci.pSubpasses = &subpass;

	VK_CHECK(vkCreateRenderPass(device, &rpci, nullptr, outRenderPass));

	outFrameBuffers->resize(presentImageViews->size());
	for (uint32_t i = 0; i < outFrameBuffers->size(); ++i)
	{
		// Create the frame buffers
		VkImageView frameBufferAttachments[2] = { 0 };
		frameBufferAttachments[0] = (*presentImageViews)[i];
		frameBufferAttachments[1] = outDepthBufferImage->imageView;

		VkFramebufferCreateInfo fbci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fbci.renderPass = *outRenderPass;
		fbci.attachmentCount = rpci.attachmentCount;
		fbci.pAttachments = frameBufferAttachments;
		fbci.width = width;
		fbci.height = height;
		fbci.layers = 1;

		VK_CHECK(vkCreateFramebuffer(device, &fbci, nullptr, &(*outFrameBuffers)[i]));
	}
}

void DestroyBufferImage(VkDevice device, BufferImage* bufferImage)
{
	vkFreeMemory(device, bufferImage->imageMemory, nullptr);
	vkDestroyImageView(device, bufferImage->imageView, nullptr);
	vkDestroyImage(device, bufferImage->image, nullptr);

	bufferImage->image = nullptr;
	bufferImage->imageView = nullptr;
	bufferImage->imageMemory = { 0 };
}

void DestroyRenderPass(VkDevice device, VkRenderPass* renderPass, std::vector<VkFramebuffer>* framebuffers)
{
	for (uint32_t i = 0; i < framebuffers->size(); ++i)
	{
		vkDestroyFramebuffer(device, (*framebuffers)[i], nullptr);
	}

	vkDestroyRenderPass(device, *renderPass, nullptr);
	*renderPass = nullptr;
}

#endif // VULKAN_RENDER_PASS_H_
