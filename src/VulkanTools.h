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

VkBool32 IsExtensionSupported(const char* extensionName, const std::vector<VkExtensionProperties>* availableExtensions)
{
	for (const auto extension : *availableExtensions)
	{
		if (strstr(extension.extensionName, extensionName)) return VK_TRUE;
	}

	return VK_FALSE;
}

VkBool32 CheckExtensionsSupport(const std::vector<const char*>* extensionNames, const std::vector<VkExtensionProperties>* availableExtensions)
{
	for (const auto extensionName : *extensionNames)
	{
		if (IsExtensionSupported(extensionName, availableExtensions) != VK_TRUE)
		{
			printf("The required %s extension is not supported.\n", extensionName);
			return VK_FALSE;
		}
	}

	return VK_TRUE;
}
#include "VulkanTypes.h"
#include "VulkanDebug.h"
// #include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanCommandBuffer.h"
#include "VulkanVertexBuffer.h"
#include "VulkanShaderAndUniforms.h"
#include "VulkanDescriptors.h"
#include "VulkanPipeline.h"
#include "VulkanRenderLoop.h"
#include "VulkanSynchronization.h"

#endif // VULKAN_TOOLS_H_
