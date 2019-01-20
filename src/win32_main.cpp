#define NOMINMAX
#include <Windows.h>

#define ENABLE_VULKAN_DEBUG_CALLBACK
#include "vkr/utils.h"
#include "vkr/device.h" 
#include "vkr/swapchain.h"
#include "vkr/buffer.h"
#include "vkr/model.h"
#include "vkr/shaders.h"
#include "vkr/app.h"

#include <assert.h>
#include <stdio.h>
#include <chrono>

vkr::App* app;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (app != nullptr)
	{
		app->handleMessages(hwnd, msg, wParam, lParam);
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

// NOTE: The following structs functions are demo specific
uint32_t width = 1600;
uint32_t height = 1200;
float scale = 1.0f;

struct Models
{
	vkr::glTF::Model scene;
} models;

// We need to create the buffers that will hold the data for our two uniform buffers
// Since we have multiple frames in flight, we need to create 2 * app->maxFramesInFlight
struct UniformBuffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDescriptorBufferInfo descriptor;
	void* data;
};

struct UniformBufferSet
{
	UniformBuffer scene;
	// TODO: Add one more for the skybox
};

// We start with the Uniform Buffer Object for the MVP Matrix and camera position
struct UBOMatrices
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	glm::vec3 cameraPosition;
} shaderValuesScene; // TODO: Add one more for the skybox

VkPipelineLayout pipelineLayout;
VkPipelineCache pipelineCache;

struct Pipelines
{
	VkPipeline scene;
	// TODO: Add more pipelines, like skybox for example, or alpha blend
} pipelines;


struct DescriptorSetLayouts {
	VkDescriptorSetLayout scene;
	VkDescriptorSetLayout node;
} descriptorSetLayouts;

struct DescriptorSets {
	VkDescriptorSet scene;
	// NOTE: The VkDescriptorSet for each node is stored in the node's struct
};

std::vector<DescriptorSets> descriptorSets;
std::vector<UniformBufferSet> uniformBuffers;

VkDescriptorPool descriptorPool;

void updateUniformBuffers()
{
	shaderValuesScene.view = glm::lookAt(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	shaderValuesScene.projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
	shaderValuesScene.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.2f));
	shaderValuesScene.cameraPosition = glm::vec3(0.0f, 0.0f, -50.0f);

	/*
	shaderValuesScene.view = app->mainCamera.matrices.view;
	shaderValuesScene.projection = app->mainCamera.matrices.perspective;

	// Hard coding the model position to the center of the world
	// the model rotation to 0 and the scale to 1
	glm::vec3 modelPosition = glm::vec3(0.0f);
	glm::vec3 modelRotation = glm::vec3(0.0f);
	float scale = 1.0f;

	shaderValuesScene.model = glm::translate(glm::mat4(1.0f), modelPosition);
	shaderValuesScene.model = glm::rotate(shaderValuesScene.model, glm::radians(modelRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	shaderValuesScene.model = glm::rotate(shaderValuesScene.model, glm::radians(modelRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	shaderValuesScene.model = glm::rotate(shaderValuesScene.model, glm::radians(modelRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	shaderValuesScene.model = glm::scale(shaderValuesScene.model, glm::vec3(scale));

	shaderValuesScene.cameraPosition = glm::vec3(
		-app->mainCamera.position.z * sin(glm::radians(app->mainCamera.rotation.y)) * cos(glm::radians(app->mainCamera.rotation.x)),
		-app->mainCamera.position.z * sin(glm::radians(app->mainCamera.rotation.x)),
		 app->mainCamera.position.z * cos(glm::radians(app->mainCamera.rotation.y)) * cos(glm::radians(app->mainCamera.rotation.x))
	);
	*/
}

void prepareUniformBuffers()
{
	for (auto &uniformBuffer : uniformBuffers)
	{
		// TODO: this creating and mapping could be moved into its own buffer struct
		VKR_CHECK(vkr::createBuffer(
			app->device,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(shaderValuesScene),
			&uniformBuffer.scene.buffer,
			&uniformBuffer.scene.memory), "Failed to create uniform buffer");

		VKR_CHECK(vkMapMemory(
			app->device->Device,
			uniformBuffer.scene.memory,
			0,
			sizeof(shaderValuesScene),
			0,
			&uniformBuffer.scene.data), "Failed to map memory");

		uniformBuffer.scene.descriptor = { uniformBuffer.scene.buffer, 0, sizeof(shaderValuesScene) };
	}
	updateUniformBuffers();
}


void setupNodeDescriptorSet(vkr::glTF::Node* node)
{
	if (node->mesh)
	{
		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayouts.node;
		VkResult result = vkAllocateDescriptorSets(app->device->Device, &allocInfo, &node->mesh->uniformBuffer.descriptorSet);
		VKR_ASSERT(result == VK_SUCCESS);

		// VKR_CHECK(vkAllocateDescriptorSets(app->device->Device, &allocInfo, &node->mesh->uniformBuffer.descriptorSet), "Failed to allocate descriptor set per node");

		VkWriteDescriptorSet writeDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.dstSet = node->mesh->uniformBuffer.descriptorSet;
		writeDescriptorSet.dstBinding = 0;
		writeDescriptorSet.pBufferInfo = &node->mesh->uniformBuffer.descriptor;

		vkUpdateDescriptorSets(app->device->Device, 1, &writeDescriptorSet, 0, nullptr);
	}

	for (auto& child : node->children)
		setupNodeDescriptorSet(child);
}

void setupDescriptors()
{
	// Assert that the model is loaded
	VKR_ASSERT(models.scene.linearNodes.size() > 0);
	// Assert that the descriptor sets has been resized
	VKR_ASSERT(descriptorSets.size() > 0);

	uint32_t meshCount = 0;
	std::vector<vkr::glTF::Model*> modelList = { &models.scene };
	for (auto& model : modelList)
	{
		for (auto node : model->linearNodes)
		{
			if (node->mesh)
				meshCount++;
		}
	}

	// TODO: Add a pool size for image samplers (textures)
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (4 + meshCount) * app->maxFramesInFlight }
		};

		VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		createInfo.pPoolSizes = poolSizes.data();
		createInfo.maxSets = (2 + meshCount) * app->maxFramesInFlight;
		VKR_CHECK(vkCreateDescriptorPool(app->device->Device, &createInfo, nullptr, &descriptorPool), "Failed to create the descriptor pool");
	}

	// Descriptor sets
	// Scene
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
			// TODO: Add other set layout bindings for textures
		};
		VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		createInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		createInfo.pBindings = setLayoutBindings.data();
		VKR_CHECK(vkCreateDescriptorSetLayout(app->device->Device, &createInfo, nullptr, &descriptorSetLayouts.scene), "Failed to create descriptor set layout for the scene");

		for (auto i = 0; i < descriptorSets.size(); ++i)
		{
			VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.pSetLayouts = &descriptorSetLayouts.scene;
			allocInfo.descriptorSetCount = 1;
			VKR_CHECK(vkAllocateDescriptorSets(app->device->Device, &allocInfo, &descriptorSets[i].scene), "Failed to allocate descriptor sets for the scene");

			// TODO: Add more write descriptor sets once we have textures
			std::array<VkWriteDescriptorSet, 1> writeDescriptorSets{};

			writeDescriptorSets[0].sType = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[0].descriptorCount = 1;
			writeDescriptorSets[0].dstSet = descriptorSets[i].scene;
			writeDescriptorSets[0].dstBinding = 0;
			writeDescriptorSets[0].pBufferInfo = &uniformBuffers[i].scene.descriptor;

			vkUpdateDescriptorSets(app->device->Device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	// Node matrices
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr },
		};

		VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		createInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		createInfo.pBindings = setLayoutBindings.data();
		VKR_CHECK(vkCreateDescriptorSetLayout(app->device->Device, &createInfo, nullptr, &descriptorSetLayouts.node), "Failed to create descriptor set layout for the node");

		// Per-node descriptor set
		for (auto& node : models.scene.nodes)
		{
			setupNodeDescriptorSet(node);
		}
	}

	// TODO: materials (samplers)
}

void preparePipelines()
{
	// NOTE: As of now we're only bilding one pipelines for opaque objects.
	// NOTE: For a skybox pipeline we'd disable depth testing
	// depthStencilCreateInfo.depthTestEnable = VK_FALSE;
	// depthStencilCreateInfo.depthWriteEnable = VK_FALSE;
	// NOTE: For an alpha blend pipeline we need a different blend attachment state with
	// blendAttachmentState.blendEnable = VK_TRUE;
	// blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	// blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	// blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	// blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	// blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationCreateInfo.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState blendAttachmentState = {};
	blendAttachmentState.blendEnable = VK_FALSE;
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &blendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilCreateInfo.front = depthStencilCreateInfo.back;
	depthStencilCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.scissorCount = 1;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	// TODO: Add the sample count once we start reading texel data
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

	// Pipeline layout
	const std::vector<VkDescriptorSetLayout> setLayouts = {
		descriptorSetLayouts.scene, descriptorSetLayouts.node
	};
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = setLayouts.data();
	// TODO: When we have materials we need to add push constant ranges
	VKR_CHECK(vkCreatePipelineLayout(app->device->Device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout), "Failed to create pipeline layout");

	// Vertex bindings and attributes
	VkVertexInputBindingDescription vertexInputBinding = { 0, sizeof(vkr::glTF::Model::Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // Position
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3 }, // Normal
		{ 2, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6 }, // UV
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBinding;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();

	// Shaders
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	// TODO: Add a function to load a shader and return a VkPipelineShaderStageCreateInfo
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
	vkr::setupShader(app->device->Device, app->device->PhysicalDevice, "../data/shaders/vert.spv", &vertShaderModule, "../data/shaders/frag.spv", &fragShaderModule);

	shaderStages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertShaderModule;
	shaderStages[0].pName = "main"; // Shader entry point
	shaderStages[0].pSpecializationInfo = nullptr;

	shaderStages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragShaderModule;
	shaderStages[1].pName = "main"; // Shader entry point
	shaderStages[1].pSpecializationInfo = nullptr;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = app->renderPass;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();

	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VKR_CHECK(vkCreatePipelineCache(app->device->Device, &pipelineCacheCreateInfo, nullptr, &pipelineCache), "Failed to create the pipeline cache");

	VKR_CHECK(vkCreateGraphicsPipelines(app->device->Device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.scene), "Failed to create graphics pipeline for the scene");

	for (auto shaderStage : shaderStages)
	{
		vkDestroyShaderModule(app->device->Device, shaderStage.module, nullptr);
	}
}

void renderNode(vkr::glTF::Node* node, uint32_t cbIndex)
{
	if (node->mesh)
	{
		// Render all the primitives
		for (vkr::glTF::Primitive* primitive : node->mesh->primitives)
		{
			// TODO: When we have materials we need to load the descriptor sets for the material as well
			const std::vector<VkDescriptorSet> descriptorSets_ = { descriptorSets[cbIndex].scene, node->mesh->uniformBuffer.descriptorSet };
			vkCmdBindDescriptorSets(app->commandBuffers[cbIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorSets_.size()), descriptorSets_.data(), 0, nullptr);

			// TODO: pass the materials as push constants

			vkCmdDrawIndexed(app->commandBuffers[cbIndex], primitive->indexCount, 1, primitive->firstIndex, 0, 0);
		}
	}

	for (auto child : node->children)
	{
		renderNode(child, cbIndex);
	}
}

void recordCommands()
{
	VkCommandBufferBeginInfo cmdBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	// Clear the color buffer and depth buffer
	VkClearValue clearValues[2];
	clearValues[0].color = { {0.0f, 191.0f / 255.0f, 1.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.renderPass = app->renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	for (size_t i = 0; i < app->commandBuffers.size(); ++i)
	{
		renderPassBeginInfo.framebuffer = app->framebuffers[i];
		VkCommandBuffer currentCB = app->commandBuffers[i];

		VKR_CHECK(vkBeginCommandBuffer(currentCB, &cmdBufferBeginInfo), "Failed to begin command buffer");
		// TODO: What it VK_SUBPASS_CONTENTS_INLINE?
		vkCmdBeginRenderPass(currentCB, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		VkViewport viewport = {};
		viewport.width = (float)width;
		viewport.height = (float)height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(currentCB, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.extent = { width, height };
		vkCmdSetScissor(currentCB, 0, 1, &scissor);

		VkDeviceSize offsets[1] = { 0 };
		// TODO: When we introduce a skybox, this is where we would bind its pipeline

		vkCmdBindPipeline(currentCB, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.scene);
		vkr::glTF::Model& model = models.scene;

		vkCmdBindVertexBuffers(currentCB, 0, 1, &model.vertices.buffer, offsets);
		vkCmdBindIndexBuffer(currentCB, model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

		// Render all the nodes.
		// TODO: When we load materials, we need to draw the primitives in this order:
		// 1. Opaque
		// 2. Mask
		// 3. Transparent (after binding a transparent pipeline)
		for (auto node : model.nodes)
		{
			renderNode(node, i);
		}

		vkCmdEndRenderPass(currentCB);
		VKR_CHECK(vkEndCommandBuffer(currentCB), "Failed to end the command buffer");
	}
}

bool prepared = false;
uint32_t currentFrameIndex = 0;

void render(VkQueue queue, VkQueue presentQueue)
{
	auto frameCPUStart = std::chrono::high_resolution_clock::now();
	static double frameCPUAvg = 0;

	if (!prepared)
	{
		return;
	}

	VKR_CHECK(vkWaitForFences(app->device->Device, 1, &app->syncObjects.InFlightFences[currentFrameIndex], VK_TRUE, UINT64_MAX), "Failed to wait for fence");
	VKR_CHECK(vkResetFences(app->device->Device, 1, &app->syncObjects.InFlightFences[currentFrameIndex]), "Failed to reset fences");

	uint32_t nextImageIndex;
	VkResult acquire = vkAcquireNextImageKHR(app->device->Device, app->swapchain->Swapchain, UINT64_MAX, app->syncObjects.ImageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &nextImageIndex);
	if (acquire == VK_ERROR_OUT_OF_DATE_KHR || acquire == VK_SUBOPTIMAL_KHR)
	{
		// TODO: we need to resize the window
	}
	else
	{
		VKR_ASSERT(acquire == VK_SUCCESS);
	}

	updateUniformBuffers();
	memcpy(uniformBuffers[currentFrameIndex].scene.data, &shaderValuesScene, sizeof(shaderValuesScene));

	const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.pWaitSemaphores = &app->syncObjects.ImageAvailableSemaphores[currentFrameIndex];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &app->syncObjects.RenderFinishedSemaphores[currentFrameIndex]; 
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pCommandBuffers = &app->commandBuffers[currentFrameIndex];
	submitInfo.commandBufferCount = 1;

	VKR_CHECK(vkQueueSubmit(queue, 1, &submitInfo, app->syncObjects.InFlightFences[currentFrameIndex]), "Failed to submit to queue");

	// Present the backbuffer
	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &app->syncObjects.RenderFinishedSemaphores[currentFrameIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &app->swapchain->Swapchain;
	presentInfo.pImageIndices = &nextImageIndex;

	VkResult present = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (!((present == VK_SUCCESS) || (present == VK_SUBOPTIMAL_KHR))) {
		if (present == VK_ERROR_OUT_OF_DATE_KHR) {
			// TODO: Resize window
			return;
		}
		else {
			VKR_ASSERT(present == VK_SUCCESS);
		}
	}

	currentFrameIndex += 1;
	currentFrameIndex %= app->maxFramesInFlight;

	auto frameCPUEnd = std::chrono::high_resolution_clock::now();
	auto frameCPU = (frameCPUEnd - frameCPUStart).count() * 1e-6;

	// app->mainCamera.update(frameCPU / 1000.0f);
	app->mainCamera.update(frameCPU / 100.0f);
	if (app->mainCamera.moving())
	{
		updateUniformBuffers();
	}

	frameCPUAvg = frameCPUAvg * 0.95 + frameCPU * 0.05;

	char title[256];
	sprintf(title, "VRK - CAMERA: %.2f, %.2f, %.2f - cpu: %.2f ms - gpu: %.2f ms", app->mainCamera.position.x, app->mainCamera.position.y, app->mainCamera.position.z, frameCPUAvg, 0);

#if _WIN32
	SetWindowTextA(app->window, title);
#endif
}


int main()
{
	uint32_t currentFrameIndex = 0;

	app = new vkr::App();
	app->initVulkan();
	app->setupWindow(width, height, GetModuleHandle(nullptr), MainWndProc);
	app->prepare();

	// Load the scene models
	models.scene.loadFromFile("../data/models/Box/glTF/Box.gltf", app->device);
	//float scale = 1.0f / models.scene.dimensions.radius;
	//app->mainCamera.setPosition(glm::vec3(-models.scene.dimensions.center.x * scale, -models.scene.dimensions.center.y * scale, app->mainCamera.position.z));

	uniformBuffers.resize(app->maxFramesInFlight);
	descriptorSets.resize(app->maxFramesInFlight);

	prepareUniformBuffers();
	setupDescriptors();
	preparePipelines();
	recordCommands();

	VkQueryPool queryPool = app->device->createQueryPool(128);

	VkQueue graphycsQueue = app->device->getQueue(app->device->GraphicsFamilyIndex);
	VkQueue presentQueue = app->device->getQueue(app->device->PresentFamilyIndex);

	MSG msg = { 0 };

	prepared = true;

	while (msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff.
		else
		{
			render(graphycsQueue, presentQueue);
		}
	}

	vkDeviceWaitIdle(app->device->Device);

	models.scene.destroy();
	/*
	// TODO: Move these
	vkDestroyDescriptorPool(app->device->Device, descriptorPool, nullptr);

	for (size_t i = 0; i < mvpUniformBuffers.size(); ++i)
	{
		vkDestroyBuffer(app->device->Device, mvpUniformBuffers[i].buffer, nullptr);
		vkFreeMemory(app->device->Device, mvpUniformBuffers[i].memory, nullptr);
	}

	// NOTE: Destroying it now to avoid validation errors
	vkDestroyDescriptorSetLayout(app->device->Device, descriptorSetLayouts[0], nullptr);
	vkDestroyDescriptorSetLayout(app->device->Device, descriptorSetLayouts[1], nullptr);

	// Cleanup
	app->device->destroyQueryPool(queryPool);
	DestroyPipeline(app->device->Device, &pipeline);
	// DestroyDescriptor(app->device->Device, &descriptor);
	DestroyShaderModule(app->device->Device, &vertShaderModule);
	DestroyShaderModule(app->device->Device, &fragShaderModule);

	vkr::destroyBuffer(app->device->Device, &vertexBuffer);
	vkr::destroyBuffer(app->device->Device, &indexBuffer);
	// vkr::destroyBuffer(app->device->Device, &uniformBuffer);
	*/ 

	delete app;

	return (int)msg.wParam;
}
