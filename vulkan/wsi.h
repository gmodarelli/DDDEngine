#pragma once

#include "volk.h"
#include "context.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Vulkan
{

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);

struct WSI
{
	WSI(int width, int height);
	bool init();

	int get_width();
	int get_height();

	bool alive();
	void cleanup();

	VkInstance get_instance() const;
	VkPhysicalDevice get_gpu() const;
	VkDevice get_device() const;

	uint32_t get_graphics_family_index() const;
	uint32_t get_transfer_family_index() const;
	VkQueue get_graphics_queue() const;
	VkQueue get_transfer_queue() const;

	VkSurfaceFormatKHR get_surface_format() const;
	VkSwapchainKHR get_swapchain() const;
	uint32_t get_swapchain_image_count() const;
	VkImageView get_swapchain_image_view(uint32_t index) const;
	VkExtent2D get_swapchain_extent() const;

	bool resizing();
	void recreate_swapchain();

	// The following function and member are used to signal
	// that the windows has been resized
	bool window_resized();
	bool framebuffer_resized = false;

	Vulkan::Context* context;

private:
	int width;
	int height;
	GLFWwindow* window = NULL;

	bool is_resizing = false;

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;

	// Surface
	VkSurfaceKHR create_surface(VkInstance instance);

	// Swapchain
	VkSwapchainKHR create_swapchain(VkSwapchainKHR old_swapchain = VK_NULL_HANDLE);
	void destroy_swapchain();

	VkSurfaceFormatKHR find_best_surface_format(VkSurfaceFormatKHR preferred_format);
	VkPresentModeKHR find_best_present_mode(VkPresentModeKHR preferred_present_mode);
	VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& surface_capabilities);
	uint32_t swapchain_image_count = 0;
	VkImage* swapchain_images = nullptr;
	VkImageView* swapchain_image_views = nullptr;
	VkSurfaceCapabilitiesKHR surface_capabilities = {};
	VkExtent2D swapchain_extent = {};
	VkSurfaceFormatKHR surface_format = { VK_FORMAT_UNDEFINED };
	// VK_PRESENT_MODE_FIFO_KHR is the only mode guaranteed to be available, 
	// so we set it as default present mode.
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
}; // struct WSI

} // namespace Vulkan