#pragma once

#include "volk.h"
#include "context.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Vulkan
{

static void framebuffer_resize_callback(GLFWwindow* window, int window_width, int window_height);

struct WSI
{
	WSI(int window_width, int window_height);
	bool init();

	bool alive();
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

	int window_width;
	int window_height;
	GLFWwindow* window = NULL;
	bool is_resizing = false;

	void set_window_title(const char* title);

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