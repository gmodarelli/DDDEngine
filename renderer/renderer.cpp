#include "renderer.h"
#include "../vulkan/shaders.h"
#include <cassert>
#include <stdio.h>

namespace Renderer
{

Renderer::Renderer(Vulkan::WSI* wsi) : wsi(wsi)
{
}

void Renderer::init()
{
}

void Renderer::render_frame()
{
	if (wsi->resizing())
		return;

	if (recreate_frame_resources)
	{
		create_frame_resources();
		return;
	}

	// We wait on the current frame fence to be signalled (ie commands have finished execution on
	// the graphics queue for the frame).
	vkWaitForFences(wsi->get_device(), 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

	// The first thing we need to do is acquire an image from the swapchain
	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(wsi->get_device(), wsi->get_swapchain(), UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || wsi->window_resized())
	{
		wsi->recreate_swapchain();
		recreate_frame_resources = true;
	} 
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		assert(!"Failed to acquire the next image");
	}

	VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

	// Which semaphores to wait on before execution begins and in which stages of the pipeline to wait.
	// We want to wait with writing colors to the image until it's available.
	VkSemaphore wait_semaphores[] = { image_available_semaphores[current_frame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = ARRAYSIZE(wait_semaphores);
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

	// The command buffer to submit for execution. We submit the command buffer that
	// binds the swapchain image we acquired with vkAcquireNextImageKHR
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffers[image_index];

	// Specify the semaphores to signal once the command buffers have finished execution
	VkSemaphore signal_semaphores[] = { render_finished_semaphores[current_frame] };
	submit_info.signalSemaphoreCount = ARRAYSIZE(signal_semaphores);
	submit_info.pSignalSemaphores = signal_semaphores;

	vkResetFences(wsi->get_device(), 1, &in_flight_fences[current_frame]);

	// Submit the command buffer to the graphics queue with the current frame fence to signal once
	// execution has finished
	result = vkQueueSubmit(wsi->get_graphics_queue(), 1, &submit_info, in_flight_fences[current_frame]);
	assert(result == VK_SUCCESS);

	VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };

	// Specify which semaphores to wait on before presentation can happen
	present_info.waitSemaphoreCount = ARRAYSIZE(signal_semaphores);
	present_info.pWaitSemaphores = signal_semaphores;

	// Specify the swapchains to present the images to and the index of the image
	// for each swapchain. Most likely a single one
	VkSwapchainKHR swapchains[] = { wsi->get_swapchain() };
	present_info.swapchainCount = ARRAYSIZE(swapchains);
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &image_index;

	result = vkQueuePresentKHR(wsi->get_present_queue(), &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || wsi->window_resized())
	{
		wsi->recreate_swapchain();
		recreate_frame_resources = true;
	} 
	else if (result != VK_SUCCESS)
	{
		assert(!"Failed to acquire the next image");
	}

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::create_frame_resources()
{
	vkQueueWaitIdle(wsi->get_present_queue());

	destroy_sync_objects();

	if (command_buffers != nullptr)
	{
		free_command_buffers();
	}

	if (command_pool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(wsi->get_device(), command_pool, nullptr);
		command_pool = VK_NULL_HANDLE;
	}

	if (framebuffers != nullptr)
	{
		destroy_framebuffers();
	}

	if (render_pass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(wsi->get_device(), render_pass, nullptr);
		render_pass = VK_NULL_HANDLE;
	}

	create_render_pass();
	create_framebuffers();
	create_command_pool();
	create_command_buffers();
	create_sync_objects();
	record_commands();

	recreate_frame_resources = false;
}

void Renderer::cleanup()
{
	// We need to wait for the operation in the present queue to be finished
	// before we can cleanup our resources. The operations are asynchronous
	// and when this function is called we don't know if they have finished
	// execution, so we wait.
	vkQueueWaitIdle(wsi->get_present_queue());

	destroy_sync_objects();

	if (command_buffers != nullptr)
	{
		free_command_buffers();
	}

	if (command_pool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(wsi->get_device(), command_pool, nullptr);
		command_pool = VK_NULL_HANDLE;
	}

	if (framebuffers != nullptr)
	{
		destroy_framebuffers();
	}

	if (graphics_pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(wsi->get_device(), graphics_pipeline, nullptr);
		graphics_pipeline = VK_NULL_HANDLE;
	}

	if (pipeline_layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(wsi->get_device(), pipeline_layout, nullptr);
		pipeline_layout = VK_NULL_HANDLE;
	}

	if (render_pass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(wsi->get_device(), render_pass, nullptr);
		render_pass = VK_NULL_HANDLE;
	}
}

void Renderer::create_render_pass()
{
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = wsi->get_surface_format().format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
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


	VkResult result = vkCreateRenderPass(wsi->get_device(), &render_pass_ci, nullptr, &render_pass);
	assert(result == VK_SUCCESS);
}

void Renderer::create_graphics_pipeline()
{
	VkPipelineShaderStageCreateInfo shader_stages[2];
	shader_stages[0] = Vulkan::Shader::load_shader(wsi->get_device(), wsi->get_gpu(), "../data/shaders/simple_triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = Vulkan::Shader::load_shader(wsi->get_device(), wsi->get_gpu(), "../data/shaders/simple_triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Pipeline Fixed Functions
	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertex_input_ci = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertex_input_ci.vertexBindingDescriptionCount = 0;
	vertex_input_ci.vertexAttributeDescriptionCount = 0;
	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_ci.primitiveRestartEnable = VK_FALSE;
	// Viewport and Scissor
	viewport = { 0, 0, (float)wsi->get_width(), (float)wsi->get_height(), 0.0f, 1.0f };
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { (uint32_t)wsi->get_width(), (uint32_t)wsi->get_height() };
	VkPipelineViewportStateCreateInfo viewport_ci = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewport_ci.viewportCount = 1;
	viewport_ci.pViewports = &viewport;
	viewport_ci.scissorCount = 1;
	viewport_ci.pScissors = &scissor;
	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer_ci = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	// If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them 
	// as opposed to discarding them. This is useful in some special cases like shadow maps.
	// Using this requires enabling a GPU feature.
	rasterizer_ci.depthClampEnable = VK_FALSE;
	// If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage.
	// This basically disables any output to the framebuffer.
	rasterizer_ci.rasterizerDiscardEnable = VK_FALSE;
	rasterizer_ci.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer_ci.lineWidth = 1.0f;
	rasterizer_ci.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer_ci.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer_ci.depthBiasEnable = VK_FALSE;
	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling_ci = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling_ci.sampleShadingEnable = VK_FALSE;
	multisampling_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	// Color Blend
	VkPipelineColorBlendAttachmentState color_blend_attachment_ci = {};
	color_blend_attachment_ci.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment_ci.blendEnable = VK_FALSE;
	// NOTE: If we want to enable blend we have to set it up this way
	// color_blend_attachment_ci.blendEnable = VK_TRUE;
	// color_blend_attachment_ci.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	// color_blend_attachment_ci.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	// color_blend_attachment_ci.colorBlendOp = VK_BLEND_OP_ADD;
	// color_blend_attachment_ci.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	// color_blend_attachment_ci.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// color_blend_attachment_ci.alphaBlendOp = VK_BLEND_OP_ADD;
	// 
	VkPipelineColorBlendStateCreateInfo color_blend_ci = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	color_blend_ci.logicOpEnable = VK_FALSE;
	color_blend_ci.attachmentCount = 1;
	color_blend_ci.pAttachments = &color_blend_attachment_ci;
	// Dynamic States (state that can be changed without having to recreate the pipeline)
	VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };
	VkPipelineDynamicStateCreateInfo dynamic_state_ci = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamic_state_ci.dynamicStateCount = ARRAYSIZE(dynamic_states);
	dynamic_state_ci.pDynamicStates = dynamic_states;
	// Pipeline Layout
	VkPipelineLayoutCreateInfo pipeline_layout_ci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	// NOTE: Fill the CI when we have actual uniform data to send to shaders
	VkResult result = vkCreatePipelineLayout(wsi->get_device(), &pipeline_layout_ci, nullptr, &pipeline_layout);
	assert(result == VK_SUCCESS);

	VkGraphicsPipelineCreateInfo pipeline_ci = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipeline_ci.stageCount = ARRAYSIZE(shader_stages);
	pipeline_ci.pStages = shader_stages;
	pipeline_ci.pVertexInputState = &vertex_input_ci;
	pipeline_ci.pInputAssemblyState = &input_assembly_ci;
	pipeline_ci.pViewportState = &viewport_ci;
	pipeline_ci.pRasterizationState = &rasterizer_ci;
	pipeline_ci.pMultisampleState = &multisampling_ci;
	pipeline_ci.pColorBlendState = &color_blend_ci;
	pipeline_ci.pDynamicState = &dynamic_state_ci;
	pipeline_ci.layout = pipeline_layout;
	pipeline_ci.renderPass = render_pass;
	pipeline_ci.subpass = 0;

	result = vkCreateGraphicsPipelines(wsi->get_device(), nullptr, 1, &pipeline_ci, nullptr, &graphics_pipeline);
	assert(result == VK_SUCCESS);

	for (uint32_t i = 0; i < ARRAYSIZE(shader_stages); ++i)
	{
		vkDestroyShaderModule(wsi->get_device(), shader_stages[i].module, nullptr);
	}
}

void Renderer::create_framebuffers()
{
	framebuffer_count = wsi->get_swapchain_image_count();
	framebuffers = new VkFramebuffer[framebuffer_count];
	for (uint32_t i = 0; i < framebuffer_count; ++i)
	{
		VkImageView attachments[] = { wsi->get_swapchain_image_view(i) };

		VkFramebufferCreateInfo framebuffer_ci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebuffer_ci.renderPass = render_pass;
		framebuffer_ci.attachmentCount = ARRAYSIZE(attachments);
		framebuffer_ci.pAttachments = attachments;
		framebuffer_ci.width = wsi->get_width();
		framebuffer_ci.height = wsi->get_height();
		framebuffer_ci.layers = 1;

		VkResult result = vkCreateFramebuffer(wsi->get_device(), &framebuffer_ci, nullptr, &framebuffers[i]);
		assert(result == VK_SUCCESS);
	}
}

void Renderer::create_command_pool()
{
	VkCommandPoolCreateInfo command_pool_ci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	command_pool_ci.queueFamilyIndex = wsi->get_graphics_family_index();
	// Flags values
	// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
	// VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
	command_pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkResult result = vkCreateCommandPool(wsi->get_device(), &command_pool_ci, nullptr, &command_pool);
	assert(result == VK_SUCCESS);
}

void Renderer::create_command_buffers()
{
	command_buffer_count = wsi->get_swapchain_image_count();
	command_buffers = new VkCommandBuffer[command_buffer_count];

	VkCommandBufferAllocateInfo command_buffer_ai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	command_buffer_ai.commandPool = command_pool;
	command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_ai.commandBufferCount = command_buffer_count;

	VkResult result = vkAllocateCommandBuffers(wsi->get_device(), &command_buffer_ai, command_buffers);
	assert(result == VK_SUCCESS);
}

void Renderer::record_commands()
{
	assert(framebuffer_count == command_buffer_count);
	assert(framebuffers != nullptr);
	assert(command_buffers != nullptr);

	for (uint32_t i = 0; i < command_buffer_count; ++i)
	{
		VkCommandBufferBeginInfo command_buffer_bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		command_buffer_bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VkResult result = vkBeginCommandBuffer(command_buffers[i], &command_buffer_bi);
		assert(result == VK_SUCCESS);

		VkRenderPassBeginInfo render_pass_bi = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		render_pass_bi.renderPass = render_pass;
		render_pass_bi.framebuffer = framebuffers[i];
		render_pass_bi.renderArea.offset = { 0, 0 };
		render_pass_bi.renderArea.extent = wsi->get_swapchain_extent();
		
		VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
		render_pass_bi.clearValueCount = 1;
		render_pass_bi.pClearValues = &clear_color;

		// VK_SUBPASS_CONTENTS_INLINE means that the render pass commands (like drawing comands)
		// will be embedded in the primary command buffer and no secondary command buffers will
		// be executed
		vkCmdBeginRenderPass(command_buffers[i], &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

		viewport = { 0, 0, (float)wsi->get_width(), (float)wsi->get_height(), 0.0f, 1.0f };
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = { (uint32_t)wsi->get_width(), (uint32_t)wsi->get_height() };

		vkCmdSetViewport(command_buffers[i], 0, 1, &viewport);
		vkCmdSetScissor(command_buffers[i], 0, 1, &scissor);

		// Draw the static triangle
		// Its vertices and colors are hard-coded in the shaders
		vkCmdDraw(command_buffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(command_buffers[i]);

		result = vkEndCommandBuffer(command_buffers[i]);
		assert(result == VK_SUCCESS);
	}
}

void Renderer::create_sync_objects()
{
	VkSemaphoreCreateInfo semaphore_ci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fence_ci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	// We create the fence in a signaled state cause we wait on them before being able to 
	// draw a frame, and since frame 0 doesn't have a previous frame that was submitted,
	// it will wait indefinitely
	fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkResult result = vkCreateSemaphore(wsi->get_device(), &semaphore_ci, nullptr, &image_available_semaphores[i]);
		assert(result == VK_SUCCESS);
		result = vkCreateSemaphore(wsi->get_device(), &semaphore_ci, nullptr, &render_finished_semaphores[i]);
		assert(result == VK_SUCCESS);
		result = vkCreateFence(wsi->get_device(), &fence_ci, nullptr, &in_flight_fences[i]);
		assert(result == VK_SUCCESS);
	}
}

void Renderer::destroy_framebuffers()
{
	for (uint32_t i = 0; i < framebuffer_count; ++i)
	{
		vkDestroyFramebuffer(wsi->get_device(), framebuffers[i], nullptr);
	}

	delete[] framebuffers;
}

void Renderer::free_command_buffers()
{
	vkFreeCommandBuffers(wsi->get_device(), command_pool, command_buffer_count, command_buffers);
	// delete[] command_buffers;
}

void Renderer::destroy_sync_objects()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(wsi->get_device(), image_available_semaphores[i], nullptr);
		vkDestroySemaphore(wsi->get_device(), render_finished_semaphores[i], nullptr);
		vkDestroyFence(wsi->get_device(), in_flight_fences[i], nullptr);
	}
}

} // namespace Renderer
