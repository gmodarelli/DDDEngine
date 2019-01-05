#ifndef VULKAN_INSTANCE_H_
#define VULKAN_INSTANCE_H_

#include <stdint.h>
#include <assert.h>
#include <vector>
#include <stdio.h>

VkBool32 debugReportErrorCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	char message[4096];
	snprintf(message, ARRAYSIZE(message), "ERROR: %s\n", pMessage);

	printf(message);
#ifdef _WIN32
	OutputDebugStringA(message);
#endif

	assert(!"Validation error encountered!");
	return VK_FALSE;
}

VkBool32 debugReportWarningCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	const char* type =
		(flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
		? "WARNING"
		: "INFO";

	char message[4096];
	snprintf(message, ARRAYSIZE(message), "%s: %s\n", type, pMessage);

	printf(message);
#ifdef _WIN32
	OutputDebugStringA(message);
#endif

	return VK_FALSE;
}

// TODO: The first 2 params should be abstracted to a platorm independent Window struct
void SetupVulkanInstance(HWND windowHandle, HINSTANCE instanceHandle, VkInstance* outInstance, VkSurfaceKHR* outSurface, VkDebugReportCallbackEXT* outErrorCallback, VkDebugReportCallbackEXT* outWarningCallback)
{
	// Load the vulkan library
	VK_CHECK(volkInitialize());

	// Layer Properties
	uint32_t count = 0;

	VK_CHECK(vkEnumerateInstanceLayerProperties(&count, nullptr));
	assert(count > 0 && L"Could not find any instance layer properties");

	std::vector<VkLayerProperties> instanceLayers(count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&count, instanceLayers.data()));

	// Extensions Properties
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
	assert(count > 0 && L"Could not find any instance extension properties");

	std::vector<VkExtensionProperties> instanceExtensions(count);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, instanceExtensions.data()));

	const char* extensions[] = {
		"VK_KHR_surface",
#ifdef _WIN32
		"VK_KHR_win32_surface",
#endif
#ifdef ENABLE_VULKAN_DEBUG_CALLBACK
		"VK_EXT_debug_report",
#endif
	};

	{
		uint32_t apiVersion;
		VK_CHECK(vkEnumerateInstanceVersion(&apiVersion));

		VkApplicationInfo ai = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		ai.pApplicationName = "Vulkan Renderer";
		ai.engineVersion = VK_MAKE_VERSION(0, 0, 1);
		ai.apiVersion = apiVersion;

		VkInstanceCreateInfo ici = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		ici.pApplicationInfo = &ai;
		ici.enabledExtensionCount = ARRAYSIZE(extensions);
		ici.ppEnabledExtensionNames = extensions;

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

	// TODO: Implement alternatives for other platforms
#ifdef _WIN32
	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(*outInstance, "vkCreateWin32SurfaceKHR");
	assert(vkCreateWin32SurfaceKHR != nullptr);

	VkWin32SurfaceCreateInfoKHR sci = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	sci.hinstance = instanceHandle;
	sci.hwnd = windowHandle;

	VK_CHECK(vkCreateWin32SurfaceKHR(*outInstance, &sci, nullptr, outSurface));
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