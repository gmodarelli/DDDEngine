#include "renderer.h"
#include <cassert>
#include <cstring>
#include <stdio.h>
#include <algorithm>

namespace Renderer
{

Renderer::Renderer(int width, int height) : width(width), height(height) {}
Renderer::Renderer(GLFWwindow* window) : window(window)
{
	glfwGetWindowSize(window, &width, &height);
}

void Renderer::init()
{
	bool result = init_window();
	assert(result);

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
	if (vulkan_swapchain_images != nullptr)
	{
		delete[] vulkan_swapchain_images;
	}

	if (vulkan_swapchain_image_views != nullptr)
	{
		for (uint32_t i = 0; i < vulkan_swapchain_image_count; ++i)
		{
			vkDestroyImageView(vulkan_device, vulkan_swapchain_image_views[i], nullptr);
		}

		delete[] vulkan_swapchain_image_views;
	}

	vulkan_destroy_swapchain();

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
	glfwInit();

	// GLFW was designed to work with OpenGL so we need to tell it not to
	// create an OpenGL context, otherwise we won't be able to create a 
	// VkSwapchainKHR later.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	const char* error;
	glfwGetError(&error);
	if (error)
	{
		printf("[WSI]: %s\n", error);
		return false;
	}


	// Disable window resizing for now.
	// TODO: Remove this line once we can handle window resize
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwGetError(&error);
	if (error)
	{
		printf("[WSI]: %s\n", error);
		return false;
	}

	window = glfwCreateWindow(width, height, "A Fante", nullptr, nullptr);
	if (window == NULL)
	{
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

	vulkan_device_required_extension_count = 1;
	vulkan_device_required_extensions = new const char*[vulkan_device_required_extension_count];
	vulkan_device_required_extensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

	// TODO: The surface should come from the outside since it is platform dependent
	result = vulkan_create_surface();
	assert(result);

	result = vulkan_pick_suitable_gpu();
	assert(result);

	result = vulkan_create_device();
	assert(result);

	vulkan_retrieve_queues();

	result = vulkan_create_swapchain();
	assert(result);

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

bool Renderer::vulkan_has_gpu_extension(const char* extension_name, const VkExtensionProperties* gpu_available_extensions, uint32_t gpu_available_extension_count)
{
	assert(gpu_available_extensions);

	for (uint32_t i = 0; i < gpu_available_extension_count; ++i)
	{
		if (strcmp(extension_name, gpu_available_extensions[i].extensionName))
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

	assert(vulkan_device_required_extension_count > 0);
	assert(vulkan_device_required_extensions);

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
			// Check for required device extensions support
			uint32_t extension_count = 0;
			vkEnumerateDeviceExtensionProperties(available_gpus[i], nullptr, &extension_count, nullptr);
			assert(extension_count > 0 && extension_count <= 256);
			VkExtensionProperties gpu_available_extensions[256];
			vkEnumerateDeviceExtensionProperties(available_gpus[i], nullptr, &extension_count, gpu_available_extensions);
			for (uint32_t ext_id = 0; ext_id < vulkan_device_required_extension_count; ++ext_id)
			{
				// If one of the required device extensions is not supported by the current physical device, we move on to the next
				if (!vulkan_has_gpu_extension(vulkan_device_required_extensions[ext_id], gpu_available_extensions, extension_count))
					continue;
			}

			// Check for swap chain details support
			// 
			// Surface Format and Color Space
			uint32_t available_format_count = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(available_gpus[i], vulkan_surface, &available_format_count, nullptr);
			// Present Mode
			uint32_t available_present_mode_count = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(available_gpus[i], vulkan_surface, &available_present_mode_count, nullptr);

			// The current physical device doesn't support any format or any present mode, we move on to the next
			if (available_format_count == 0 || available_present_mode_count == 0)
				continue;


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

				// It the current physical device does not have support for either of graphics or present queue, reset the state
				// and continue with the next physical device
				if (vulkan_graphics_family_index == VK_QUEUE_FAMILY_IGNORED || vulkan_present_family_index == VK_QUEUE_FAMILY_IGNORED)
				{
					vulkan_graphics_family_index = VK_QUEUE_FAMILY_IGNORED;
					vulkan_present_family_index = VK_QUEUE_FAMILY_IGNORED;
					continue;
				}

				// Pick the first suitable GPU
				gpu = available_gpus[i];
				gpu_properties = available_gpu_properties[i];
				gpu_features = available_gpu_features[i];
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
	device_create_info.enabledExtensionCount = vulkan_device_required_extension_count;
	device_create_info.ppEnabledExtensionNames = vulkan_device_required_extension_count > 0 ? vulkan_device_required_extensions : nullptr;

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
	assert(result == VK_SUCCESS);
	return (result == VK_SUCCESS);
}

//
// Vulkan
//
// Vulkan Swapchain
//
bool Renderer::vulkan_create_swapchain()
{
	assert(gpu);
	assert(vulkan_device);
	assert(vulkan_surface);

	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, vulkan_surface, &surface_capabilities);

	vulkan_surface_format = vulkan_find_best_surface_format({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR });
	vulkan_present_mode = vulkan_find_best_present_mode(VK_PRESENT_MODE_MAILBOX_KHR);
	vulkan_surface_extent = vulkan_choose_swapchain_extent(surface_capabilities);

	uint32_t image_count = surface_capabilities.minImageCount + 1;
	// Make sure the surface supports it
	if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
		image_count = surface_capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	create_info.surface = vulkan_surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = vulkan_surface_format.format;
	create_info.imageColorSpace = vulkan_surface_format.colorSpace;
	create_info.imageExtent = vulkan_surface_extent;
	create_info.imageArrayLayers = 1;
	// NOTE: imageUsage specifies what kind of operation we'll use the images in the swapchain for.
	// If we're going to render directly to them we need to use VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT.
	// If we're going to render to a separate image first to perform operations like post-processing,
	// or multi-sampling (?) then we need to use VK_IMAGE_USAGE_TRANSFER_DST_BIT and use a memory
	// operation to transfer the rendered image to a swapchain image.
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// NOTE: VK_SHARING_MODE_EXCLUSIVE offers the best performance, so we should switch to it
	// even if we have queues with different indices. With sharing mode exclusive and image is
	// owned by a queue family at a time and ownership must be explicitly transfered before
	// using it in another queue family.
	// TODO: Check the specs
	if (vulkan_graphics_family_index != vulkan_present_family_index)
	{
		uint32_t queue_family_indices[] = { vulkan_graphics_family_index, vulkan_present_family_index };
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else
	{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = surface_capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = vulkan_present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(vulkan_device, &create_info, nullptr, &vulkan_swapchain);
	if (result != VK_SUCCESS)
		return false;

	// Retrieve the Swapchain VkImage handles. These images are create by the implementation of the swapchain
	// and will be automatically cleaned up
	vulkan_swapchain_image_count = image_count;
	vkGetSwapchainImagesKHR(vulkan_device, vulkan_swapchain, &vulkan_swapchain_image_count, nullptr);
	assert(vulkan_swapchain_image_count == image_count);
	vulkan_swapchain_images = new VkImage[vulkan_swapchain_image_count];
	vkGetSwapchainImagesKHR(vulkan_device, vulkan_swapchain, &vulkan_swapchain_image_count, vulkan_swapchain_images);
	// Create a VkImageView per VkImage
	vulkan_swapchain_image_views = new VkImageView[vulkan_swapchain_image_count];
	for (uint32_t i = 0; i < vulkan_swapchain_image_count; ++i)
	{
		VkImageViewCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		create_info.image = vulkan_swapchain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = vulkan_surface_format.format;
		// The components are used to swizzle the color channels around for different effects.
		create_info.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		// The subresourceRange describes the image's purporse and which part of the image should be accessed
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(vulkan_device, &create_info, nullptr, &vulkan_swapchain_image_views[i]);
		assert (result == VK_SUCCESS);
	}

	return true;
}

void Renderer::vulkan_destroy_swapchain()
{
	assert(vulkan_swapchain != VK_NULL_HANDLE);
	vkDestroySwapchainKHR(vulkan_device, vulkan_swapchain, nullptr);
}

VkSurfaceFormatKHR Renderer::vulkan_find_best_surface_format(VkSurfaceFormatKHR preferred_format)
{
	assert(gpu);

	uint32_t available_format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, vulkan_surface, &available_format_count, nullptr);
	VkSurfaceFormatKHR* available_formats = new VkSurfaceFormatKHR[available_format_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, vulkan_surface, &available_format_count, available_formats);

	// If the surface has no preferred format
	if (available_format_count == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
	{
		// we pick the one we want
		delete[] available_formats;
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (uint32_t f = 0; f < available_format_count; ++f)
	{
		if (available_formats[f].format == preferred_format.format && available_formats[f].colorSpace == preferred_format.colorSpace)
		{
			delete[] available_formats;
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}
	}

	VkSurfaceFormatKHR best_match = { available_formats[0].format, available_formats[0].colorSpace };
	delete[] available_formats;
	return best_match;
}

VkPresentModeKHR Renderer::vulkan_find_best_present_mode(VkPresentModeKHR preferred_present_mode)
{
	assert(gpu);

	uint32_t available_present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, vulkan_surface, &available_present_mode_count, nullptr);
	VkPresentModeKHR* available_present_modes = new VkPresentModeKHR[available_present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, vulkan_surface, &available_present_mode_count, available_present_modes);

	for (uint32_t p = 0; p < available_present_mode_count; ++p)
	{
		if (available_present_modes[p] == preferred_present_mode)
		{
			delete[] available_present_modes;
			return VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}

	delete[] available_present_modes;
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::vulkan_choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& surface_capabilities)
{
	assert(gpu);
	assert(vulkan_surface);

	// Min/Max Swapchain Images
	// Mix/Max Width and Height 

	if (surface_capabilities.currentExtent.width != UINT32_MAX)
	{
		return surface_capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actual_extent = { width, height };
		actual_extent.width = std::max(surface_capabilities.minImageExtent.width, std::min(surface_capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(surface_capabilities.minImageExtent.height, std::min(surface_capabilities.maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
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
