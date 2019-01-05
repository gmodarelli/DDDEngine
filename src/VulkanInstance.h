#ifndef VULKAN_INSTANCE_H_
#define VULKAN_INSTANCE_H_

#include <stdint.h>
#include <vector>
#include <stdio.h>

void SetupVulkanInstance(WindowParameters window, VkInstance* outInstance, VkSurfaceKHR* outSurface, VkDebugReportCallbackEXT* outErrorCallback, VkDebugReportCallbackEXT* outWarningCallback)
{
	// Load the vulkan library
	VK_CHECK(volkInitialize());

	// Layer Properties
	uint32_t count = 0;

	VK_CHECK(vkEnumerateInstanceLayerProperties(&count, nullptr));
	R_ASSERT(count > 0 && L"Could not find any instance layer properties");

	std::vector<VkLayerProperties> availableLayers(count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&count, availableLayers.data()));

	LogInstanceLayers(&availableLayers);

	// Extensions Properties
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
	R_ASSERT(count > 0 && L"Could not find any instance extension properties");

	std::vector<VkExtensionProperties> availableExtensions(count);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, availableExtensions.data()));

	LogInstanceExtensions(&availableExtensions);

	std::vector<const char*> desiredExtensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,

#ifdef VK_USE_PLATFORM_WIN32_KHR
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined VK_USE_PLATFORM_XLIB_KHR
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif defined VK_USE_PLATFORM_XCB_KHR
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif

#ifdef ENABLE_VULKAN_DEBUG_CALLBACK
		"VK_EXT_debug_report",
#endif
	};

	VkBool32 supported = CheckExtensionsSupport(&desiredExtensions, &availableExtensions);
	R_ASSERT(supported == VK_TRUE && L"Required instance extensions not supported!");

	{
		uint32_t apiVersion;
		VK_CHECK(vkEnumerateInstanceVersion(&apiVersion));

		VkApplicationInfo ai = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		ai.pApplicationName = "Vulkan Renderer";
		ai.engineVersion = VK_MAKE_VERSION(0, 0, 1);
		ai.apiVersion = apiVersion;

		VkInstanceCreateInfo ici = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		ici.pApplicationInfo = &ai;
		ici.enabledExtensionCount = static_cast<uint32_t>(desiredExtensions.size());
		ici.ppEnabledExtensionNames = desiredExtensions.data();

#ifdef ENABLE_VULKAN_DEBUG_CALLBACK
		const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
		ici.enabledLayerCount = ARRAYSIZE(layers);
		ici.ppEnabledLayerNames = layers;
#endif

		VK_CHECK(vkCreateInstance(&ici, nullptr, outInstance));
		volkLoadInstance(*outInstance);
	}

#ifdef ENABLE_VULKAN_DEBUG_CALLBACK
	{
		VkDebugReportCallbackCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };

		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT;
		createInfo.pfnCallback = debugReportErrorCallback;
		VK_CHECK(vkCreateDebugReportCallbackEXT(*outInstance, &createInfo, nullptr, outErrorCallback));

		createInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		createInfo.pfnCallback = debugReportWarningCallback;
		VK_CHECK(vkCreateDebugReportCallbackEXT(*outInstance, &createInfo, nullptr, outWarningCallback));
	}
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR

	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(*outInstance, "vkCreateWin32SurfaceKHR");
	R_ASSERT(vkCreateWin32SurfaceKHR != nullptr);

	VkWin32SurfaceCreateInfoKHR sci = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	sci.hinstance = window.Hinstance;
	sci.hwnd = window.HWnd;

	VK_CHECK(vkCreateWin32SurfaceKHR(*outInstance, &sci, nullptr, outSurface));

#elif defined VK_USE_PLATFORM_XLIB_KHR

	PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(*outInstance, "vkCreateXlibSurfaceKHR");
	R_ASSERT(vkCreateXlibSurfaceKHR != nullptr);

	VkXlibSurfaceCreateInfoKHR sci = { VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR };
	sci.dpy = window.Dpy;
	sci.Window = window.Window;

	VK_CHECK(vkCreateXlibSurfaceKHR(*outInstance, &sci, nullptr, outSurface));

#elif defined VK_USE_PLATFORM_XCB_KHR

	PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)vkGetInstanceProcAddr(*outInstance, "vkCreateXcbSurfaceKHR");
	VK_CHECK(vkCreateXcbSurfaceKHR != nullptr);

	VkXcbSurfaceCreateInfoKHR sci = { VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR };
	sci.connection = window.Connection;
	sci.window = window.Window;

	VK_CHECK(vkCreateXcbSurfaceKHR(*outInstance, &sci, nullptr, outSurface));

#endif
}

void DestroyInstance(VkInstance* outInstance)
{
	vkDestroyInstance(*outInstance, nullptr);
	*outInstance = nullptr;
}

void DestroyDebugReportCallback(VkInstance instance, VkDebugReportCallbackEXT* outCallback)
{
	vkDestroyDebugReportCallbackEXT(instance, *outCallback, nullptr);
	*outCallback = nullptr;
}

#endif // VULKAN_INSTANCE_H_