#include "wsi.h"
#include <stdio.h>
#include <cassert>
#include <algorithm>

namespace Vulkan
{

WSI::WSI(int width, int height) : width(width), height(height) {}

bool WSI::init()
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
		assert(!"Failed to initialize GLFW");
	}

	// Disable window resizing for now.
	// TODO: Remove this line once we can handle window resize
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwGetError(&error);
	if (error)
	{
		printf("[WSI]: %s\n", error);
		assert(!"Failed to initialize GLFW");
	}

	window = glfwCreateWindow(width, height, "73 Games", nullptr, nullptr);
	if (window == NULL)
	{
		glfwGetError(&error);
		if (error)
		{
			printf("[WSI]: %s\n", error);
			assert(false);
		}
	}

	context = new Vulkan::Context();
	bool result = context->init();
	assert(result);

	surface = create_surface(context->get_instance());

	const char* device_required_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	result = context->pick_suitable_gpu(surface, device_required_extensions, ARRAYSIZE(device_required_extensions));
	assert(result);

	result = context->create_device(device_required_extensions, ARRAYSIZE(device_required_extensions));
	assert(result);

	context->retrieve_queues();

	swapchain = create_swapchain();

	return true;
}

void WSI::cleanup()
{
	if (swapchain != VK_NULL_HANDLE)
	{
		destroy_swapchain();
	}

	if (surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(context->get_instance(), surface, nullptr);
		surface = VK_NULL_HANDLE;
	}

	context->cleanup();

	glfwDestroyWindow(window);
	glfwTerminate();
}


int WSI::get_width()
{
	return surface_extent.width;
}

int WSI::get_height()
{
	return surface_extent.height;
}

bool WSI::alive()
{
	glfwPollEvents();
	return !glfwWindowShouldClose(window);
}

VkInstance WSI::get_instance() const
{
	return context->get_instance();
}

VkPhysicalDevice WSI::get_gpu() const
{
	return context->get_gpu();
}

VkDevice WSI::get_device() const
{
	return context->get_device();
}

VkSurfaceFormatKHR WSI::get_surface_format() const
{
	return surface_format;
}

uint32_t WSI::get_swapchain_image_count() const
{
	return swapchain_image_count;
}

VkImageView WSI::get_swapchain_image_view(uint32_t index) const
{
	assert(index >= 0 && index < swapchain_image_count);
	return swapchain_image_views[index];
}

VkSurfaceKHR WSI::create_surface(VkInstance instance)
{
	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
	assert(vkCreateWin32SurfaceKHR != nullptr);

	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfaceCreateInfo.hwnd = glfwGetWin32Window(window);
	surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

	VkSurfaceKHR surface;
	VkResult result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
	assert(result == VK_SUCCESS);

	return surface;
}

VkSwapchainKHR WSI::create_swapchain(VkSwapchainKHR old_swapchain)
{
	assert(surface);

	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->get_gpu(), surface, &surface_capabilities);

	surface_format = find_best_surface_format({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR });
	present_mode = find_best_present_mode(VK_PRESENT_MODE_MAILBOX_KHR);
	surface_extent = choose_swapchain_extent(surface_capabilities);

	uint32_t image_count = surface_capabilities.minImageCount + 1;
	// Make sure the surface supports it
	if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
		image_count = surface_capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	create_info.surface = surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = surface_extent;
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
	if (context->get_graphics_family_index() != context->get_present_family_index())
	{
		uint32_t queue_family_indices[] = { context->get_graphics_family_index(), context->get_present_family_index() };
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
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = old_swapchain;

	VkSwapchainKHR swapchain;
	VkResult result = vkCreateSwapchainKHR(context->get_device(), &create_info, nullptr, &swapchain);
	assert(result == VK_SUCCESS);

	// Retrieve the Swapchain VkImage handles. These images are create by the implementation of the swapchain
	// and will be automatically cleaned up
	swapchain_image_count = image_count;
	vkGetSwapchainImagesKHR(context->get_device(), swapchain, &swapchain_image_count, nullptr);
	assert(swapchain_image_count == image_count);
	swapchain_images = new VkImage[swapchain_image_count];
	vkGetSwapchainImagesKHR(context->get_device(), swapchain, &swapchain_image_count, swapchain_images);
	// Create a VkImageView per VkImage
	swapchain_image_views = new VkImageView[swapchain_image_count];
	for (uint32_t i = 0; i < swapchain_image_count; ++i)
	{
		VkImageViewCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		create_info.image = swapchain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = surface_format.format;
		// The components are used to swizzle the color channels around for different effects.
		create_info.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		// The subresourceRange describes the image's purporse and which part of the image should be accessed
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(context->get_device(), &create_info, nullptr, &swapchain_image_views[i]);
		assert (result == VK_SUCCESS);
	}

	return swapchain;
}

void WSI::destroy_swapchain()
{
	if (swapchain_images != nullptr)
	{
		delete[] swapchain_images;
	}

	if (swapchain_image_views != nullptr)
	{
		for (uint32_t i = 0; i < swapchain_image_count; ++i)
		{
			vkDestroyImageView(context->get_device(), swapchain_image_views[i], nullptr);
		}

		delete[] swapchain_image_views;
	}

	if (swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(context->get_device(), swapchain, nullptr);
	}
}

VkSurfaceFormatKHR WSI::find_best_surface_format(VkSurfaceFormatKHR preferred_format)
{
	uint32_t available_format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(context->get_gpu(), surface, &available_format_count, nullptr);
	VkSurfaceFormatKHR* available_formats = new VkSurfaceFormatKHR[available_format_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(context->get_gpu(), surface, &available_format_count, available_formats);

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

VkPresentModeKHR WSI::find_best_present_mode(VkPresentModeKHR preferred_present_mode)
{
	uint32_t available_present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(context->get_gpu(), surface, &available_present_mode_count, nullptr);
	VkPresentModeKHR* available_present_modes = new VkPresentModeKHR[available_present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(context->get_gpu() , surface, &available_present_mode_count, available_present_modes);

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

VkExtent2D WSI::choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& surface_capabilities)
{
	assert(surface);

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

} // namespace Vulkan