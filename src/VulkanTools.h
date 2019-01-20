#ifndef VULKAN_TOOLS_H_
#define VULKAN_TOOLS_H_

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined __linux__
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined __APPLE__
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include "volk.h"
#include <vector>

#include "VulkanDebug.h"

#include "vkr/device.h" 
#include "vkr/swapchain.h"

#include "VulkanShaderAndUniforms.h"

#include "vkr/buffer.h"
#include "vkr/model.h"

#endif // VULKAN_TOOLS_H_
