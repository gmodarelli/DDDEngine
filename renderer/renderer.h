#pragma once

#include "volk.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#if _DEBUG
#define VULKAN_DEBUG_ENABLED
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData);
static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_messenger(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

namespace Renderer
{
	struct Renderer
	{
		Renderer(int width, int height);
		Renderer(GLFWwindow* window);

		void run();

	private:
		// Window functions and data
		int width;
		int height;
		GLFWwindow* window = NULL;
		bool init_window();

		//
		// Vulkan functions and data
		//
		// Vulkan Instance
		VkInstance instance = VK_NULL_HANDLE;
		bool vulkan_init();
		bool vulkan_create_instance();
		// Vulkan Instance Extensions
		VkExtensionProperties* available_extensions = nullptr;
		uint32_t available_extensions_count = 0;
		bool vulkan_has_extension(const char* extension_name);
		// Vulkan Instance Layers
		VkLayerProperties* available_layers = nullptr;
		uint32_t available_layer_count = 0;
		bool vulkan_has_layer(const char* layer_name);
		// Vulkan Debug callbacks
		VkDebugReportCallbackEXT vulkan_debug_report_callback = VK_NULL_HANDLE;
		void vulkan_create_debug_report_callback();
		void vulkan_destroy_debug_report_callback();
		VkDebugUtilsMessengerEXT vulkan_debug_utils_messenger = VK_NULL_HANDLE;
		void vulkan_create_debug_utils_messenger();
		void vulkan_destroy_debug_utils_messenger();
		// Vulkan Cleanup

		void main_loop();

		void cleanup();

	}; // struct Renderer
} // namespace Renderer
