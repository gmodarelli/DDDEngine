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

void Renderer::run()
{
	vulkan_init();
	init_window();
	main_loop();
	cleanup();
}

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

bool Renderer::vulkan_init()
{
	VkResult result = volkInitialize();
	assert(result == VK_SUCCESS);
	if (result != VK_SUCCESS)
		return false;

	vulkan_create_instance();

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

	result = vkCreateInstance(&create_info, nullptr, &instance);
	assert(result == VK_SUCCESS);
	volkLoadInstance(instance);

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
	VkResult result = vkCreateDebugReportCallbackEXT(instance, &create_info, nullptr, &vulkan_debug_report_callback);
	assert(result == VK_SUCCESS);
}

void Renderer::vulkan_destroy_debug_report_callback()
{
	assert(vkDestroyDebugReportCallbackEXT);

	if (vulkan_debug_report_callback != VK_NULL_HANDLE)
		vkDestroyDebugReportCallbackEXT(instance, vulkan_debug_report_callback, nullptr);
}

void Renderer::vulkan_destroy_debug_utils_messenger()
{
	assert(vkDestroyDebugUtilsMessengerEXT);

	if (vulkan_debug_utils_messenger != VK_NULL_HANDLE)
		vkDestroyDebugUtilsMessengerEXT(instance, vulkan_debug_utils_messenger, nullptr);
}

void Renderer::vulkan_create_debug_utils_messenger()
{
	assert(vkCreateDebugUtilsMessengerEXT);

	VkDebugUtilsMessengerCreateInfoEXT create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = vulkan_debug_messenger;
	create_info.pUserData = this;

	VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &vulkan_debug_utils_messenger);
	assert(result == VK_SUCCESS);
}

void Renderer::main_loop()
{
	while (!glfwWindowShouldClose(window))
		glfwPollEvents();
}

void Renderer::cleanup()
{
	vulkan_destroy_debug_report_callback();
	vulkan_destroy_debug_utils_messenger();

	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
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
