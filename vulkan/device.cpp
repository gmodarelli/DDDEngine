#include "device.h"
#include <cassert>
#include <stdio.h>

namespace Vulkan
{

Device::Device(WSI* wsi, Context* context) : wsi(wsi), context(context)
{
	frame_resources = new FrameResources[MAX_FRAMES_IN_FLIGHT];
}

void Device::init()
{
	create_render_pass();
	create_command_pools();
	create_vertex_index_buffers();
	create_depth_buffer();
	allocate_command_buffers();
	create_sync_objects();
	create_descriptor_pool();
	create_ubo_buffers();
	create_query_pool();
}

void Device::cleanup()
{
	// We need to wait for the operation in the present queue to be finished
	// before we can cleanup our resources. The operations are asynchronous
	// and when this function is called we don't know if they have finished
	// execution, so we wait.
	vkQueueWaitIdle(context->graphics_queue);

	destroy_query_pool();
	destroy_ubo_buffers();
	destroy_descriptor_pool();
	destroy_sync_objects();
	free_command_buffers();
	destroy_depth_buffer();
	free_vertex_index_buffers();
	destroy_command_pools();
	destroy_framebuffers();
	destroy_render_pass();
}

VkDeviceSize Device::upload_vertex_buffer(Vulkan::Buffer* staging_buffer)
{
	VkDeviceSize vertex_offset = vertex_head_cursor;
	// NOTE: For now we're copying the whole content of the buffer (staging_buffer->size)
	// from its start offset (0). In the future we might want to take both information
	// as input
	VkCommandBuffer copy_cmd = create_transfer_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkBufferCopy copy_region = {};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = vertex_head_cursor;
	copy_region.size = staging_buffer->size;

	vkCmdCopyBuffer(copy_cmd, staging_buffer->buffer, vertex_buffer->buffer, 1, &copy_region);

	flush_transfer_command_buffer(copy_cmd);

	vertex_head_cursor += staging_buffer->size;

	return vertex_offset;
}

VkDeviceSize Device::upload_index_buffer(Vulkan::Buffer* staging_buffer)
{
	VkDeviceSize index_offset = index_head_cursor;
	// NOTE: For now we're copying the whole content of the buffer (staging_buffer->size)
	// from its start offset (0). In the future we might want to take both information
	// as input
	VkCommandBuffer copy_cmd = create_transfer_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkBufferCopy copy_region = {};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = index_head_cursor;
	copy_region.size = staging_buffer->size;

	vkCmdCopyBuffer(copy_cmd, staging_buffer->buffer, index_buffer->buffer, 1, &copy_region);

	flush_transfer_command_buffer(copy_cmd);

	index_head_cursor += staging_buffer->size;

	return index_offset;
}

void Device::create_vertex_index_buffers()
{
	// TODO: Figure out the needed size
	// NOTE: 10 MB
	uint64_t size = 10 * 1024 * 1024;
	// We'll transfer staging buffers data into it
	// using offsets and alignments.
	// https://developer.nvidia.com/vulkan-memory-management
	vertex_buffer = new Vulkan::Buffer(
		context->device,
		context->gpu,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		size);

	index_buffer = new Vulkan::Buffer(
		context->device,
		context->gpu,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		size);
}

void Device::free_vertex_index_buffers()
{
	vertex_buffer->destroy(context->device);
	index_buffer->destroy(context->device);
}

void Device::transition_image_layout(VkImage image, VkFormat format, VkImageLayout src_layout, VkImageLayout dst_layout)
{
	VkCommandBuffer command_buffer = create_transfer_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;

	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.oldLayout = src_layout;
	barrier.newLayout = dst_layout;
	barrier.srcQueueFamilyIndex = context->transfer_family_index;
	barrier.dstQueueFamilyIndex = context->graphics_family_index;
	barrier.image = image;

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (dst_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	// TODO: Add more layout transition stages when we start working with textures
	if (src_layout == VK_IMAGE_LAYOUT_UNDEFINED && dst_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		assert(!"unsupported layout transition");
	}

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	flush_transfer_command_buffer(command_buffer);
}

VkCommandBuffer Device::create_transfer_command_buffer(VkCommandBufferLevel level, bool begin)
{
	VkCommandBufferAllocateInfo command_buffer_ai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	command_buffer_ai.commandPool = transfer_command_pool;
	command_buffer_ai.level = level;
	command_buffer_ai.commandBufferCount = 1;

	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	VkResult result = vkAllocateCommandBuffers(context->device, &command_buffer_ai, &command_buffer);
	assert(result == VK_SUCCESS);

	if (begin)
	{
		VkCommandBufferBeginInfo command_buffer_bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		vkBeginCommandBuffer(command_buffer, &command_buffer_bi);
	}

	return command_buffer;
}

void Device::flush_transfer_command_buffer(VkCommandBuffer& command_buffer, bool free)
{
	VkResult result = vkEndCommandBuffer(command_buffer);
	assert(result == VK_SUCCESS);

	VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	VkFenceCreateInfo fence_ci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	VkFence fence = VK_NULL_HANDLE;
	result = vkCreateFence(context->device, &fence_ci, nullptr, &fence);
	assert(result == VK_SUCCESS);

	result = vkQueueSubmit(context->transfer_queue, 1, &submit_info, fence);
	assert(result == VK_SUCCESS);

	result = vkWaitForFences(context->device, 1, &fence, VK_TRUE, UINT64_MAX);
	assert(result == VK_SUCCESS);

	vkDestroyFence(context->device, fence, nullptr);

	if (free)
	{
		vkFreeCommandBuffers(context->device, transfer_command_pool, 1, &command_buffer);
		command_buffer = VK_NULL_HANDLE;
	}
}

VkResult Device::allocate_descriptor_set(const VkDescriptorSetLayout& descriptor_set_layout, VkDescriptorSet& descriptor_set)
{
	VkDescriptorSetAllocateInfo descriptor_set_ai = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptor_set_ai.descriptorPool = descriptor_pool;
	descriptor_set_ai.descriptorSetCount = 1;
	descriptor_set_ai.pSetLayouts = &descriptor_set_layout;

	return vkAllocateDescriptorSets(context->device, &descriptor_set_ai, &descriptor_set);
}

FrameResources& Device::begin_draw_frame()
{
	FrameResources& current_frame = frame_resources[frame_index];

	// We wait on the current frame fence to be signalled (ie commands have finished execution on
	// the graphics queue for the frame).
	vkWaitForFences(context->device, 1, &current_frame.drawing_finished_fence, VK_TRUE, UINT64_MAX);
	vkResetFences(context->device, 1, &current_frame.drawing_finished_fence);

	if (current_frame.framebuffer != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(context->device, current_frame.framebuffer, nullptr);
		current_frame.framebuffer = VK_NULL_HANDLE;
	}

	// Acquire a swapchain image
	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(context->device, wsi->swapchain, UINT64_MAX, current_frame.image_acquired_semaphore, VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || wsi->window_resized())
	{
		wsi->recreate_swapchain();
		// TODO: Should we recreate the render pass as well?
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		assert(!"Failed to acquire the next image");
	}

	current_frame.image_index = image_index;

	// Prepare the framebuffer
	VkImageView attachments[] = { wsi->swapchain_image_views[image_index] };

	VkFramebufferCreateInfo framebuffer_ci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebuffer_ci.renderPass = render_pass;
	framebuffer_ci.attachmentCount = ARRAYSIZE(attachments);
	framebuffer_ci.pAttachments = attachments;
	framebuffer_ci.width = wsi->swapchain_extent.width;
	framebuffer_ci.height = wsi->swapchain_extent.height;
	framebuffer_ci.layers = 1;

	result = vkCreateFramebuffer(context->device, &framebuffer_ci, nullptr, &current_frame.framebuffer);
	assert(result == VK_SUCCESS);

	VkCommandBufferBeginInfo command_buffer_bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	command_buffer_bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	result = vkBeginCommandBuffer(current_frame.command_buffer, &command_buffer_bi);
	assert(result == VK_SUCCESS);

	vkCmdResetQueryPool(current_frame.command_buffer, current_frame.timestamp_query_pool, 0, current_frame.timestamp_query_pool_count);
	vkCmdWriteTimestamp(current_frame.command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, current_frame.timestamp_query_pool, 0);

	VkRenderPassBeginInfo render_pass_bi = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	render_pass_bi.renderPass = render_pass;
	render_pass_bi.framebuffer = current_frame.framebuffer;
	render_pass_bi.renderArea.offset = { 0, 0 };
	render_pass_bi.renderArea.extent = wsi->swapchain_extent;

	VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
	render_pass_bi.clearValueCount = 1;
	render_pass_bi.pClearValues = &clear_color;

	// VK_SUBPASS_CONTENTS_INLINE means that the render pass commands (like drawing comands)
	// will be embedded in the primary command buffer and no secondary command buffers will
	// be executed
	vkCmdBeginRenderPass(current_frame.command_buffer, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);

	return current_frame;
}

void Device::end_draw_frame(FrameResources& current_frame)
{
	vkCmdEndRenderPass(current_frame.command_buffer);

	vkCmdWriteTimestamp(current_frame.command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, current_frame.timestamp_query_pool, 1);
	uint64_t timestamp_results[2] = {};
	VkResult result = vkGetQueryPoolResults(context->device, current_frame.timestamp_query_pool, 0, ARRAYSIZE(timestamp_results), sizeof(timestamp_results), timestamp_results, sizeof(timestamp_results[0]), VK_QUERY_RESULT_64_BIT);

	double frame_gpu_begin = double(timestamp_results[0]) * context->gpu_properties.limits.timestampPeriod * 1e-6;
	double frame_gpu_end = double(timestamp_results[1]) * context->gpu_properties.limits.timestampPeriod * 1e-6;
	frame_gpu_avg = frame_gpu_avg * 0.95 + (frame_gpu_end - frame_gpu_begin) * 0.05;

	result = vkEndCommandBuffer(current_frame.command_buffer);
	assert(result == VK_SUCCESS);

	VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

	// Which semaphores to wait on before execution begins and in which stages of the pipeline to wait.
	// We want to wait with writing colors to the image until it's available.
	VkSemaphore wait_semaphores[] = { current_frame.image_acquired_semaphore };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = ARRAYSIZE(wait_semaphores);
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

	// The command buffer to submit for execution. We submit the command buffer that
	// binds the swapchain image we acquired with vkAcquireNextImageKHR
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &current_frame.command_buffer;

	// Specify the semaphores to signal once the command buffers have finished execution
	VkSemaphore signal_semaphores[] = { current_frame.ready_to_present_semaphore };
	submit_info.signalSemaphoreCount = ARRAYSIZE(signal_semaphores);
	submit_info.pSignalSemaphores = signal_semaphores;

	// Submit the command buffer to the graphics queue with the current frame fence to signal once
	// execution has finished
	result = vkQueueSubmit(context->graphics_queue, 1, &submit_info, current_frame.drawing_finished_fence);
	assert(result == VK_SUCCESS);

	VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };

	// Specify which semaphores to wait on before presentation can happen
	present_info.waitSemaphoreCount = ARRAYSIZE(signal_semaphores);
	present_info.pWaitSemaphores = signal_semaphores;

	// Specify the swapchains to present the images to and the index of the image
	// for each swapchain. Most likely a single one
	VkSwapchainKHR swapchains[] = { wsi->swapchain };
	present_info.swapchainCount = ARRAYSIZE(swapchains);
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &current_frame.image_index;

	result = vkQueuePresentKHR(context->graphics_queue, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || wsi->window_resized())
	{
		wsi->recreate_swapchain();
		// TODO: Should we recreate the render pass as well?
	}
	else if (result != VK_SUCCESS)
	{
		assert(!"Failed to acquire the next image");
	}

	frame_index = (frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

// Internal

void Device::create_render_pass()
{
	// We have a single color buffer attachment represented by one of the
	// images from the swapchain.
	VkAttachmentDescription color_attachment = {};
	// Its format should match the format of the swapchain images (that we
	// get from the surface).
	color_attachment.format = wsi->surface_format.format;
	// Since we're not doing any multisampling, we'll only have 1 sample
	// per pixel for now
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// Specify what to do with the data in the attachment before and after
	// rendering.
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear the attachment when loading it
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// For now we don't have a depth/stencil buffer so we don't care
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// For now we're just presenting the color attachment to the framebuffer,
	// so we are not expecting a specify initial layout and we're gonna transition
	// to present source as final layout
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkRenderPassCreateInfo render_pass_ci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	render_pass_ci.attachmentCount = 1;
	render_pass_ci.pAttachments = &color_attachment;
	render_pass_ci.subpassCount = 1;
	render_pass_ci.pSubpasses = &subpass;

	// Make the render pass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage
	// We specify VK_SUBPASS_EXTERNAL to refer to the implicity subpass before the render pass
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	// We need to wait for the swapchain to finish reading from the image before we can access it,
	// so we wait on the color attachment output stage.
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	// The operation that should wait on this are reading and writing of the color attachment in the 
	// color attachment stage. These settings will prevent the transition from happening until it is
	// necessary and allowed: when we want to start writing colors
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	render_pass_ci.dependencyCount = 1;
	render_pass_ci.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(context->device, &render_pass_ci, nullptr, &render_pass);
	assert(result == VK_SUCCESS);
}

void Device::destroy_render_pass()
{
	if (render_pass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(context->device, render_pass, nullptr);
		render_pass = VK_NULL_HANDLE;
	}
}

void Device::create_depth_buffer()
{
	VkFormat depth_format = find_supported_format(VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depth_buffer = new Vulkan::Image(context->device, context->gpu, wsi->swapchain_extent, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	transition_image_layout(depth_buffer->image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

VkFormat Device::find_supported_format(VkImageTiling tiling, VkFormatFeatureFlags features)
{
	VkFormat candidates[] =
	{
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
	};

	for (uint32_t i = 0; i < ARRAYSIZE(candidates); ++i)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(context->gpu, candidates[i], &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
		{
			return candidates[i];
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
		{
			return candidates[i];
		}
	}

	assert(!"Failed to find supported format");
}


void Device::destroy_depth_buffer()
{
	if (depth_buffer != nullptr)
	{
		depth_buffer->destroy(context->device);
	}
}


void Device::destroy_framebuffers()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (frame_resources[i].framebuffer != VK_NULL_HANDLE)
			vkDestroyFramebuffer(context->device, frame_resources[i].framebuffer, nullptr);
	}
}

// Command pool and command buffer helpers
void Device::create_command_pools()
{
	VkCommandPoolCreateInfo command_pool_ci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	command_pool_ci.queueFamilyIndex = context->graphics_family_index;
	// Flags values
	// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
	// VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
	command_pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkResult result = vkCreateCommandPool(context->device, &command_pool_ci, nullptr, &command_pool);
	assert(result == VK_SUCCESS);

	command_pool_ci.queueFamilyIndex = context->transfer_family_index;

	result = vkCreateCommandPool(context->device, &command_pool_ci, nullptr, &transfer_command_pool);
	assert(result == VK_SUCCESS);
}

void Device::destroy_command_pools()
{
	if (command_pool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(context->device, command_pool, nullptr);
		command_pool = VK_NULL_HANDLE;
	}

	if (transfer_command_pool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(context->device, transfer_command_pool, nullptr);
		transfer_command_pool = VK_NULL_HANDLE;
	}
}

void Device::allocate_command_buffers()
{
	VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];

	VkCommandBufferAllocateInfo command_buffer_ai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	command_buffer_ai.commandPool = command_pool;
	command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_ai.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

	VkResult result = vkAllocateCommandBuffers(context->device, &command_buffer_ai, command_buffers);
	assert(result == VK_SUCCESS);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		frame_resources[i].command_buffer = command_buffers[i];
	}
}

void Device::free_command_buffers()
{
	if (command_pool != VK_NULL_HANDLE)
	{
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			vkFreeCommandBuffers(context->device, command_pool, 1, &frame_resources[i].command_buffer);
		}
	}
}

// Synchronization Objects helpers
void Device::create_sync_objects()
{
	VkSemaphoreCreateInfo semaphore_ci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fence_ci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	// We create the fence in a signaled state cause we wait on them before being able to 
	// draw a frame, and since frame 0 doesn't have a previous frame that was submitted,
	// it will wait indefinitely
	fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkResult result = vkCreateSemaphore(context->device, &semaphore_ci, nullptr, &frame_resources[i].image_acquired_semaphore);
		assert(result == VK_SUCCESS);

		result = vkCreateSemaphore(context->device, &semaphore_ci, nullptr, &frame_resources[i].ready_to_present_semaphore);
		assert(result == VK_SUCCESS);

		result = vkCreateFence(context->device, &fence_ci, nullptr, &frame_resources[i].drawing_finished_fence);
		assert(result == VK_SUCCESS);
	}
}

void Device::destroy_sync_objects()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(context->device, frame_resources[i].image_acquired_semaphore, nullptr);
		vkDestroySemaphore(context->device, frame_resources[i].ready_to_present_semaphore, nullptr);
		vkDestroyFence(context->device, frame_resources[i].drawing_finished_fence, nullptr);
	}
}

// Query Pool helpers

void Device::create_query_pool()
{
	uint32_t query_count = 16;

	VkQueryPoolCreateInfo create_info = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
	create_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
	create_info.queryCount = query_count;

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkResult result = vkCreateQueryPool(context->device, &create_info, nullptr, &frame_resources[i].timestamp_query_pool);
		assert(result == VK_SUCCESS);
		frame_resources[i].timestamp_query_pool_count = query_count;
	}
}

void Device::destroy_query_pool()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroyQueryPool(context->device, frame_resources[i].timestamp_query_pool, nullptr);
	}
}

// Descriptor Pool helpers
void Device::create_descriptor_pool()
{
	VkDescriptorPoolSize pool_size = {};
	pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_size.descriptorCount = MAX_FRAMES_IN_FLIGHT;

	VkDescriptorPoolCreateInfo pool_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_ci.poolSizeCount = 1;
	pool_ci.pPoolSizes = &pool_size;
	pool_ci.maxSets = MAX_FRAMES_IN_FLIGHT;

	VkResult result = vkCreateDescriptorPool(context->device, &pool_ci, nullptr, &descriptor_pool);
	assert(result == VK_SUCCESS);
}

void Device::destroy_descriptor_pool()
{
	if (descriptor_pool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(context->device, descriptor_pool, nullptr);
		descriptor_pool = VK_NULL_HANDLE;
	}
}

// UBO Buffers Helpers
void Device::create_ubo_buffers()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		frame_resources[i].ubo_buffer = new Vulkan::Buffer(
			context->device,
			context->gpu,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(float) * 16 * 3);
	}
}

void Device::destroy_ubo_buffers()
{

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		frame_resources[i].ubo_buffer->destroy(context->device);
	}
}

} // namsepace Vulkan