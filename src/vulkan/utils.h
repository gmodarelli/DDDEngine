#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined __linux__
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined __APPLE__
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include "volk.h"
#include <stdio.h>
#include <string.h>

#if _DEBUG

#include <assert.h>
#include <stdio.h>

#define GM_CHECK(call, message) \
	do { \
		VkResult result = call; \
		assert(result == VK_SUCCESS && message); \
	} while (0);

#define GM_ASSERT(x) assert(x)

#else

#define GM_CHECK(call, message) call
#define GM_ASSERT(x) x

#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(array) sizeof(array) / sizeof(array[0])
#endif

namespace gm
{
	static VkBool32 checkExtensionSupport(const char* extensionName, VkExtensionProperties* availableExtensions, uint32_t extensionsCount)
	{
		VkBool32 supported = VK_FALSE;
		for (uint32_t j = 0; j < extensionsCount; ++j)
		{
			if (strstr(extensionName, availableExtensions[j].extensionName))
			{
				supported = VK_TRUE;
				break;
			}
		}

		return supported;
	}

	static VkBool32 checkLayerSupport(const char* layerName, VkLayerProperties* availableLayers, uint32_t layersCount)
	{
		VkBool32 supported = VK_FALSE;
		for (uint32_t j = 0; j < layersCount; ++j)
		{
			if (strstr(layerName, availableLayers[j].layerName))
			{
				supported = VK_TRUE;
				break;
			}
		}

		return supported;
	}

	VkBool32 debugReportErrorCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
	{
#if _DEBUG
		char message[4096];
		snprintf(message, ARRAYSIZE(message), "\n [ Vulkan Validation Error ]\n%s\n\n", pMessage);

		printf(message);

		GM_ASSERT(!"Validation error encountered!");
		return VK_FALSE;
#else
		return VK_FALSE;
#endif
	}

	VkBool32 debugReportWarningCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
	{
#if _DEBUG
		const char* type =
			(flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
			? "Warning"
			: "Info";

		char message[4096];

		snprintf(message, ARRAYSIZE(message), "\n [ Vulkan Validation %s ]\n%s\n\n", type, pMessage);
		printf(message);

		return VK_FALSE;
#else
		return VK_FALSE;
#endif
	}

	VkBool32 debugReportDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
	{
#if _DEBUG
		char message[4096];

		snprintf(message, ARRAYSIZE(message), "\n [ Vulkan Validation Debug ]\n%s\n\n", pMessage);
		printf(message);

		return VK_FALSE;
#else
		return VK_FALSE;
#endif
	}
}
