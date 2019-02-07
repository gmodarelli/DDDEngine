#pragma once

#include "volk.h"

#if _DEBUG
#define VULKAN_DEBUG_ENABLED
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData);
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

namespace Vulkan
{

struct Context
{
	Context();
	~Context() {}

	bool init();
	void cleanup();

	VkInstance get_instance() const;
	VkPhysicalDevice get_gpu() const;
	VkDevice get_device() const;

	bool pick_suitable_gpu(VkSurfaceKHR surface, const char** gpu_required_extensions, uint32_t gpu_required_extension_count);
	bool create_device(const char** device_required_extensions, uint32_t device_required_extension_count);
	void retrieve_queues();

	uint32_t get_graphics_family_index() const;
	uint32_t get_present_family_index() const;
	VkQueue get_graphics_queue() const;
	VkQueue get_present_queue() const;

private:

	bool init_vulkan();
	bool create_instance();

	VkInstance instance = VK_NULL_HANDLE;
	VkPhysicalDevice gpu = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;

	// Vulkan Instance
	//
	// Instance Extensions
	VkExtensionProperties* available_extensions = nullptr;
	uint32_t available_extensions_count = 0;
	bool has_extension(const char* extension_name);
	// Instance Layers
	VkLayerProperties* available_layers = nullptr;
	uint32_t available_layer_count = 0;
	bool has_layer(const char* layer_name);
	// Debug callbacks
	VkDebugReportCallbackEXT debug_report_callback = VK_NULL_HANDLE;
	void create_debug_report_callback();
	void destroy_debug_report_callback();
	VkDebugUtilsMessengerEXT debug_utils_messenger = VK_NULL_HANDLE;
	void create_debug_utils_messenger();
	void destroy_debug_utils_messenger();

	// Vulkan Physical Device
	//
	// Vulkan Physical Device Required Extensions
	bool has_gpu_extension(const char* extension_name, const VkExtensionProperties* gpu_available_extensions, uint32_t gpu_available_extension_count);
	uint32_t available_gpu_count = 0;
	VkPhysicalDevice* available_gpus = nullptr;
	VkPhysicalDeviceProperties* available_gpu_properties = nullptr;
	VkPhysicalDeviceFeatures* available_gpu_features = nullptr;
	VkPhysicalDeviceProperties gpu_properties = {};
	VkPhysicalDeviceFeatures gpu_features = {};
	VkPhysicalDeviceFeatures gpu_enabled_features = {};
	// Vulkan Queues
	uint32_t graphics_family_index = VK_QUEUE_FAMILY_IGNORED;
	uint32_t present_family_index = VK_QUEUE_FAMILY_IGNORED;
	uint32_t transfer_family_index = VK_QUEUE_FAMILY_IGNORED;

	// Vulkan Logical Device
	//
	VkQueue graphics_queue = VK_NULL_HANDLE;
	VkQueue present_queue = VK_NULL_HANDLE;
	VkQueue transfer_queue = VK_NULL_HANDLE;
};

} // namespace Vulkan
