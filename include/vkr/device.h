#pragma once

#ifdef _WIN32
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#endif

#include "volk.h"

#include "vkr/types.h"
#include "vkr/utils.h"
#include <vector>

namespace vkr
{
	struct VulkanDevice
	{
		// Vulkan Instance
		VkInstance Instance = VK_NULL_HANDLE;

		// Instance Layers
		uint32_t AvailableLayersCount;
		VkLayerProperties AvailableLayers[256];
		const char* EnabledLayerNames[16];
		uint32_t EnabledLayersCount = 0;

		// Instance Extensions
		uint32_t AvailableExtensionsCount;
		VkExtensionProperties AvailableExtensions[256];
		const char* EnabledExtensionNames[16];
		uint32_t EnabledExtensionsCount = 0;

		// Vulkan API version
		uint32_t ApiVersion = 0;

		// Surface
		VkSurfaceKHR Surface = VK_NULL_HANDLE;

		// GPU
		VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties PhysicalDeviceProperties = {};
		VkPhysicalDeviceFeatures PhysicalDeviceEnabledFeatures = {};
		uint32_t AvailablePhysicalDeviceExtensionCount;
		VkExtensionProperties AvailablePhysicalDeviceExtensions[256];
		const char* EnabledPhysicalDeviceExtensions[16];
		uint32_t EnabledPhysicalDeviceExtensionsCount = 0;

		// Logical Device
		VkDevice Device = VK_NULL_HANDLE;

		// Queue Indices
		uint32_t GraphicsFamilyIndex = UINT32_MAX;
		uint32_t PresentFamilyIndex  = UINT32_MAX;
		uint32_t TransferFamilyIndex = UINT32_MAX;
		uint32_t ComputeFamilyIndex  = UINT32_MAX;

#if _DEBUG
		// Errors and Warnings Callbacks
		VkDebugReportCallbackEXT ErrorCallback = VK_NULL_HANDLE;
		VkDebugReportCallbackEXT WarningCallback = VK_NULL_HANDLE;
#endif

		VulkanDevice()
		{

		}

		VulkanDevice(WindowParameters windowParameters, VkQueueFlags requiredQueues)
		{
			// Load the Vulkan Library
			VKR_CHECK(volkInitialize(), "Failed to load the Vulkan Library");

			enableInstanceLayers();
			enableInstanceExtensions();
			createInstance();

			createDebugCallbacks();

			createSurface(windowParameters);
			pickPhysicalDevice(requiredQueues);
		}

		~VulkanDevice()
		{
		}

		void Destroy()
		{
			if (Device != VK_NULL_HANDLE)
			{
				vkDestroyDevice(Device, nullptr);
				Device = VK_NULL_HANDLE;
			}
#if _DEBUG
			if (ErrorCallback != VK_NULL_HANDLE)
			{
				vkDestroyDebugReportCallbackEXT(Instance, ErrorCallback, nullptr);
				ErrorCallback = VK_NULL_HANDLE;
			}

			if (WarningCallback != VK_NULL_HANDLE)
			{
				vkDestroyDebugReportCallbackEXT(Instance, WarningCallback, nullptr);
				WarningCallback = VK_NULL_HANDLE;
			}
#endif
			if (Surface != VK_NULL_HANDLE)
			{
				vkDestroySurfaceKHR(Instance, Surface, nullptr);
				Surface = VK_NULL_HANDLE;
			}

			if (Instance != VK_NULL_HANDLE)
			{
				vkDestroyInstance(Instance, nullptr);
				Instance = VK_NULL_HANDLE;
			}
		}

	private:

		void enableInstanceExtensions()
		{
			// Enumerate Instance Extensions
			VKR_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &AvailableExtensionsCount, nullptr), "Failed to enumerate the instance extensions");
			VKR_ASSERT(AvailableExtensionsCount > 0 && "No instance exntesions found");
			VKR_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &AvailableExtensionsCount, &AvailableExtensions[0]), "Failed to fetch instance extensions");

			const char* requiredExtensions[] =
			{
				VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
				VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
			};

			const char* optionalExtensions[] = 
			{
#if _DEBUG
				VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
			};

			for (uint32_t i = 0; i < ARRAYSIZE(requiredExtensions); ++i)
			{
				VKR_ASSERT(vkr::checkExtensionSupport(requiredExtensions[i], AvailableExtensions, AvailableExtensionsCount) == VK_TRUE);
				EnabledExtensionNames[EnabledExtensionsCount++] = requiredExtensions[i];
			}

			for (uint32_t i = 0; i < ARRAYSIZE(optionalExtensions); ++i)
			{
				if(vkr::checkExtensionSupport(optionalExtensions[i], AvailableExtensions, AvailableExtensionsCount) == VK_TRUE)
					EnabledExtensionNames[EnabledExtensionsCount++] = optionalExtensions[i];
			}
		}

		void enableInstanceLayers()
		{
			// Enumerate Instance Layers
			VKR_CHECK(vkEnumerateInstanceLayerProperties(&AvailableLayersCount, nullptr), "Failed to enumerate the instance layers");
			VKR_ASSERT(AvailableLayersCount > 0 && "No instance layers found");
			VKR_CHECK(vkEnumerateInstanceLayerProperties(&AvailableLayersCount, &AvailableLayers[0]), "Failed to fetch instance layers");

			const char* layers[] = {
#if _DEBUG
				// Enable debug validation layers
				"VK_LAYER_LUNARG_standard_validation",
#endif
			};

			for (uint32_t i = 0; i < ARRAYSIZE(layers); ++i)
			{
				if(vkr::checkLayerSupport(layers[i], AvailableLayers, AvailableLayersCount) == VK_TRUE)
					EnabledLayerNames[EnabledLayersCount++] = layers[i];
			}
		}

		void createInstance()
		{
			// Application Info
			VKR_CHECK(vkEnumerateInstanceVersion(&ApiVersion), "");
			VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
			applicationInfo.pApplicationName = "VKR Vulkan Renderer";
			applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
			applicationInfo.apiVersion = ApiVersion;

			// Create a Vulkan Instance
			VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
			instanceCreateInfo.pApplicationInfo = &applicationInfo;
			instanceCreateInfo.enabledExtensionCount = EnabledExtensionsCount;
			instanceCreateInfo.ppEnabledExtensionNames = EnabledExtensionsCount > 0 ? EnabledExtensionNames : nullptr;
			instanceCreateInfo.enabledLayerCount = EnabledLayersCount;
			instanceCreateInfo.ppEnabledLayerNames = EnabledLayersCount > 0 ? EnabledLayerNames : nullptr;

			VKR_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &Instance), "Failed to create instance");
			volkLoadInstance(Instance);
		}

		void createDebugCallbacks()
		{
#if _DEBUG
			// Create Debug Callbacks
			VkDebugReportCallbackCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
			debugCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT;
			debugCreateInfo.pfnCallback = vkr::debugReportErrorCallback;
			VKR_CHECK(vkCreateDebugReportCallbackEXT(Instance, &debugCreateInfo, nullptr, &ErrorCallback), "Failed to create the debug error callback");
			debugCreateInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			debugCreateInfo.pfnCallback = vkr::debugReportWarningCallback;
			VKR_CHECK(vkCreateDebugReportCallbackEXT(Instance, &debugCreateInfo, nullptr, &WarningCallback), "Failed to create the debug warning callback");
#endif
		}

		void createSurface(WindowParameters windowParameters)
		{
#ifdef VK_USE_PLATFORM_WIN32_KHR
			PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(Instance, "vkCreateWin32SurfaceKHR");
			VKR_ASSERT(vkCreateWin32SurfaceKHR != nullptr);

			VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
			surfaceCreateInfo.hinstance = windowParameters.Hinstance;
			surfaceCreateInfo.hwnd = windowParameters.HWnd;

			VKR_CHECK(vkCreateWin32SurfaceKHR(Instance, &surfaceCreateInfo, nullptr, &Surface), "Failed to create the Surface");
#endif
		}

		uint32_t getQueueFamilyIndex(VkQueueFamilyProperties* queueFamilies, uint32_t queueFamilyCount, VkQueueFlagBits queueFlags)
		{
			// Dedicated queue for compute
			// Try to find a queue family index that supports compute but not graphics
			if (queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				for (uint32_t i = 0; i < queueFamilyCount; ++i)
				{
					if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
					{
						return i;
						break;
					}
				}
			}

			// Dedicated queue for transfer
			// Try to find a queue family index that supports transfer but not graphics and compute
			if (queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				for (uint32_t i = 0; i < queueFamilyCount; ++i)
				{
					if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
					{
						return i;
						break;
					}
				}
			}

			// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				if (queueFamilies[i].queueFlags & queueFlags)
				{
					return i;
					break;
				}
			}

			VKR_ASSERT(!"Failed to find a queue family index");
		}

		void pickPhysicalDevice(VkQueueFlags requiredQueues)
		{
			uint32_t deviceCount;
			VKR_CHECK(vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr), "Failed to enumerate physical devices");
			VKR_ASSERT(deviceCount > 0 && "Failed to find physical devices");
			VkPhysicalDevice* devices = new VkPhysicalDevice[deviceCount];
			uint32_t maxScore = 0;
			uint32_t deviceIndex = 0;

			VKR_CHECK(vkEnumeratePhysicalDevices(Instance, &deviceCount, devices), "Failed to enumerate physical devices");

			for (uint32_t i = 0; i < deviceCount; ++i)
			{
				uint32_t currentScore = 0;

				VkPhysicalDeviceProperties deviceProperties;
				memset(&deviceProperties, 0, sizeof(deviceProperties));
				vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

				if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
					currentScore += 1000;
				else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
					currentScore += 100;
				else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
					currentScore += 10;

				VkPhysicalDeviceFeatures features;
				vkGetPhysicalDeviceFeatures(devices[i], &features);

				if (features.geometryShader == VK_TRUE)
					currentScore += 100;

				if (features.shaderClipDistance == VK_TRUE)
					currentScore += 100;

				if (currentScore > maxScore)
				{
					maxScore = currentScore;
					deviceIndex = i;
				}
			}

			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(devices[deviceIndex], &deviceProperties);

			PhysicalDevice = devices[deviceIndex];
			PhysicalDeviceProperties = deviceProperties;
			PhysicalDeviceEnabledFeatures = {};
			PhysicalDeviceEnabledFeatures.geometryShader = VK_TRUE;
			PhysicalDeviceEnabledFeatures.shaderClipDistance = VK_TRUE;

			delete[] devices;

			VKR_ASSERT(PhysicalDevice != VK_NULL_HANDLE && "Failed to find a suitable physical device");

			uint32_t queueCount;
			vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueCount, nullptr);
			VKR_ASSERT(queueCount > 0 && "No queue families found");
			VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[queueCount];
			vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueCount, &queueFamilies[0]);

			float queuePriorities[] = { 1.0f };
			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

			if (requiredQueues & VK_QUEUE_GRAPHICS_BIT)
			{
				GraphicsFamilyIndex = getQueueFamilyIndex(queueFamilies, queueCount, VK_QUEUE_GRAPHICS_BIT);
				PresentFamilyIndex = GraphicsFamilyIndex;
				VkDeviceQueueCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO }; 
				createInfo.queueFamilyIndex = GraphicsFamilyIndex;
				createInfo.queueCount = 1;
				createInfo.pQueuePriorities = queuePriorities;
				queueCreateInfos.push_back(createInfo);
			}

			if (requiredQueues & VK_QUEUE_COMPUTE_BIT)
			{
				ComputeFamilyIndex = getQueueFamilyIndex(queueFamilies, queueCount, VK_QUEUE_COMPUTE_BIT);
				if (ComputeFamilyIndex != GraphicsFamilyIndex)
				{
					VkDeviceQueueCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO }; 
					createInfo.queueFamilyIndex = ComputeFamilyIndex;
					createInfo.queueCount = 1;
					createInfo.pQueuePriorities = queuePriorities;
					queueCreateInfos.push_back(createInfo);
				}
			}
			else
			{
				ComputeFamilyIndex = GraphicsFamilyIndex;
			}

			if (requiredQueues & VK_QUEUE_TRANSFER_BIT)
			{
				TransferFamilyIndex = getQueueFamilyIndex(queueFamilies, queueCount, VK_QUEUE_TRANSFER_BIT);
				if (TransferFamilyIndex != GraphicsFamilyIndex)
				{
					VkDeviceQueueCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO }; 
					createInfo.queueFamilyIndex = TransferFamilyIndex;
					createInfo.queueCount = 1;
					createInfo.pQueuePriorities = queuePriorities;
					queueCreateInfos.push_back(createInfo);
				}
			}
			else
			{
				TransferFamilyIndex = GraphicsFamilyIndex;
			}

			const char* requiredExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

			// Enumerate Device Extensions
			VKR_CHECK(vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &AvailablePhysicalDeviceExtensionCount, nullptr), "Failed to enumerate physical device extensions");
			VKR_ASSERT(AvailablePhysicalDeviceExtensionCount > 0 && "No physical device extensions found");
			VKR_CHECK(vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &AvailablePhysicalDeviceExtensionCount, &AvailablePhysicalDeviceExtensions[0]), "Failed to enumerate physical device extensions");

			for (uint32_t i = 0; i < ARRAYSIZE(requiredExtensions); ++i)
			{
				VKR_ASSERT(vkr::checkExtensionSupport(requiredExtensions[i], AvailablePhysicalDeviceExtensions, AvailablePhysicalDeviceExtensionCount) == VK_TRUE);
				EnabledPhysicalDeviceExtensions[EnabledPhysicalDeviceExtensionsCount++] = requiredExtensions[i];
			}

			VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
			deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
			deviceCreateInfo.enabledExtensionCount = EnabledPhysicalDeviceExtensionsCount;
			deviceCreateInfo.ppEnabledExtensionNames = EnabledPhysicalDeviceExtensionsCount > 0 ? EnabledPhysicalDeviceExtensions : nullptr;
			deviceCreateInfo.pEnabledFeatures = &PhysicalDeviceEnabledFeatures;

			VKR_CHECK(vkCreateDevice(PhysicalDevice, &deviceCreateInfo, nullptr, &Device), "Failed to create logical device");
			volkLoadDevice(Device);

			delete[] queueFamilies;
		}
	};
}
