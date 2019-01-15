#pragma once

#define ENABLE_VULKAN_DEBUG_CALLBACK
#include "../VulkanTools.h"

namespace vkr
{
	struct App
	{
		bool ready = false;
		bool resizing = false;

		uint32_t width;
		uint32_t height;
#if _WIN32
		HINSTANCE hInstance;
		HWND window;
#endif

		vkr::VulkanDevice* device;
		vkr::VulkanSwapchain* swapchain;

		VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
		// This format include stencil as well VK_FORMAT_D16_UNORM_S8_UINT
		VkFormat depthFormat = VK_FORMAT_D16_UNORM;
		VkRenderPass renderPass;

		std::vector<VkFramebuffer> framebuffers;
		struct DepthBuffer
		{
			VkImage Image = VK_NULL_HANDLE;
			VkImageView ImageView = VK_NULL_HANDLE;
			VkDeviceMemory ImageMemory = { 0 };
		} depthBuffer;

		vkr::Camera mainCamera;

		App() {}

		~App()
		{
			if (depthBuffer.Image != VK_NULL_HANDLE)
			{
				vkDestroyImageView(device->Device, depthBuffer.ImageView, nullptr);
				vkDestroyImage(device->Device, depthBuffer.Image, nullptr);
				vkFreeMemory(device->Device, depthBuffer.ImageMemory, nullptr);

				depthBuffer.Image = VK_NULL_HANDLE;
				depthBuffer.ImageView = VK_NULL_HANDLE;
				depthBuffer.ImageMemory = { 0 };
			}

			for (uint32_t i = 0; i < framebuffers.size(); ++i)
			{
				vkDestroyFramebuffer(device->Device, framebuffers[i], nullptr);
			}

			if (renderPass != VK_NULL_HANDLE)
			{
				vkDestroyRenderPass(device->Device, renderPass, nullptr);
				renderPass = VK_NULL_HANDLE;
			}

			swapchain->Destroy();
			device->destroy();
		}

		void initVulkan()
		{

		}

#if _WIN32
		bool setupWindow(uint32_t width, uint32_t height, HINSTANCE hInstance, WNDPROC wndProc)
		{
			this->width = width;
			this->height = height;
			this->hInstance = hInstance;

			WNDCLASS windowClass;
			windowClass.style = CS_HREDRAW | CS_VREDRAW;
			windowClass.lpfnWndProc = wndProc;
			windowClass.cbClsExtra = 0;
			windowClass.cbWndExtra = 0;
			windowClass.hInstance = hInstance;
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

			window = CreateWindow(L"MainWnd", L"Renderer", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, adjustedWidth, adjustedHeight, 0, 0, hInstance, 0);
			if (!window)
			{
				MessageBox(0, L"CreateWindow failed", 0, 0);
				return false;
			}

			ShowWindow(window, SW_SHOW);
			SetForegroundWindow(window);
			SetFocus(window);
			// UpdateWindow(window);

			return true;
		}

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

		LRESULT CALLBACK
		handleMessages(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
				}
				else
				{
					printf("The window is active, should start the app\n");
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
				onMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				return 0;

			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
				onMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				return 0;

			case WM_MOUSEMOVE:
				onMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
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
						mainCamera.keys.up = isDown;
					}

					if (wParam == 0x53 || wParam == VK_DOWN)
					{
						mainCamera.keys.down = isDown;
					}

					if (wParam == 0x44 || wParam == VK_RIGHT)
					{
						mainCamera.keys.right = isDown;
					}

					if (wParam == 0x41 || wParam == VK_LEFT)
					{
						mainCamera.keys.left = isDown;
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
		}

		void onMouseDown(WPARAM btnState, int x, int y)
		{
			
		}

		void onMouseUp(WPARAM btnState, int x, int y)
		{
			
		}

		void onMouseMove(WPARAM btnState, int x, int y)
		{
			
		}
#endif

		void prepare()
		{
			VkQueueFlags requiredQueues = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
			device = new vkr::VulkanDevice(hInstance, window, requiredQueues);

			VkSurfaceFormatKHR desiredFormat { VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
			VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

			swapchain = new vkr::VulkanSwapchain(*device, desiredFormat, desiredPresentMode, width, height);

			prepareRenderPass();
			prepareDepthBuffer();
			prepareFrameBuffers();

			initMainCamera();
		}

	private:

		void prepareRenderPass()
		{
			VkAttachmentDescription pass[2] = {};

			// 0 - Color screen buffer
			pass[0].format = colorFormat;
			pass[0].samples = VK_SAMPLE_COUNT_1_BIT;
			pass[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			pass[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			pass[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			pass[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			pass[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			pass[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			
			// 1 - Depth buffer
			pass[1].format = depthFormat;
			pass[1].samples = VK_SAMPLE_COUNT_1_BIT;
			pass[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			pass[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			pass[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			pass[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			pass[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			pass[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef = {};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// Create one main subpass for the render pass
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			// Create the main render pass
			VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
			renderPassInfo.attachmentCount = 2;
			renderPassInfo.pAttachments = pass;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;

			VKR_CHECK(vkCreateRenderPass(device->Device, &renderPassInfo, nullptr, &renderPass), "Failed to create the render pass");
		}

		void prepareDepthBuffer()
		{
			// Create a depth image
			VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = depthFormat;  
			imageInfo.extent = { swapchain->ImageExtent.width, swapchain->ImageExtent.height, 1 };
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.queueFamilyIndexCount = 0;
			imageInfo.pQueueFamilyIndices = nullptr;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			VKR_CHECK(vkCreateImage(device->Device, &imageInfo, nullptr, &depthBuffer.Image), "Failed to create depth buffer image");

			// Query for the memory requirements of the depth buffer
			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(device->Device, depthBuffer.Image, &memoryRequirements);

			// Allocate memory for the depth buffer
			VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
			allocateInfo.allocationSize = memoryRequirements.size;
			allocateInfo.memoryTypeIndex = vkr::findMemoryType(device->PhysicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			depthBuffer.ImageMemory = { 0 };
			VKR_CHECK(vkAllocateMemory(device->Device, &allocateInfo, nullptr, &depthBuffer.ImageMemory), "Failed to allocate memory for the depth buffer");
			VKR_CHECK(vkBindImageMemory(device->Device, depthBuffer.Image, depthBuffer.ImageMemory, 0), "Faild to bind memory to the depth buffer");

			// Create the depth image view
			VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			VkImageViewCreateInfo imageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			imageViewInfo.image = depthBuffer.Image;
			imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewInfo.format = depthFormat;
			imageViewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
			imageViewInfo.subresourceRange.aspectMask = aspectMask;
			imageViewInfo.subresourceRange.baseMipLevel = 0;
			imageViewInfo.subresourceRange.levelCount = 1;
			imageViewInfo.subresourceRange.baseArrayLayer = 0;
			imageViewInfo.subresourceRange.layerCount = 1;

			VKR_CHECK(vkCreateImageView(device->Device, &imageViewInfo, nullptr, &depthBuffer.ImageView), "Failed to create depth image view");
		}

		void prepareFrameBuffers()
		{
			framebuffers.resize(swapchain->ImageCount);
			for (uint32_t i = 0; i < swapchain->ImageCount; ++i)
			{
				VkImageView frameBufferAttachments[2] = { 0 };
				frameBufferAttachments[0] = swapchain->ImageViews[i];
				frameBufferAttachments[1] = depthBuffer.ImageView;

				VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
				createInfo.renderPass = renderPass;
				createInfo.attachmentCount = 2;
				createInfo.pAttachments = frameBufferAttachments;
				createInfo.width = swapchain->ImageExtent.width;
				createInfo.height = swapchain->ImageExtent.height;
				createInfo.layers = 1;

				VK_CHECK(vkCreateFramebuffer(device->Device, &createInfo, nullptr, &framebuffers[i]));
			}
		}

		void initMainCamera()
		{
			mainCamera.type = vkr::Camera::CameraType::firstperson;
			mainCamera.movementSpeed = 7.5f;
			mainCamera.position = { 55.0f, -13.5f, 0.0f };
			mainCamera.setRotation(glm::vec3(5.0f, 90.0f, 0.0f));
			mainCamera.setPerspective(60.0f, (float)swapchain->ImageExtent.width / (float)swapchain->ImageExtent.height, 0.1f, 256.0f);
		}
	};

}
