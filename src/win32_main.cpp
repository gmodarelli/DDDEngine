#include <Windows.h>
#include "vkr/device.h" 
#include "vkr/swapchain.h"

#define ENABLE_VULKAN_DEBUG_CALLBACK
#include "VulkanTools.h"
#include "MeshLoader.h"

#include <assert.h>
#include <stdio.h>

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

	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
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

	Mesh mesh;
	bool meshLoaded = LoadMesh(mesh, "../data/models/kitten.obj");
	R_ASSERT(meshLoaded && "Could not load the mesh");
	Buffer vertexBuffer;
	Buffer indexBuffer;
	CreateBuffersForMesh(vulkanDevice.Device, vulkanDevice.PhysicalDevice, mesh, &vertexBuffer, &indexBuffer);

	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
	Buffer uniformBuffer;
	SetupShaderandUniforms(vulkanDevice.Device, vulkanDevice.PhysicalDevice, vulkanSwapchain.ImageExtent.width, vulkanSwapchain.ImageExtent.height, &vertShaderModule, &fragShaderModule, &uniformBuffer);

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
			RecordCommands(vulkanDevice.Device, syncObjects, command, vertexBuffer, indexBuffer, static_cast<uint32_t>(mesh.Indices.size()), framebuffers, renderPass, descriptor, pipeline, queryPool, vulkanSwapchain.ImageExtent.width, vulkanSwapchain.ImageExtent.height, currentFrameIndex);
			RenderLoop(vulkanDevice.Device, vulkanDevice.PhysicalDeviceProperties, vulkanSwapchain.Swapchain, command, queryPool, graphycsQueue, presentQueue, syncObjects, currentFrameIndex, windowParams);
			currentFrameIndex = (currentFrameIndex + 1) % maxFramesInFlight;
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
