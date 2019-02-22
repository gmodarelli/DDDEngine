#pragma once

#include "volk.h"
#include "context.h"
#include "../application/platform.h"

namespace Vulkan
{

struct WSI
{
	WSI(Platform* platform);
	bool init();
	void cleanup();

	bool resizing();
	void recreate_swapchain();

	// The following function and member are used to signal
	// that the windows has been resized
	bool window_resized();
	bool framebuffer_resized = false;

	Vulkan::Context* context;

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;

	uint32_t swapchain_image_count = 0;
	VkImage* swapchain_images = nullptr;
	VkImageView* swapchain_image_views = nullptr;

	VkExtent2D swapchain_extent = {};
	VkSurfaceFormatKHR surface_format = { VK_FORMAT_UNDEFINED };
	// VK_PRESENT_MODE_FIFO_KHR is the only mode guaranteed to be available, 
	// so we set it as default present mode.
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

	Platform* platform;
	bool is_resizing = false;

private:

	// Surface
	VkSurfaceKHR create_surface(VkInstance instance);

	// Swapchain
	VkSwapchainKHR create_swapchain(VkSwapchainKHR old_swapchain = VK_NULL_HANDLE);
	void destroy_swapchain();

	VkSurfaceFormatKHR find_best_surface_format(VkSurfaceFormatKHR preferred_format);
	VkPresentModeKHR find_best_present_mode(VkPresentModeKHR preferred_present_mode);
	VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& surface_capabilities);
	VkSurfaceCapabilitiesKHR surface_capabilities = {};

}; // struct WSI

} // namespace Vulkan