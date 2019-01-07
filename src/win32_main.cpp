#include <Windows.h>

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
	HINSTANCE hInstance = GetModuleHandle(nullptr);
	HWND windowHandle = NULL;
	SetupWindow(800, 600, hInstance, &windowHandle);
	assert(windowHandle != NULL && L"Window is NULL");

	VkInstance instance = nullptr;
	VkSurfaceKHR surface = nullptr;
	VkDebugReportCallbackEXT errorCallback = nullptr;
	VkDebugReportCallbackEXT warningCallback = nullptr;
	WindowParameters windowParams = { hInstance, windowHandle };

	SetupVulkanInstance(windowParams, &instance, &surface, &errorCallback, &warningCallback);
	assert(instance != nullptr && L"The Vulkan instance is nullptr");
	assert(surface != nullptr && L"The Vulkan surface is nullptr");

	VkPhysicalDevice physicalDevice = nullptr;
	VkDevice device = nullptr;
	QueueFamilyIndices queueFamilyIndices = {};
	VkQueueFlags requiredQueues = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT;
	SetupPhysicalDevice(instance, surface, requiredQueues, &physicalDevice, &device, &queueFamilyIndices);
	
	VkSwapchainKHR swapChain = nullptr;
	std::vector<VkImage> presentImages;
	std::vector<VkImageView> presentImageViews;
	uint32_t sWidth;
	uint32_t sHeight;
	SetupSwapChain(device, physicalDevice, surface, queueFamilyIndices.GraphicsFamilyIndex, &sWidth, &sHeight, &swapChain, &presentImages, &presentImageViews);

	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;
	BufferImage depthBufferImage;
	SetupRenderPass(device, physicalDevice, sWidth, sHeight, &presentImageViews, &renderPass, &framebuffers, &depthBufferImage);

	Command command;
	SetupCommandBuffer(device, physicalDevice, queueFamilyIndices.GraphicsFamilyIndex, static_cast<uint32_t>(framebuffers.size()), &command);

	Mesh mesh;
	bool meshLoaded = LoadMesh(mesh, "../data/models/kitten.obj");
	R_ASSERT(meshLoaded && "Could not load the mesh");
	Buffer vertexBuffer;
	Buffer indexBuffer;
	CreateBuffersForMesh(device, physicalDevice, mesh, &vertexBuffer, &indexBuffer);

	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
	Buffer uniformBuffer;
	SetupShaderandUniforms(device, physicalDevice, sWidth, sHeight, &vertShaderModule, &fragShaderModule, &uniformBuffer);

	Descriptor descriptor;
	SetupDescriptors(device, uniformBuffer.Buffer, 1, &descriptor);

	Pipeline pipeline;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { descriptor.DescriptorSetLayout };
	SetupPipeline(device, sWidth, sHeight, descriptorSetLayouts, vertShaderModule, fragShaderModule, renderPass, &pipeline);

	RecordCommands(command, vertexBuffer, indexBuffer, static_cast<uint32_t>(mesh.Indices.size()), framebuffers, renderPass, descriptor, pipeline, sWidth, sHeight);

	VkQueue graphycsQueue = GetQueue(device, queueFamilyIndices.GraphicsFamilyIndex);
	VkQueue presentQueue = GetQueue(device, queueFamilyIndices.PresentFamilyIndex);

	size_t maxFramesInFlight = framebuffers.size();
	SyncObjects syncObjects;

	CreateSyncObjects(device, maxFramesInFlight, &syncObjects);
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
			RenderLoop(device, swapChain, command, presentImages, graphycsQueue, presentQueue, syncObjects, currentFrameIndex);
			currentFrameIndex = (currentFrameIndex + 1) % maxFramesInFlight;
		}
	}

	vkDeviceWaitIdle(device);

	// Cleanup
	DestroySyncObjects(device, &syncObjects);
	DestroyPipeline(device, &pipeline);
	DestroyDescriptor(device, &descriptor);
	DestroyShaderModule(device, &vertShaderModule);
	DestroyShaderModule(device, &fragShaderModule);

	DestroyBuffer(device, &vertexBuffer);
	DestroyBuffer(device, &indexBuffer);
	DestroyBuffer(device, &uniformBuffer);

	DestroyCommandBuffer(device, &command);
	DestroyBufferImage(device, &depthBufferImage);
	DestroyRenderPass(device, &renderPass, &framebuffers);
	DestroySwapChain(device, &swapChain, &presentImages, &presentImageViews);
	DestroyDevice(&device);
	DestroyDebugReportCallback(instance, &errorCallback);
	DestroyDebugReportCallback(instance, &warningCallback);
	DestroyInstance(&instance);

	return (int)msg.wParam;
}