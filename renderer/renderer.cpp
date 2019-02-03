#include "renderer.h"
#include "../vulkan/shaders.h"
#include <cassert>

namespace Renderer
{

Renderer::Renderer(Vulkan::WSI* wsi) : wsi(wsi)
{
}

void Renderer::init()
{
}

void Renderer::render()
{
}

void Renderer::cleanup()
{
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
	VkViewport viewport = { 0, 0, (float)wsi->get_width(), (float)wsi->get_height(), 0.0f, 1.0f };
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
	if (framebuffers != nullptr)
		destroy_framebuffers();

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

void Renderer::destroy_framebuffers()
{
	for (uint32_t i = 0; i < framebuffer_count; ++i)
	{
		vkDestroyFramebuffer(wsi->get_device(), framebuffers[i], nullptr);
	}

	delete[] framebuffers;
}

} // namespace Renderer
