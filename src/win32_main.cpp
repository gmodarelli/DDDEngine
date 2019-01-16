#define NOMINMAX
#include <Windows.h>

#define ENABLE_VULKAN_DEBUG_CALLBACK
#include "VulkanTools.h"
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

int main()
{
	uint32_t width = 1600;
	uint32_t height = 1200;

	app = new vkr::App();
	app->initVulkan();
	app->setupWindow(width, height, GetModuleHandle(nullptr), MainWndProc);
	app->prepare();

	// All the following settings are app-specific

	vkr::glTF::Model model;
	model.loadFromFile("../data/models/DamagedHelmet/glTF/DamagedHelmet.gltf", app->device);
	model.destroy();

	vkr::Scene scene;
	Buffer vertexBuffer;
	Buffer indexBuffer;
	scene.load("../data/models/sibenik/sibenik.dae", *(app->device), &vertexBuffer, &indexBuffer);

	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
	SetupShader(app->device->Device, app->device->PhysicalDevice, &vertShaderModule, &fragShaderModule);

	// TODO: Move this into its own file
	Buffer uniformBuffer;

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::vec4 lightPos;
	};

	UniformData ubo = {};
	ubo.model = glm::mat4(1.0f);
	ubo.view = app->mainCamera.matrices.view;
	ubo.projection = app->mainCamera.matrices.perspective;
	ubo.lightPos = glm::vec4(-app->mainCamera.position, 1.0f);
	bool viewUpdated = true;

	VkDeviceSize bufferSize = sizeof(ubo);
	vkr::createBuffer(app->device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufferSize, &uniformBuffer.Buffer, &uniformBuffer.DeviceMemory);

	// Set buffer content
	VK_CHECK(vkMapMemory(app->device->Device, uniformBuffer.DeviceMemory, 0, VK_WHOLE_SIZE, 0, &uniformBuffer.data));
	// memcpy(uniformBuffer.data, &ubo, sizeof(ubo));
	// vkUnmapMemory(vulkanDevice.Device, uniformBuffer.DeviceMemory);

	Descriptor descriptor;
	SetupDescriptors(app->device->Device, uniformBuffer.Buffer, 1, &descriptor);

	Pipeline pipeline;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { descriptor.DescriptorSetLayout };
	SetupPipeline(app->device->Device, app->swapchain->ImageExtent.width, app->swapchain->ImageExtent.height, descriptorSetLayouts, vertShaderModule, fragShaderModule, app->renderPass, &pipeline);

	VkQueryPool queryPool = app->device->createQueryPool(128);

	VkQueue graphycsQueue = app->device->getQueue(app->device->GraphicsFamilyIndex);
	VkQueue presentQueue = app->device->getQueue(app->device->PresentFamilyIndex);

	size_t maxFramesInFlight = app->framebuffers.size();
	SyncObjects syncObjects;

	CreateSyncObjects(app->device->Device, maxFramesInFlight, &syncObjects);

	uint32_t currentFrameIndex = 0;

	MSG msg = { 0 };

	double frameCpuAvg = 0;
	double frameGpuAvg = 0;

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
			auto frameCPUStart = std::chrono::high_resolution_clock::now();

			if (viewUpdated)
			{
				memcpy(uniformBuffer.data, &ubo, sizeof(ubo));
				viewUpdated = false;
			}

			RecordCommands(app->device->Device, syncObjects, app->commandBuffers[currentFrameIndex], vertexBuffer, indexBuffer, scene, app->framebuffers, app->renderPass, descriptor, pipeline, queryPool, app->swapchain->ImageExtent.width, app->swapchain->ImageExtent.height, currentFrameIndex);
			double frameGPU = RenderLoop(app->device->Device, app->device->PhysicalDeviceProperties, app->swapchain->Swapchain, app->commandBuffers, queryPool, graphycsQueue, presentQueue, syncObjects, currentFrameIndex);
			currentFrameIndex = (currentFrameIndex + 1) % maxFramesInFlight;

			auto frameCPUEnd = std::chrono::high_resolution_clock::now();
			auto frameCPU = (frameCPUEnd - frameCPUStart).count() * 1e-6;

			frameCpuAvg = frameCpuAvg * 0.95 + frameCPU * 0.05;
			frameGpuAvg = frameGpuAvg * 0.95 + frameGPU * 0.05;

			char title[256];
			sprintf(title, "VRK - cpu: %.2f ms - gpu: %.2f ms", frameCpuAvg, frameGpuAvg);

			app->mainCamera.update(frameCPU / 1000.0f);
			if (app->mainCamera.moving())
			{
				ubo.view = app->mainCamera.matrices.view;
				viewUpdated = true;
			}

#if _WIN32
			SetWindowTextA(app->window, title);
#endif
		}
	}

	vkDeviceWaitIdle(app->device->Device);

	// Cleanup
	app->device->destroyQueryPool(queryPool);
	DestroySyncObjects(app->device->Device, &syncObjects);
	DestroyPipeline(app->device->Device, &pipeline);
	DestroyDescriptor(app->device->Device, &descriptor);
	DestroyShaderModule(app->device->Device, &vertShaderModule);
	DestroyShaderModule(app->device->Device, &fragShaderModule);

	vkr::destroyBuffer(app->device->Device, &vertexBuffer);
	vkr::destroyBuffer(app->device->Device, &indexBuffer);
	vkr::destroyBuffer(app->device->Device, &uniformBuffer);

	delete app;

	return (int)msg.wParam;
}
