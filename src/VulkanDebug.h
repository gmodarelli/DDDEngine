#pragma once
#include "volk.h"

#ifdef _DEBUG
	#include <assert.h>
	#include <iostream>

	#define DDD_ASSERT(value) assert(value)
	// Macro to check Vulkan function results
	#define VK_CHECK(call) \
		do { \
			VkResult result = call; \
			assert(result == VK_SUCCESS); \
		} while (0);

	namespace ddd
	{
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
		{
			std::cerr << "[validation layer]: " << pCallbackData->pMessage << std::endl;
			return VK_FALSE;
		}

		void destroyDebugCallback(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator)
		{
			vkDestroyDebugUtilsMessengerEXT(instance, callback, pAllocator);
		}

		VkDebugUtilsMessengerEXT setupDebugCallback(VkInstance instance, const VkAllocationCallbacks* pAllocator)
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = ddd::debugCallback;
			createInfo.pUserData = nullptr;

			VkDebugUtilsMessengerEXT callback = VK_NULL_HANDLE;
			VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &createInfo, pAllocator, &callback));
			return callback;
		}
	}
#else
#define DDD_ASSERT(value) value
#define VK_CHECK(call) \
	do { \
		VkResult result = call;
	} while (0);
#endif
