#ifndef VULKAN_RENDER_LOOP_H_
#define VULKAN_RENDER_LOOP_H_

void RecordCommands(VkDevice device, SyncObjects syncObjects, VkCommandBuffer commandBuffer, Buffer vertexBuffer, Buffer indexBuffer, vkr::Scene scene, std::vector<VkFramebuffer> framebuffers, VkRenderPass renderPass, Descriptor descriptor, Pipeline pipeline, VkQueryPool queryPool, uint32_t width, uint32_t height, uint32_t currentFrameIndex)
{
	vkWaitForFences(device, 1, &syncObjects.InFlightFences[currentFrameIndex], VK_TRUE, UINT64_MAX);

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	// Use the ith and i+1th queries from the query pool
	vkCmdResetQueryPool(commandBuffer, queryPool, 2 * currentFrameIndex, 2);
	vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 2 * currentFrameIndex);

	// Activate the render pass
	VkClearValue clearValue[] = { { 135 / 255.0f, 206 / 255.0f, 250 / 255.0f, 1.0f }, { 1.0f, 0.0f } };
	VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = framebuffers[currentFrameIndex];
	renderPassBeginInfo.renderArea = { { 0, 0 }, { width, height } };
	renderPassBeginInfo.clearValueCount = ARRAYSIZE(clearValue);
	renderPassBeginInfo.pClearValues = clearValue;
	
	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Bind the graphics pipeline to the command buffer. 
	// Any vkDraw command afterward is affected by this pipeline.
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Pipeline);

	// Take care of the dynamic state (what does it mean?)
	VkViewport viewport = { 0, 0, (float)width, (float)height, 0, 1 };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	VkRect2D scissors = { 0, 0, width, height };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissors);

	// Render the triangles
	VkDeviceSize offsets = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.Buffer, &offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

	// Bind the shaders parameters
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.PipelineLayout, 0, descriptor.DescriptorSetCount, descriptor.DescriptorSets.data(), 0, nullptr);

	// Render all meshes in the scene
	for (size_t i = 0; i < scene.meshes.size(); ++i)
	{
		vkCmdDrawIndexed(commandBuffer, scene.meshes[i].IndexCount, 1, 0, scene.meshes[i].IndexBase, 0);
	}

	vkCmdEndRenderPass(commandBuffer);

	vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 2 * currentFrameIndex + 1);
	vkEndCommandBuffer(commandBuffer);
}

double RenderLoop(VkDevice device, VkPhysicalDeviceProperties properties, VkSwapchainKHR swapchain, std::vector<VkCommandBuffer> commandBuffers, VkQueryPool queryPool, VkQueue graphicsQueue, VkQueue presentQueue, SyncObjects syncObjects, uint32_t currentFrameIndex)
{
	uint32_t nextImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, syncObjects.ImageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &nextImageIndex));

	VkCommandBuffer currentCB = commandBuffers[nextImageIndex];

	// Present
	VkSemaphore waitSemaphores[] = { syncObjects.ImageAvailableSemaphores[currentFrameIndex] };
	VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = ARRAYSIZE(waitSemaphores);
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &currentCB;

	VkSemaphore signalSemaphores[] = { syncObjects.RenderFinishedSemaphores[currentFrameIndex] };
	submitInfo.signalSemaphoreCount = ARRAYSIZE(signalSemaphores);
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device, 1, &syncObjects.InFlightFences[currentFrameIndex]);

	// Submit command buffer
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, syncObjects.InFlightFences[currentFrameIndex]);

	// Present the backbuffer
	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = ARRAYSIZE(signalSemaphores);
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &nextImageIndex;

	VK_CHECK(vkQueuePresentKHR(presentQueue, &presentInfo));

	uint64_t queryResults[2] = {};
	VK_CHECK(vkGetQueryPoolResults(device, queryPool, currentFrameIndex * 2, ARRAYSIZE(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));

	double frameGpuBegin = double(queryResults[0]) * properties.limits.timestampPeriod * 1e-6;
	double frameGpuEnd = double(queryResults[1]) * properties.limits.timestampPeriod * 1e-6;

	return frameGpuEnd - frameGpuBegin;
}

#endif // VULKAN_RENDER_LOOP_H_ 
