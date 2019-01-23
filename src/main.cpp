#define NOMINMAX
#include <Windows.h>

#define ENABLE_VULKAN_DEBUG_CALLBACK
#include "vulkan/utils.h"
#include "vulkan/device.h" 
#include "vulkan/swapchain.h"
#include "vulkan/buffer.h"
#include "vulkan/shaders.h"
#include "vulkan/app.h"
#include "vulkan/gltf.h"

#include <assert.h>
#include <stdio.h>
#include <chrono>

gm::App* app;

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

struct VertexBuffer
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = { 0 };
} vertexBuffer;

struct IndexBuffer
{
	uint32_t count = 0;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = { 0 };
} indexBuffer;

struct Models
{
	// gm::Model scene;
	gm::gModel scene;
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
	shaderValuesScene.view = app->mainCamera.matrices.view;
	shaderValuesScene.projection = app->mainCamera.matrices.perspective;

	// Hard coding the model position to the center of the world
	// the model rotation to 0 and the scale to 1
	glm::vec3 modelPosition = glm::vec3(0.0f);
	glm::vec3 modelRotation = glm::vec3(45.0f, 0.0f, 15.0f);
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
}

void prepareUniformBuffers()
{
	for (auto &uniformBuffer : uniformBuffers)
	{
		// TODO: this creating and mapping could be moved into its own buffer struct
		GM_CHECK(gm::createBuffer(
			app->device,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(shaderValuesScene),
			&uniformBuffer.scene.buffer,
			&uniformBuffer.scene.memory), "Failed to create uniform buffer");

		GM_CHECK(vkMapMemory(
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

void setupDescriptors()
{
	// Assert that the model is loaded
	// GM_ASSERT(models.scene.linearNodes.size() > 0);
	GM_ASSERT(models.scene.nodes.size() > 0);
	// Assert that the descriptor sets has been resized
	GM_ASSERT(descriptorSets.size() > 0);

	uint32_t meshCount = models.scene.meshes.size();
	/*
	std::vector<gm::Model*> modelList = { &models.scene };
	for (auto& model : modelList)
	{
		for (auto node : model->linearNodes)
		{
			if (node->mesh)
				meshCount++;
		}
	}
	*/

	// TODO: Add a pool size for image samplers (textures)
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (4 + meshCount) * app->maxFramesInFlight }
		};

		VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		createInfo.pPoolSizes = poolSizes.data();
		createInfo.maxSets = (2 + meshCount) * app->maxFramesInFlight;
		GM_CHECK(vkCreateDescriptorPool(app->device->Device, &createInfo, nullptr, &descriptorPool), "Failed to create the descriptor pool");
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
		GM_CHECK(vkCreateDescriptorSetLayout(app->device->Device, &createInfo, nullptr, &descriptorSetLayouts.scene), "Failed to create descriptor set layout for the scene");

		for (auto i = 0; i < descriptorSets.size(); ++i)
		{
			VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.pSetLayouts = &descriptorSetLayouts.scene;
			allocInfo.descriptorSetCount = 1;
			GM_CHECK(vkAllocateDescriptorSets(app->device->Device, &allocInfo, &descriptorSets[i].scene), "Failed to allocate descriptor sets for the scene");

			// TODO: Add more write descriptor sets once we have textures
			std::vector<VkWriteDescriptorSet> writeDescriptorSets(1);

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
		GM_CHECK(vkCreateDescriptorSetLayout(app->device->Device, &createInfo, nullptr, &descriptorSetLayouts.node), "Failed to create descriptor set layout for the node");

		for (size_t n = 0; n < models.scene.nodes.size(); ++n)
		{
			if (models.scene.nodes[n].meshId > -1)
			{
				VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
				allocInfo.descriptorPool = descriptorPool;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts = &descriptorSetLayouts.node;
				uint32_t meshId = models.scene.nodes[n].meshId;
				gm::gUniformBuffer& uniformBuffer = models.scene.uniformBuffers[meshId];

				VkResult result = vkAllocateDescriptorSets(app->device->Device, &allocInfo, &uniformBuffer.descriptorSet);
				GM_ASSERT(result == VK_SUCCESS);

				// VKR_CHECK(vkAllocateDescriptorSets(app->device->Device, &allocInfo, &node->mesh->uniformBuffer.descriptorSet), "Failed to allocate descriptor set per node");

				VkWriteDescriptorSet writeDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSet.descriptorCount = 1;
				writeDescriptorSet.dstSet = uniformBuffer.descriptorSet;
				writeDescriptorSet.dstBinding = 0;
				writeDescriptorSet.pBufferInfo = &uniformBuffer.descriptor;

				vkUpdateDescriptorSets(app->device->Device, 1, &writeDescriptorSet, 0, nullptr);
			}
		}

		/*
		// Per-node descriptor set
		for (auto& node : models.scene.nodes)
		{
			setupNodeDescriptorSet(node);
		}
		*/
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
	rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
	GM_CHECK(vkCreatePipelineLayout(app->device->Device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout), "Failed to create pipeline layout");

	// Vertex bindings and attributes
	VkVertexInputBindingDescription vertexInputBinding = { 0, sizeof(gm::gVertex), VK_VERTEX_INPUT_RATE_VERTEX };
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // Position
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3 }, // Normal
		// { 2, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6 }, // UV
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBinding;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();

	// Shaders
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);

	// TODO: Add a function to load a shader and return a VkPipelineShaderStageCreateInfo
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
	gm::setupShader(app->device->Device, app->device->PhysicalDevice, "../data/shaders/vert.spv", &vertShaderModule, "../data/shaders/frag.spv", &fragShaderModule);

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
	GM_CHECK(vkCreatePipelineCache(app->device->Device, &pipelineCacheCreateInfo, nullptr, &pipelineCache), "Failed to create the pipeline cache");

	GM_CHECK(vkCreateGraphicsPipelines(app->device->Device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.scene), "Failed to create graphics pipeline for the scene");

	for (auto shaderStage : shaderStages)
	{
		vkDestroyShaderModule(app->device->Device, shaderStage.module, nullptr);
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

		GM_CHECK(vkBeginCommandBuffer(currentCB, &cmdBufferBeginInfo), "Failed to begin command buffer");
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
		// gm::Model& model = models.scene;
		gm::gModel& model = models.scene;

		// vkCmdBindVertexBuffers(currentCB, 0, 1, &model.vertices.buffer, offsets);
		// vkCmdBindIndexBuffer(currentCB, model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindVertexBuffers(currentCB, 0, 1, &vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(currentCB, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		// Render all the nodes.
		// TODO: When we load materials, we need to draw the primitives in this order:
		// 1. Opaque
		// 2. Mask
		// 3. Transparent (after binding a transparent pipeline)
		for (size_t p = 0; p < models.scene.primitives.size(); ++p)
		{
			gm::gPrimitive& primitive = models.scene.primitives[p];
			gm::gUniformBuffer& uniformBuffer = models.scene.uniformBuffers[primitive.meshId];
			// TODO: When we have materials we need to load the descriptor sets for the material as well
			const std::vector<VkDescriptorSet> descriptorSets_ = { descriptorSets[i].scene, uniformBuffer.descriptorSet };
			vkCmdBindDescriptorSets(app->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorSets_.size()), descriptorSets_.data(), 0, nullptr);

			// TODO: pass the materials as push constants

			vkCmdDrawIndexed(app->commandBuffers[i], primitive.indexCount, 1, primitive.firstIndex, 0, 0);
		}

		vkCmdEndRenderPass(currentCB);
		GM_CHECK(vkEndCommandBuffer(currentCB), "Failed to end the command buffer");
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

	GM_CHECK(vkWaitForFences(app->device->Device, 1, &app->syncObjects.InFlightFences[currentFrameIndex], VK_TRUE, UINT64_MAX), "Failed to wait for fence");
	GM_CHECK(vkResetFences(app->device->Device, 1, &app->syncObjects.InFlightFences[currentFrameIndex]), "Failed to reset fences");

	uint32_t nextImageIndex;
	VkResult acquire = vkAcquireNextImageKHR(app->device->Device, app->swapchain->Swapchain, UINT64_MAX, app->syncObjects.ImageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &nextImageIndex);
	if (acquire == VK_ERROR_OUT_OF_DATE_KHR || acquire == VK_SUBOPTIMAL_KHR)
	{
		// TODO: we need to resize the window
	}
	else
	{
		GM_ASSERT(acquire == VK_SUCCESS);
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

	GM_CHECK(vkQueueSubmit(queue, 1, &submitInfo, app->syncObjects.InFlightFences[currentFrameIndex]), "Failed to submit to queue");

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
			GM_ASSERT(present == VK_SUCCESS);
		}
	}

	currentFrameIndex += 1;
	currentFrameIndex %= app->maxFramesInFlight;

	auto frameCPUEnd = std::chrono::high_resolution_clock::now();
	auto frameCPU = (frameCPUEnd - frameCPUStart).count() * 1e-6;

	app->mainCamera.update((float)(frameCPU / 1000.0f));
	if (app->mainCamera.moving())
	{
		updateUniformBuffers();
	}

	frameCPUAvg = frameCPUAvg * 0.95 + frameCPU * 0.05;

	char title[256];
	sprintf(title, "VRK - CAMERA: %.2f, %.2f, %.2f - cpu: %.2f ms - gpu: %.2f ms", app->mainCamera.position.x, app->mainCamera.position.y, app->mainCamera.position.z, frameCPUAvg, 0.0f);

#if _WIN32
	SetWindowTextA(app->window, title);
#endif
}


int main()
{
	uint32_t currentFrameIndex = 0;

	app = new gm::App();
	app->initVulkan();
	app->setupWindow(width, height, GetModuleHandle(nullptr), MainWndProc);
	app->prepare();

	models.scene = gm::loadModelFromFile("../data/models/Box/glTF/Box.gltf", app->device);
	// Upload this model indices and vertices to the index and vertex buffers on the GPU
	{
		// For now we only have one model so we're gonna make the vertexBuffer and indexBuffer
		// match our model sizes
		size_t vertexBufferSize = models.scene.vertices.size() * sizeof(gm::gVertex);
		size_t indexBufferSize = models.scene.indices.size() * sizeof(uint32_t);
		// TODO: Why do we need this?
		indexBuffer.count = static_cast<uint32_t>(models.scene.indices.size());

		GM_ASSERT((vertexBufferSize > 0) && (indexBufferSize > 0));

		struct StagingBuffer
		{
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory = { 0 };
		} vertexStaging, indexStaging;

		// Create staging buffers
		// Vertex data
		GM_CHECK(gm::createBuffer(app->device,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBufferSize,
			&vertexStaging.buffer,
			&vertexStaging.memory,
			models.scene.vertices.data()), "Failed to upload vertices to staging buffer");

		// Index data
		GM_CHECK(gm::createBuffer(app->device,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indexBufferSize,
			&indexStaging.buffer,
			&indexStaging.memory,
			models.scene.indices.data()), "Failed to upload indices to staging buffer");

		// Create device local buffers
		// Vertex buffer
		GM_CHECK(gm::createBuffer(app->device,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBufferSize,
			&vertexBuffer.buffer,
			&vertexBuffer.memory), "Failed to create vertex buffer");

		// Index buffer
		GM_CHECK(gm::createBuffer(app->device,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			indexBufferSize,
			&indexBuffer.buffer,
			&indexBuffer.memory), "Failed to create index buffer");

		// Copy the staging buffers
		VkCommandBuffer copyCmd = app->device->createTransferCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkQueue queue = app->device->getQueue(app->device->TransferFamilyIndex);
		
		// NOTE: Once we have multiple objects we'll have to adjust the region we're copying
		VkBufferCopy copyRegion = {};
		copyRegion.size = vertexBufferSize;

		vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertexBuffer.buffer, 1, &copyRegion);

		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indexBuffer.buffer, 1, &copyRegion);

		app->device->flushCommandBuffer(copyCmd, queue, true, true);

		// Destroy the staging vertex and index buffers
		vkDestroyBuffer(app->device->Device, vertexStaging.buffer, nullptr);
		vkFreeMemory(app->device->Device, vertexStaging.memory, nullptr);
		vkDestroyBuffer(app->device->Device, indexStaging.buffer, nullptr);
		vkFreeMemory(app->device->Device, indexStaging.memory, nullptr);
	}
	// gm::destroyModel(model, app->device);

	// Load the scene models
	// models.scene.loadFromFile("../data/models/Box/glTF/Box.gltf", app->device);
	// models.scene.loadFromFile("../data/models/DamagedHelmet/glTF/DamagedHelmet.gltf", app->device);

	//float scale = 1.0f / models.scene.dimensions.radius;
	//app->mainCamera.setPosition(glm::vec3(-models.scene.dimensions.center.x * scale, -models.scene.dimensions.center.y * scale, -2 * app->mainCamera.position.z));
	app->mainCamera.setPosition(glm::vec3(0.0f, 0.0f, -3.0f));

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

	// models.scene.destroy();
	gm::destroyModel(models.scene, app->device);

	// Destroy the vertex and index buffers
	vkDestroyBuffer(app->device->Device, vertexBuffer.buffer, nullptr);
	vkFreeMemory(app->device->Device, vertexBuffer.memory, nullptr);
	vkDestroyBuffer(app->device->Device, indexBuffer.buffer, nullptr);
	vkFreeMemory(app->device->Device, indexBuffer.memory, nullptr);

	for (auto &uniformBuffer : uniformBuffers)
	{
		vkDestroyBuffer(app->device->Device, uniformBuffer.scene.buffer, nullptr);
		vkFreeMemory(app->device->Device, uniformBuffer.scene.memory, nullptr);
	}

	app->device->destroyQueryPool(queryPool);

	vkDestroyPipelineCache(app->device->Device, pipelineCache, nullptr);
	vkDestroyPipelineLayout(app->device->Device, pipelineLayout, nullptr);
	vkDestroyPipeline(app->device->Device, pipelines.scene, nullptr);
	vkDestroyDescriptorSetLayout(app->device->Device, descriptorSetLayouts.scene, nullptr);
	vkDestroyDescriptorSetLayout(app->device->Device, descriptorSetLayouts.node, nullptr);
	vkDestroyDescriptorPool(app->device->Device, descriptorPool, nullptr);

	delete app;

	return (int)msg.wParam;
}
