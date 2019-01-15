#define NOMINMAX
#include <Windows.h>

#include "vkr/device.h" 
#include "vkr/swapchain.h"

#define ENABLE_VULKAN_DEBUG_CALLBACK
#include "VulkanTools.h"
#include "MeshLoader.h"

#include <assert.h>
#include <stdio.h>
#include <chrono>

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

void OnMouseDown(WPARAM btnState, int x, int y)
{
	
}

void OnMouseUp(WPARAM btnState, int x, int y)
{
	
}

void OnMouseMove(WPARAM btnState, int x, int y)
{
	
}

struct Input
{
	bool up = false;
	bool right = false;
	bool down = false;
	bool left = false;
} keyboardInput;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		// WM_ACTIVATE is sent when the window is activated or deactivated.
		// TODO: Once we add a timer this will be the place to start/stop
		// the time
		if (LOWORD(wParam) == WA_INACTIVE)
		{

			printf("The window is inactive, should pause the app\n");
			OutputDebugString(L"The window is inactive, should pause the app");
		}
		else
		{
			printf("The window is active, should start the app\n");
			OutputDebugString(L"The window is active, should start the app");
		}
		return 0;

	case WM_DESTROY:
		// WM_DESTROY is sent when the window is being destroyed.
		PostQuitMessage(0);
		return 0;

	case WM_MENUCHAR:
		// THE WM_MENUCHAR is sent when a menu is active and the user
		// presses a key that does not correspond to any mnemonic
		// or accelerator key.
		// Don't beep when we alt-enter
		return MAKELRESULT(0, MNC_CLOSE);

	case WM_GETMINMAXINFO:
		// Catch this message so to prevent the window from becoming too small.
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
		bool wasDown = ((lParam & (1 << 30)) != 0);
		bool isDown = ((lParam & (1UL << 31)) == 0);
		if (wasDown != isDown)
		{
			if (wParam == 0x57 || wParam == VK_UP)
			{
				keyboardInput.up = isDown;
			}

			if (wParam == 0x53 || wParam == VK_DOWN)
			{
				keyboardInput.down = isDown;
			}

			if (wParam == 0x44 || wParam == VK_RIGHT)
			{
				keyboardInput.right = isDown;
			}

			if (wParam == 0x41 || wParam == VK_LEFT)
			{
				keyboardInput.left = isDown;
			}

			if (isDown)
			{
				if (wParam == VK_ESCAPE)
				{
					PostQuitMessage(0);
				}
			}
		}

		return 0;

	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool SetupWindow(uint32_t width, uint32_t height, HINSTANCE instance, HWND* window)
{
	WNDCLASS windowClass;
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = MainWndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = instance;
	windowClass.hIcon = LoadIcon(0, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(0, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = L"MainWnd";

	if (!RegisterClass(&windowClass))
	{
		MessageBox(0, L"RegisterClass failed!", 0, 0);
		return false;
	}

	RECT R = { 0, 0, (LONG)width, (LONG)height };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int adjustedWidth = R.right - R.left;
	int adjustedHeight = R.bottom - R.top;

	*window = CreateWindow(L"MainWnd", L"Renderer", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, adjustedWidth, adjustedHeight, 0, 0, instance, 0);
	if (!window)
	{
		MessageBox(0, L"CreateWindow failed", 0, 0);
		return false;
	}

	ShowWindow(*window, SW_SHOW);
	UpdateWindow(*window);

	return true;
}

int main()
{
	WindowParameters windowParams = { GetModuleHandle(nullptr) };
	uint32_t windowWidth = 1600;
	uint32_t windowHeight = 1200;

	SetupWindow(windowWidth, windowHeight, windowParams.Hinstance, &windowParams.HWnd);

	VkQueueFlags requiredQueues = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT;
	vkr::VulkanDevice vulkanDevice(windowParams, requiredQueues);
	VkSurfaceFormatKHR desiredFormat { VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
	VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	vkr::VulkanSwapchain vulkanSwapchain(vulkanDevice, desiredFormat, desiredPresentMode, windowWidth, windowHeight);

	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;
	BufferImage depthBufferImage;
	SetupRenderPass(vulkanDevice.Device, vulkanDevice.PhysicalDevice, vulkanSwapchain.ImageExtent.width, vulkanSwapchain.ImageExtent.height, vulkanSwapchain.ImageViews, vulkanSwapchain.ImageCount, &renderPass, &framebuffers, &depthBufferImage);

	Command command;
	SetupCommandBuffer(vulkanDevice.Device, vulkanDevice.PhysicalDevice, vulkanDevice.GraphicsFamilyIndex, static_cast<uint32_t>(framebuffers.size()), &command);

	vkr::Scene scene;
	Buffer vertexBuffer;
	Buffer indexBuffer;
	scene.load("../data/models/sibenik/sibenik.dae", vulkanDevice, &vertexBuffer, &indexBuffer);
	// scene.load("../data/models/cube.dae", vulkanDevice, &vertexBuffer, &indexBuffer);

	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
	SetupShader(vulkanDevice.Device, vulkanDevice.PhysicalDevice, &vertShaderModule, &fragShaderModule);

	// TODO: Move this into its own file
	Buffer uniformBuffer;

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::vec4 lightPos;
	};

	scene.camera.type = vkr::Camera::CameraType::firstperson;
	scene.camera.movementSpeed = 7.5f;
	scene.camera.position = { 55.0f, -13.5f, 0.0f };
	scene.camera.setRotation(glm::vec3(5.0f, 90.0f, 0.0f));
	scene.camera.setPerspective(60.0f, (float)vulkanSwapchain.ImageExtent.width / (float)vulkanSwapchain.ImageExtent.height, 0.1f, 256.0f);

	UniformData ubo = {};
	ubo.model = glm::mat4(1.0f);
	ubo.view = scene.camera.matrices.view;
	ubo.projection = scene.camera.matrices.perspective;
	ubo.lightPos = glm::vec4(-scene.camera.position, 1.0f);
	// ubo.projection[1][1] *= -1;
	bool viewUpdated = true;

	// TODO: Use push constants
	VkDeviceSize bufferSize = sizeof(ubo);
	CreateBuffer(vulkanDevice.Device, vulkanDevice.PhysicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffer);

	// Set buffer content
	VK_CHECK(vkMapMemory(vulkanDevice.Device, uniformBuffer.DeviceMemory, 0, VK_WHOLE_SIZE, 0, &uniformBuffer.data));
	// memcpy(uniformBuffer.data, &ubo, sizeof(ubo));
	// vkUnmapMemory(vulkanDevice.Device, uniformBuffer.DeviceMemory);

	Descriptor descriptor;
	SetupDescriptors(vulkanDevice.Device, uniformBuffer.Buffer, 1, &descriptor);

	Pipeline pipeline;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { descriptor.DescriptorSetLayout };
	SetupPipeline(vulkanDevice.Device, vulkanSwapchain.ImageExtent.width, vulkanSwapchain.ImageExtent.height, descriptorSetLayouts, vertShaderModule, fragShaderModule, renderPass, &pipeline);

	VkQueryPool queryPool = CreateQueryPool(vulkanDevice.Device, 128);
	// RecordCommands(command, vertexBuffer, indexBuffer, static_cast<uint32_t>(mesh.Indices.size()), framebuffers, renderPass, descriptor, pipeline, queryPool, sWidth, sHeight);

	VkQueue graphycsQueue = GetQueue(vulkanDevice.Device, vulkanDevice.GraphicsFamilyIndex);
	VkQueue presentQueue = GetQueue(vulkanDevice.Device, vulkanDevice.PresentFamilyIndex);

	size_t maxFramesInFlight = framebuffers.size();
	SyncObjects syncObjects;

	CreateSyncObjects(vulkanDevice.Device, maxFramesInFlight, &syncObjects);

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

			RecordCommands(vulkanDevice.Device, syncObjects, command, vertexBuffer, indexBuffer, scene, framebuffers, renderPass, descriptor, pipeline, queryPool, vulkanSwapchain.ImageExtent.width, vulkanSwapchain.ImageExtent.height, currentFrameIndex);
			double frameGPU = RenderLoop(vulkanDevice.Device, vulkanDevice.PhysicalDeviceProperties, vulkanSwapchain.Swapchain, command, queryPool, graphycsQueue, presentQueue, syncObjects, currentFrameIndex, windowParams);
			currentFrameIndex = (currentFrameIndex + 1) % maxFramesInFlight;

			auto frameCPUEnd = std::chrono::high_resolution_clock::now();
			auto frameCPU = (frameCPUEnd - frameCPUStart).count() * 1e-6;

			frameCpuAvg = frameCpuAvg * 0.95 + frameCPU * 0.05;
			frameGpuAvg = frameGpuAvg * 0.95 + frameGPU * 0.05;

			char title[256];
			sprintf(title, "VRK - cpu: %.2f ms - gpu: %.2f ms", frameCpuAvg, frameGpuAvg);

			scene.camera.keys.up = keyboardInput.up;
			scene.camera.keys.down = keyboardInput.down;
			scene.camera.keys.left = keyboardInput.left;
			scene.camera.keys.right = keyboardInput.right;

			scene.camera.update(frameCPU / 1000.0f);
			if (scene.camera.moving())
			{
				ubo.view = scene.camera.matrices.view;
				viewUpdated = true;
			}

#if _WIN32
			SetWindowTextA(windowParams.HWnd, title);
#endif
		}
	}

	vkDeviceWaitIdle(vulkanDevice.Device);

	// Cleanup
	DestroyQueryPool(vulkanDevice.Device, queryPool);
	DestroySyncObjects(vulkanDevice.Device, &syncObjects);
	DestroyPipeline(vulkanDevice.Device, &pipeline);
	DestroyDescriptor(vulkanDevice.Device, &descriptor);
	DestroyShaderModule(vulkanDevice.Device, &vertShaderModule);
	DestroyShaderModule(vulkanDevice.Device, &fragShaderModule);

	DestroyBuffer(vulkanDevice.Device, &vertexBuffer);
	DestroyBuffer(vulkanDevice.Device, &indexBuffer);
	DestroyBuffer(vulkanDevice.Device, &uniformBuffer);

	DestroyCommandBuffer(vulkanDevice.Device, &command);
	DestroyBufferImage(vulkanDevice.Device, &depthBufferImage);
	DestroyRenderPass(vulkanDevice.Device, &renderPass, &framebuffers);

	vulkanSwapchain.Destroy();
	vulkanDevice.Destroy();

	return (int)msg.wParam;
}
