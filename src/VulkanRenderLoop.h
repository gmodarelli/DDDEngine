#ifndef VULKAN_RENDER_LOOP_H_
#define VULKAN_RENDER_LOOP_H_

void RecordCommands(Command commandBuffer, Buffer vertexInputBuffer, uint32_t triangleCount, std::vector<VkFramebuffer> framebuffers, VkRenderPass renderPass, Descriptor descriptor, Pipeline pipeline, uint32_t width, uint32_t height)
{
	for (uint32_t i = 0; i < commandBuffer.CommandBufferCount; ++i)
	{
		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VK_CHECK(vkBeginCommandBuffer(commandBuffer.CommandBuffers[i], &beginInfo));

		// Activate the render pass
		VkClearValue clearValue[] = { { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } };
		VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = framebuffers[i];
		renderPassBeginInfo.renderArea = { { 0, 0 }, { width, height } };
		renderPassBeginInfo.clearValueCount = ARRAYSIZE(clearValue);
		renderPassBeginInfo.pClearValues = clearValue;
		
		vkCmdBeginRenderPass(commandBuffer.CommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind the graphics pipeline to the command buffer. 
		// Any vkDraw command afterward is affected by this pipeline.
		vkCmdBindPipeline(commandBuffer.CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Pipeline);

		// Take care of the dynamic state (what does it mean?)
		VkViewport viewport = { 0, 0, (float)width, (float)height, 0, 1 };
		vkCmdSetViewport(commandBuffer.CommandBuffers[i], 0, 1, &viewport);
		VkRect2D scissors = { 0, 0, width, height };
		vkCmdSetScissor(commandBuffer.CommandBuffers[i], 0, 1, &scissors);

		// Bind the shaders parameters
		vkCmdBindDescriptorSets(commandBuffer.CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.PipelineLayout, 0, descriptor.DescriptorSetCount, descriptor.DescriptorSets.data(), 0, nullptr);

		// Render the triangles
		VkDeviceSize offsets = { 0 };
		vkCmdBindVertexBuffers(commandBuffer.CommandBuffers[i], 0, 1, &vertexInputBuffer.Buffer, &offsets);

		vkCmdDraw(commandBuffer.CommandBuffers[i], triangleCount * 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer.CommandBuffers[i]);

		vkEndCommandBuffer(commandBuffer.CommandBuffers[i]);
	}
}

void RenderLoop(VkDevice device, VkSwapchainKHR swapchain, Command commandBuffer, std::vector<VkImage> presentImages, uint32_t queueFamilyIndex)
{
#if 1
	uint32_t nextImageIndex;
	VkSemaphore semaphore;
	VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore);

	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &nextImageIndex));

	// Present
	VkFence renderFence;
	VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &renderFence));

	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkQueue presentQueue;
	vkGetDeviceQueue(device, queueFamilyIndex, 0, &presentQueue);

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = VK_NULL_HANDLE;
	submitInfo.pWaitDstStageMask = &waitStageMask;
	submitInfo.commandBufferCount = 1; // commandBuffer.bufferCount;
	submitInfo.pCommandBuffers = &commandBuffer.CommandBuffers[nextImageIndex];
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = VK_NULL_HANDLE;

	// Submit command buffer
	vkQueueSubmit(presentQueue, 1, &submitInfo, renderFence);

	// Wait for the command queue to finish
	vkWaitForFences(device, 1, &renderFence, VK_TRUE, UINT64_MAX);

	vkDestroyFence(device, renderFence, nullptr);
	vkDestroySemaphore(device, semaphore, nullptr);

	// Present the backbuffer
	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &nextImageIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(presentQueue, &presentInfo);
#endif
}

#endif // VULKAN_RENDER_LOOP_H_ 
