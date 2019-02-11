#include "context.h"
#include <cassert>
#include <stdio.h>

namespace Vulkan
{

Context::Context()
{
}

bool Context::init()
{
	bool result = init_vulkan();
	assert(result);

	result = create_instance();
	assert(result);

	return true;
}

VkInstance Context::get_instance() const
{
	assert(instance != VK_NULL_HANDLE);
	return instance;
}

VkPhysicalDevice Context::get_gpu() const
{
	assert(gpu != VK_NULL_HANDLE);
	return gpu;
}

VkPhysicalDeviceProperties Context::get_gpu_properties() const
{
	return gpu_properties;
}

VkDevice Context::get_device() const
{
	assert(device != VK_NULL_HANDLE);
	return device;
}

uint32_t Context::get_graphics_family_index() const
{
	return graphics_family_index;
}

uint32_t Context::get_transfer_family_index() const
{
	return transfer_family_index;
}

VkQueue Context::get_graphics_queue() const
{
	return graphics_queue;
}

VkQueue Context::get_transfer_queue() const
{
	return transfer_queue;
}
	
bool Context::init_vulkan()
{
	VkResult result = volkInitialize();
	assert(result == VK_SUCCESS);

	return true;
}

bool Context::create_device(const char** device_required_extensions, uint32_t device_required_extension_count)
{
	assert(gpu != VK_NULL_HANDLE);
	assert(graphics_family_index != VK_QUEUE_FAMILY_IGNORED);
	assert(transfer_family_index != VK_QUEUE_FAMILY_IGNORED);

	// Queues Information
	VkDeviceQueueCreateInfo queue_create_info[2];
	uint32_t queue_count = 0;
	float queue_priority = 1.0f;

	if (graphics_family_index != transfer_family_index)
	{
		queue_count = 2;

		queue_create_info[0] = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queue_create_info[0].queueFamilyIndex = graphics_family_index;
		// We only need 1 graphics queue cause even if we start multithreading, all separate command buffers
		// can record commands and then we can submit them all at once to 1 queue on the main thread
		// with a single low-overhead call (vulkan-tutorial.com)
		queue_create_info[0].queueCount = 1;
		queue_create_info[0].pQueuePriorities = &queue_priority;

		queue_create_info[1] = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queue_create_info[1].queueFamilyIndex = transfer_family_index;
		queue_create_info[1].queueCount = 1;
		queue_create_info[1].pQueuePriorities = &queue_priority;
	}
	else
	{
		queue_count = 1;

		queue_create_info[0] = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queue_create_info[0].queueFamilyIndex = graphics_family_index;
		// We only need 1 graphics queue cause even if we start multithreading, all separate command buffers
		// can record commands and then we can submit them all at once to 1 queue on the main thread
		// with a single low-overhead call (vulkan-tutorial.com)
		queue_create_info[0].queueCount = 1;
		queue_create_info[0].pQueuePriorities = &queue_priority;
	}

	// Device Features to enable
	// For now we don't need any features
	gpu_enabled_features = {};

	// Device create info
	VkDeviceCreateInfo device_create_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	device_create_info.pQueueCreateInfos = queue_create_info;
	device_create_info.queueCreateInfoCount = queue_count;
	device_create_info.pEnabledFeatures = &gpu_enabled_features;
	device_create_info.enabledExtensionCount = device_required_extension_count;
	device_create_info.ppEnabledExtensionNames = device_required_extension_count > 0 ? device_required_extensions : nullptr;

	// Validation Layers
#ifdef VULKAN_DEBUG_ENABLED
	const char* validation_layers[] = { "VK_LAYER_LUNARG_standard_validation" };
	uint32_t validation_layer_count = ARRAYSIZE(validation_layers);

	device_create_info.enabledLayerCount = validation_layer_count;
	device_create_info.ppEnabledLayerNames = validation_layer_count > 0 ? validation_layers : nullptr;
#endif


	VkResult result = vkCreateDevice(gpu, &device_create_info, nullptr, &device);
	assert(result == VK_SUCCESS);

	return true;
}

bool Context::create_instance()
{
	// Query for the supported Vulkan API Version
	uint32_t supported_api_version = volkGetInstanceVersion();
	// Create the application info struct
	VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app_info.apiVersion = supported_api_version;
	app_info.pApplicationName = "73 Games";
	app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	app_info.pEngineName = "73 Games";
	app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);

	VkInstanceCreateInfo create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	create_info.pApplicationInfo = &app_info;

	// Required extensions
	// TODO: VK_KHR_WIN32_SURFACE_EXTENSION_NAME should come from the WSI
	const char* required_extensions[] = 
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif

#ifdef VULKAN_DEBUG_ENABLED
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
	};

	uint32_t required_extension_count = ARRAYSIZE(required_extensions);

	// Query for available global extensions
	VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, nullptr);
	assert(result == VK_SUCCESS);
	available_extensions = new VkExtensionProperties[available_extensions_count];
	result = vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, available_extensions);
	assert(result == VK_SUCCESS);

	// Check for extensions support
	for (uint32_t i = 0; i < required_extension_count; ++i)
	{
		if (!has_extension(required_extensions[i]))
			return false;
	}

	create_info.enabledExtensionCount = required_extension_count;
	create_info.ppEnabledExtensionNames = required_extension_count > 0 ? required_extensions : nullptr;

	// Validation Layers
#ifdef VULKAN_DEBUG_ENABLED
	const char* validation_layers[] = { "VK_LAYER_LUNARG_standard_validation" };
	uint32_t validation_layer_count = ARRAYSIZE(validation_layers);

	// Query for available layers
	result = vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);
	assert(result == VK_SUCCESS);
	available_layers = new VkLayerProperties[available_layer_count];
	result = vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);
	assert(result == VK_SUCCESS);

	// Check for layers support
	for (uint32_t i = 0; i < validation_layer_count; ++i)
	{
		if (!has_layer(validation_layers[i]))
			return false;
	}

	create_info.enabledLayerCount = validation_layer_count;
	create_info.ppEnabledLayerNames = validation_layer_count > 0 ? validation_layers : nullptr;
#endif

	result = vkCreateInstance(&create_info, nullptr, &instance);
	assert(result == VK_SUCCESS);
	volkLoadInstance(instance);

#ifdef VULKAN_DEBUG_ENABLED
	create_debug_report_callback();
	create_debug_utils_messenger();
#endif

	return true;
	return true;
}

void Context::cleanup()
{
	destroy_debug_report_callback();
	destroy_debug_utils_messenger();

	if (device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(device, nullptr);
		device = VK_NULL_HANDLE;
	}

	if (instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(instance, nullptr);
		instance = VK_NULL_HANDLE;
	}
}

bool Context::has_extension(const char* extension_name)
{
	assert(available_extensions);
	for (uint32_t i = 0; i < available_extensions_count; ++i)
	{
		if (strcmp(extension_name, available_extensions[i].extensionName))
			return true;
	}

	return false;
}

bool Context::has_layer(const char* layer_name)
{
	assert(available_layers);
	for (uint32_t i = 0; i < available_layer_count; ++i)
	{
		if (strcmp(layer_name, available_layers[i].layerName))
			return true;
	}

	return false;
}

bool Context::has_gpu_extension(const char* extension_name, const VkExtensionProperties* gpu_available_extensions, uint32_t gpu_available_extension_count)
{
	assert(gpu_available_extensions);

	for (uint32_t i = 0; i < gpu_available_extension_count; ++i)
	{
		if (strcmp(extension_name, gpu_available_extensions[i].extensionName))
			return true;
	}

	return false;
}

bool Context::pick_suitable_gpu(VkSurfaceKHR surface, const char** gpu_required_extensions, uint32_t gpu_required_extension_count)
{
	// Enumerate all available GPUs on the system
	VkResult result = vkEnumeratePhysicalDevices(instance, &available_gpu_count, nullptr);
	assert(result == VK_SUCCESS);

	if (available_gpu_count == 0)
		return false;

	available_gpus = new VkPhysicalDevice[available_gpu_count];
	result = vkEnumeratePhysicalDevices(instance, &available_gpu_count, available_gpus);
	assert(result == VK_SUCCESS);

	// Collect all GPUs properties, features and queue support
	available_gpu_properties = new VkPhysicalDeviceProperties[available_gpu_count];
	available_gpu_features = new VkPhysicalDeviceFeatures[available_gpu_count];

	for (uint32_t i = 0; i < available_gpu_count; ++i)
	{
		vkGetPhysicalDeviceProperties(available_gpus[i], &available_gpu_properties[i]);
		vkGetPhysicalDeviceFeatures(available_gpus[i], &available_gpu_features[i]);

		if (gpu == VK_NULL_HANDLE)
		{
			// Check for required device extensions support
			uint32_t extension_count = 0;
			vkEnumerateDeviceExtensionProperties(available_gpus[i], nullptr, &extension_count, nullptr);
			assert(extension_count > 0 && extension_count <= 256);
			VkExtensionProperties gpu_available_extensions[256];
			vkEnumerateDeviceExtensionProperties(available_gpus[i], nullptr, &extension_count, gpu_available_extensions);
			for (uint32_t ext_id = 0; ext_id < gpu_required_extension_count; ++ext_id)
			{
				// If one of the required device extensions is not supported by the current physical device, we move on to the next
				if (!has_gpu_extension(gpu_required_extensions[ext_id], gpu_available_extensions, extension_count))
					continue;
			}

			// Check for swap chain details support
			// 
			// Surface Format and Color Space
			uint32_t available_format_count = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(available_gpus[i], surface, &available_format_count, nullptr);
			// Present Mode
			uint32_t available_present_mode_count = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(available_gpus[i], surface, &available_present_mode_count, nullptr);

			// The current physical device doesn't support any format or any present mode, we move on to the next
			if (available_format_count == 0 || available_present_mode_count == 0)
				continue;

			// Check for graphics queue support
			QueueFamilyIndices queue_family_indices = find_queue_families(available_gpus[i], surface);
			if (queue_family_indices.is_complete())
			{
				// Pick the first suitable GPU
				gpu = available_gpus[i];
				gpu_properties = available_gpu_properties[i];
				gpu_features = available_gpu_features[i];

				graphics_family_index = queue_family_indices.graphics_index;
				transfer_family_index = queue_family_indices.transfer_index;
			}
		}
	}

	return (gpu != VK_NULL_HANDLE && graphics_family_index != VK_QUEUE_FAMILY_IGNORED && transfer_family_index != VK_QUEUE_FAMILY_IGNORED);
}

QueueFamilyIndices Context::find_queue_families(VkPhysicalDevice gpu, VkSurfaceKHR surface)
{
	QueueFamilyIndices queue_family_indices;

	uint32_t queue_family_count = 0;
	VkQueueFamilyProperties* queue_families;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, nullptr);
	
	if (queue_family_count == 0)
		return queue_family_indices;

	queue_families = new VkQueueFamilyProperties[queue_family_count];
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_families);

	// Look for graphics and present queues
	for (uint32_t i = 0; i < queue_family_count; ++i)
	{
		VkQueueFlags required = VK_QUEUE_GRAPHICS_BIT;

		VkBool32 present_support = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &present_support);

		if (present_support && ((queue_families[i].queueFlags & required) == required))
		{
			queue_family_indices.graphics_index = i;
		}
	}

	// Look for a dedicated transfer queue
	for (uint32_t i = 0; i < queue_family_count; ++i)
	{
		VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;

		if (i != queue_family_indices.graphics_index && ((queue_families[i].queueFlags & required) == required))
		{
			queue_family_indices.transfer_index = i;
		}
	}

	if (queue_family_indices.transfer_index == VK_QUEUE_FAMILY_IGNORED)
	{
		// Look for a shared transfer queue
		for (uint32_t i = 0; i < queue_family_count; ++i)
		{
			VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;

			if ((queue_families[i].queueFlags & required) == required)
			{
				queue_family_indices.transfer_index = i;
			}
		}
	}

	delete[] queue_families;

	return queue_family_indices;
}

void Context::retrieve_queues()
{
	assert(device != VK_NULL_HANDLE);
	assert(graphics_family_index != VK_QUEUE_FAMILY_IGNORED);
	assert(transfer_family_index != VK_QUEUE_FAMILY_IGNORED);

	vkGetDeviceQueue(device, graphics_family_index, 0, &graphics_queue);
	vkGetDeviceQueue(device, transfer_family_index, 0, &transfer_queue);
}

void Context::create_debug_report_callback()
{
	assert(vkCreateDebugReportCallbackEXT);

	VkDebugReportCallbackCreateInfoEXT create_info = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
	create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	create_info.pfnCallback = debug_callback;
	create_info.pUserData = this;
	VkResult result = vkCreateDebugReportCallbackEXT(instance, &create_info, nullptr, &debug_report_callback);
	assert(result == VK_SUCCESS);
}

void Context::destroy_debug_report_callback()
{
	assert(vkDestroyDebugReportCallbackEXT);

	if (debug_report_callback != VK_NULL_HANDLE)
		vkDestroyDebugReportCallbackEXT(instance, debug_report_callback, nullptr);
}

void Context::destroy_debug_utils_messenger()
{
	assert(vkDestroyDebugUtilsMessengerEXT);

	if (debug_utils_messenger != VK_NULL_HANDLE)
		vkDestroyDebugUtilsMessengerEXT(instance, debug_utils_messenger, nullptr);
}

void Context::create_debug_utils_messenger()
{
	assert(vkCreateDebugUtilsMessengerEXT);

	VkDebugUtilsMessengerCreateInfoEXT create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debug_messenger;
	create_info.pUserData = this;

	VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_utils_messenger);
	assert(result == VK_SUCCESS);
}

} // namespace Vulkan

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
#ifdef VULKAN_DEBUG_ENABLED
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		printf("\n[Vulkan]: Error: %s: %s\n", pLayerPrefix, pMessage);
		assert(!"Validation error encountered!");
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		printf("\n[Vulkan]: Warning: %s: %s\n", pLayerPrefix, pMessage);
	}
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		printf("\n[Vulkan]: Performance Warning: %s: %s\n", pLayerPrefix, pMessage);
	}
	else
	{
		printf("\n[Vulkan]: Info: %s: %s\n", pLayerPrefix, pMessage);
	}
#endif

	return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
#ifdef VULKAN_DEBUG_ENABLED
	if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
	{
		printf("\n[Vulkan Messenger]: General: %s: %s\n", pCallbackData->pMessageIdName, pCallbackData->pMessage);
	}
	else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
	{
		printf("\n[Vulkan Messenger]: Performance: %s: %s\n", pCallbackData->pMessageIdName, pCallbackData->pMessage);
	}
	else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
	{
		printf("\n[Vulkan Messenger]: Validation: %s: %s\n", pCallbackData->pMessageIdName, pCallbackData->pMessage);
	}
#endif

	return VK_FALSE;
}
