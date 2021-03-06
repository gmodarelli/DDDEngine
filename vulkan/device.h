#pragma once

#include "volk.h"
#include "wsi.h"
#include "context.h"
#include "buffer.h"
#include "image.h"

namespace Vulkan
{

// TODO: Measure how many in-flight frames our good
// for our application.
const int MAX_FRAMES_IN_FLIGHT = 3;

// A FrameResources struct manages the lifetime of
// a single frame of animation.
struct FrameResources
{
	// Command buffer handle used to record operations
	// of a single, indipendent frame of animation.
	// NOTE: In real-life application there will be
	// multiple command buffers, recorded in multiple
	// threads.
	VkCommandBuffer command_buffer = VK_NULL_HANDLE;

	// Semaphore passed to the presentation engine
	// when we acquire an image from the swapchain.
	// This semaphore must then be provided as one
	// of the wait semaphores when we submit the
	// command buffer to a queue.
	VkSemaphore	image_acquired_semaphore = VK_NULL_HANDLE;

	// Semaphore that gets signaled when a queue
	// stops processing our command buffer. We use
	// it during the image presentation, so the 
	// presentation engine knows when the image is
	// ready.
	VkSemaphore	ready_to_present_semaphore = VK_NULL_HANDLE;

	// We provide this fence during the command buffer
	// submission. It gets signaled when the command
	// buffer is no longer executed on a queue.
	// This is necessary to synchronize operations on
	// the CPU side (the operations our application
	// performs), not the GPU (and the presentation
	// engine). When this fence is signaled we know
	// that we can both re-record the command buffer
	// and destroy a framebuffer.
	VkFence	drawing_finished_fence = VK_NULL_HANDLE;

	// Image view for an image serving as depth
	// attachment inside a sub-pass.
	VkImageView	depth_attachment = VK_NULL_HANDLE;

	// Temporary framebuffer handle created for the
	// lifetime of a single frame of animation.
	VkFramebuffer framebuffer = VK_NULL_HANDLE;

	// The index of the Swapchain image we have
	// aqcuired from the swapchain at the beginning
	// of the current frame
	uint32_t image_index;

	// A Query Pool to measure the time it takes
	// to draw a single frame on the GPU
	uint32_t timestamp_query_pool_count = 0;
	VkQueryPool timestamp_query_pool = VK_NULL_HANDLE;

	// Renderer-specific data
	void* custom = nullptr;
};

struct Device
{
	Device(WSI* wsi, Context* context);

	void init();
	void cleanup();

	FrameResources& begin_draw_frame();
	void end_draw_frame(FrameResources& frame_resources);

	WSI* wsi;
	Context* context;

	// Render Pass
	VkRenderPass render_pass = VK_NULL_HANDLE;

	// Color Buffer
	Vulkan::Image* color_buffer;
	// Depth Buffer
	Vulkan::Image* depth_buffer;

	// Command Pools
	VkCommandPool command_pool = VK_NULL_HANDLE;
	VkCommandPool transfer_command_pool = VK_NULL_HANDLE;

	uint32_t frame_index = 0;
	FrameResources* frame_resources;

	// Query pool
	VkQueryPool timestamp_query_pool;

	double frame_gpu_avg = 0.0f;
	double frame_gpu_min = 100.0f;
	double frame_gpu_max = -1.0f;

	// TODO: Merge these 3 buffers into one
	Vulkan::Buffer* vertex_buffer;
	Vulkan::Buffer* index_buffer;
	Vulkan::Buffer* uniform_buffer;
	VkDeviceSize vertex_head_cursor = 0;
	VkDeviceSize index_head_cursor = 0;
	VkDeviceSize uniform_head_cursor = 0;

	VkDeviceSize upload_vertex_buffer(Vulkan::Buffer* staging_buffer);
	VkDeviceSize upload_index_buffer(Vulkan::Buffer* staging_buffer);
	VkDeviceSize upload_uniform_buffer(Vulkan::Buffer* staging_buffer);

	void upload_buffer_to_image(VkBuffer buffer, VkImage image, VkFormat format, uint32_t width, uint32_t height, VkImageLayout old_layout, VkImageLayout new_old_layout, VkImageLayout new_layout);
	void transition_image_layout(VkImage image, VkFormat format, VkImageLayout src_layout, VkImageLayout dst_layout);

	VkCommandBuffer create_graphics_command_buffer(VkCommandBufferLevel level, bool begin = true);
	void flush_graphics_command_buffer(VkCommandBuffer& command_buffer, bool free = true);

	VkCommandBuffer create_transfer_command_buffer(VkCommandBufferLevel level, bool begin = true);
	void flush_transfer_command_buffer(VkCommandBuffer& command_buffer, bool free = true);

private:

	// Vertex, Index, Uniform buffers helpers
	void create_gpu_buffers();
	void free_gpu_buffers();

	// Render pass helpers
	void create_render_pass();
	void destroy_render_pass();

	// Color buffer helpers
	void create_color_buffer();
	void destroy_color_buffer();

	// Depth buffer helpers
	void create_depth_buffer();
	void destroy_depth_buffer();
	VkFormat find_supported_format(VkImageTiling tiling, VkFormatFeatureFlags features);

	// Framebuffer helpers
	void destroy_framebuffers();

	// Command Pool and command buffer helpers
	void create_command_pools();
	void destroy_command_pools();
	void allocate_command_buffers();
	void free_command_buffers();

	// Synchronization Objects helpers
	void create_sync_objects();
	void destroy_sync_objects();

	// Query pool helpers
	void create_query_pool();
	void destroy_query_pool();

}; // struct Device

} // namespace Vulkan