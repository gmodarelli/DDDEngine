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

		void init();
		void render();
		void cleanup();

	private:
		//
		// Window functions and data
		//
		int width;
		int height;
		GLFWwindow* window = NULL;
		bool init_window();

		void main_loop();

		//
		// Vulkan functions and data
		// TODO: Eventually all this code will be moved to separate structs
		// on a layer below the Renderer
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
		// Vulkan Physical Device
		uint32_t available_gpu_count = 0;
		VkPhysicalDevice* available_gpus = nullptr;
		VkPhysicalDeviceProperties* available_gpu_properties = nullptr;
		VkPhysicalDeviceFeatures* available_gpu_features = nullptr;
		VkPhysicalDevice gpu = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties gpu_properties = {};
		VkPhysicalDeviceFeatures gpu_features = {};
		VkPhysicalDeviceFeatures gpu_enabled_features = {};
		bool vulkan_pick_suitable_gpu();
		// Vulkan Logical Device
		VkDevice vulkan_device = VK_NULL_HANDLE;
		bool vulkan_create_device();
		// Vulkan Queues
		uint32_t vulkan_graphics_family_index = VK_QUEUE_FAMILY_IGNORED;
		uint32_t vulkan_transfer_family_index = VK_QUEUE_FAMILY_IGNORED;
		VkQueue vulkan_graphics_queue = VK_NULL_HANDLE;
		VkQueue vulkan_transfer_queue = VK_NULL_HANDLE;
		void vulkan_retrieve_queues();


	}; // struct Renderer
} // namespace Renderer
