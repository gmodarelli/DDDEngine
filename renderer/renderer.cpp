#include "renderer.h"
#include <cassert>
#include <cstring>
#include <stdio.h>

namespace Renderer
{

Renderer::Renderer(int width, int height) : width(width), height(height) {}
Renderer::Renderer(GLFWwindow* window) : window(window)
{
	glfwGetWindowSize(window, &width, &height);
}

void Renderer::init()
{
	init_window();
	vulkan_init();
}

void Renderer::render()
{
	main_loop();
}

void Renderer::main_loop()
{
	while (!glfwWindowShouldClose(window))
		glfwPollEvents();
}

void Renderer::cleanup()
{
	if (vulkan_surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(vulkan_instance, vulkan_surface, nullptr);
		vulkan_surface = VK_NULL_HANDLE;
	}

	vulkan_destroy_debug_report_callback();
	vulkan_destroy_debug_utils_messenger();

	if (vulkan_device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(vulkan_device, nullptr);
		vulkan_device = VK_NULL_HANDLE;
	}

	if (vulkan_instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(vulkan_instance, nullptr);
		vulkan_instance = VK_NULL_HANDLE;
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}

//
// WSI
//
bool Renderer::init_window()
{
	// GLFW was designed to work with OpenGL so we need to tell it not to
	// create an OpenGL context, otherwise we won't be able to create a 
	// VkSurfaceKHR later.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Disable window resizing for now.
	// TODO: Remove this line once we can handle window resize
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	glfwInit();

	window = glfwCreateWindow(width, height, "A Fante", nullptr, nullptr);
	if (window == NULL)
	{
		const char* error;
		glfwGetError(&error);
		if (error)
		{
			printf(error);
			return false;
		}
	}

	return true;
}

// 
// Vulkan
//
// Vulkan Instance
//
bool Renderer::vulkan_init()
{
	VkResult vulkan_result = volkInitialize();
	assert(vulkan_result == VK_SUCCESS);

	bool result = vulkan_create_instance();
	assert(result);

	// TODO: The surface should come from the outside since it is platform dependent
	result = vulkan_create_surface();
	assert(result);

	result = vulkan_pick_suitable_gpu();
	assert(result);

	result = vulkan_create_device();
	assert(result);

	vulkan_retrieve_queues();


	return true;
}

bool Renderer::vulkan_create_instance()
{
	// Query for the supported Vulkan API Version
	uint32_t supported_api_version = volkGetInstanceVersion();
	// Create the application info struct
	VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app_info.apiVersion = supported_api_version;
	app_info.pApplicationName = "A Fante";
	app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	app_info.pEngineName = "A Fante";
	app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);

	VkInstanceCreateInfo create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	create_info.pApplicationInfo = &app_info;

	// Required extensions
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
		if (!vulkan_has_extension(required_extensions[i]))
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
		if (!vulkan_has_layer(validation_layers[i]))
			return false;
	}

	create_info.enabledLayerCount = validation_layer_count;
	create_info.ppEnabledLayerNames = validation_layer_count > 0 ? validation_layers : nullptr;
#endif

	result = vkCreateInstance(&create_info, nullptr, &vulkan_instance);
	assert(result == VK_SUCCESS);
	volkLoadInstance(vulkan_instance);

#ifdef VULKAN_DEBUG_ENABLED
	vulkan_create_debug_report_callback();
	vulkan_create_debug_utils_messenger();
#endif

	return true;
}

bool Renderer::vulkan_has_extension(const char* extension_name)
{
	assert(available_extensions);
	for (uint32_t i = 0; i < available_extensions_count; ++i)
	{
		if (strcmp(extension_name, available_extensions[i].extensionName))
			return true;
	}

	return false;
}

bool Renderer::vulkan_has_layer(const char* layer_name)
{
	assert(available_layers);
	for (uint32_t i = 0; i < available_layer_count; ++i)
	{
		if (strcmp(layer_name, available_layers[i].layerName))
			return true;
	}

	return false;
}

void Renderer::vulkan_create_debug_report_callback()
{
	assert(vkCreateDebugReportCallbackEXT);

	VkDebugReportCallbackCreateInfoEXT create_info = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
	create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	create_info.pfnCallback = vulkan_debug_callback;
	create_info.pUserData = this;
	VkResult result = vkCreateDebugReportCallbackEXT(vulkan_instance, &create_info, nullptr, &vulkan_debug_report_callback);
	assert(result == VK_SUCCESS);
}

void Renderer::vulkan_destroy_debug_report_callback()
{
	assert(vkDestroyDebugReportCallbackEXT);

	if (vulkan_debug_report_callback != VK_NULL_HANDLE)
		vkDestroyDebugReportCallbackEXT(vulkan_instance, vulkan_debug_report_callback, nullptr);
}

void Renderer::vulkan_destroy_debug_utils_messenger()
{
	assert(vkDestroyDebugUtilsMessengerEXT);

	if (vulkan_debug_utils_messenger != VK_NULL_HANDLE)
		vkDestroyDebugUtilsMessengerEXT(vulkan_instance, vulkan_debug_utils_messenger, nullptr);
}

void Renderer::vulkan_create_debug_utils_messenger()
{
	assert(vkCreateDebugUtilsMessengerEXT);

	VkDebugUtilsMessengerCreateInfoEXT create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = vulkan_debug_messenger;
	create_info.pUserData = this;

	VkResult result = vkCreateDebugUtilsMessengerEXT(vulkan_instance, &create_info, nullptr, &vulkan_debug_utils_messenger);
	assert(result == VK_SUCCESS);
}

// 
// Vulkan
//
// Vulkan Physical Device
//
bool Renderer::vulkan_pick_suitable_gpu()
{
	assert(vulkan_surface != VK_NULL_HANDLE);

	// Enumerate all available GPUs on the system
	VkResult result = vkEnumeratePhysicalDevices(vulkan_instance, &available_gpu_count, nullptr);
	assert(result == VK_SUCCESS);

	if (available_gpu_count == 0)
		return false;

	available_gpus = new VkPhysicalDevice[available_gpu_count];
	result = vkEnumeratePhysicalDevices(vulkan_instance, &available_gpu_count, available_gpus);
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
			// Check for graphics queue support
			// TODO: Add check for transfer queue as well
			uint32_t queue_family_count;
			VkQueueFamilyProperties queue_families[16];
			vkGetPhysicalDeviceQueueFamilyProperties(available_gpus[i], &queue_family_count, nullptr);
			assert(queue_family_count <= 16);
			if (queue_family_count > 0)
			{
				vkGetPhysicalDeviceQueueFamilyProperties(available_gpus[i], &queue_family_count, queue_families);
				for (uint32_t q = 0; q < queue_family_count; ++q)
				{
					if (queue_families[q].queueCount > 0 && queue_families[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						if (vulkan_graphics_family_index == VK_QUEUE_FAMILY_IGNORED)
						{
							vulkan_graphics_family_index = q;
						}

						if (vulkan_present_family_index == VK_QUEUE_FAMILY_IGNORED)
						{
							VkBool32 present_support = VK_FALSE;
							vkGetPhysicalDeviceSurfaceSupportKHR(available_gpus[i], q, vulkan_surface, &present_support);

							if (present_support)
							{
								vulkan_present_family_index = q;
							}
						}
					}
				}

				if (vulkan_graphics_family_index != VK_QUEUE_FAMILY_IGNORED && vulkan_present_family_index != VK_QUEUE_FAMILY_IGNORED)
				{
					// Pick the first suitable GPU
					gpu = available_gpus[i];
					gpu_properties = available_gpu_properties[i];
					gpu_features = available_gpu_features[i];
				}
			}
		}
	}

	return (gpu != VK_NULL_HANDLE && vulkan_graphics_family_index != VK_QUEUE_FAMILY_IGNORED && vulkan_present_family_index != VK_QUEUE_FAMILY_IGNORED);
}

bool Renderer::vulkan_create_device()
{
	assert(gpu != VK_NULL_HANDLE);
	assert(vulkan_graphics_family_index != VK_QUEUE_FAMILY_IGNORED);
	assert(vulkan_present_family_index != VK_QUEUE_FAMILY_IGNORED);

	// Queues Information
	VkDeviceQueueCreateInfo queue_create_info[2];
	uint32_t queue_count = 0;
	float queue_priority = 1.0f;

	if (vulkan_graphics_family_index != vulkan_present_family_index)
	{
		queue_count = 2;

		queue_create_info[0] = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queue_create_info[0].queueFamilyIndex = vulkan_graphics_family_index;
		// We only need 1 graphics queue cause even if we start multithreading, all separate command buffers
		// can record commands and then we can submit them all at once to 1 queue on the main thread
		// with a single low-overhead call (vulkan-tutorial.com)
		queue_create_info[0].queueCount = 1;
		queue_create_info[0].pQueuePriorities = &queue_priority;

		queue_create_info[1] = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queue_create_info[1].queueFamilyIndex = vulkan_present_family_index;
		queue_create_info[1].queueCount = 1;
		queue_create_info[1].pQueuePriorities = &queue_priority;
	}
	else
	{
		queue_count = 1;

		queue_create_info[0] = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queue_create_info[0].queueFamilyIndex = vulkan_graphics_family_index;
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

	// Validation Layers
#ifdef VULKAN_DEBUG_ENABLED
	const char* validation_layers[] = { "VK_LAYER_LUNARG_standard_validation" };
	uint32_t validation_layer_count = ARRAYSIZE(validation_layers);

	device_create_info.enabledLayerCount = validation_layer_count;
	device_create_info.ppEnabledLayerNames = validation_layer_count > 0 ? validation_layers : nullptr;
#endif

	VkResult result = vkCreateDevice(gpu, &device_create_info, nullptr, &vulkan_device);
	assert(result == VK_SUCCESS);

	return true;
}

void Renderer::vulkan_retrieve_queues()
{
	assert(vulkan_device != VK_NULL_HANDLE);
	assert(vulkan_graphics_family_index != VK_QUEUE_FAMILY_IGNORED);
	assert(vulkan_present_family_index != VK_QUEUE_FAMILY_IGNORED);

	vkGetDeviceQueue(vulkan_device, vulkan_graphics_family_index, 0, &vulkan_graphics_queue);
	vkGetDeviceQueue(vulkan_device, vulkan_present_family_index, 0, &vulkan_present_queue);
}

//
// Vulkan
//
// Vulkan Surface
//
bool Renderer::vulkan_create_surface()
{
	// TODO: This surface creation should be moved to a platform specific wsi file
	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(vulkan_instance, "vkCreateWin32SurfaceKHR");
	assert(vkCreateWin32SurfaceKHR != nullptr);

	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfaceCreateInfo.hwnd = glfwGetWin32Window(window);
	surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

	VkResult result = vkCreateWin32SurfaceKHR(vulkan_instance, &surfaceCreateInfo, nullptr, &vulkan_surface);
	return (result == VK_SUCCESS);
}

} // namespace Renderer

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
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

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_messenger(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
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
