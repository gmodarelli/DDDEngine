#pragma once

#include "volk.h"
#include "Types.h"
#include "VulkanDebug.h"
#include <vector>

namespace ddd
{
	bool IsExtensionSupported(const std::vector<VkExtensionProperties> extensions, const char* extensionName);
	bool IsLayerSupported(const std::vector<VkLayerProperties> layers, const char* layerName);

	bool createInstance(VulkanInstance* vulkanInstance, std::vector<const char*> extensions)
	{
		// Querying for the supported Vulkan API Version
		uint32_t supportedApiVersion;
		VK_CHECK(vkEnumerateInstanceVersion(&supportedApiVersion));

		// Creating a struct with info about our Vulkan Application
		VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		appInfo.apiVersion = supportedApiVersion;
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.pApplicationName = "ddd";

		// Querying for the supported Instance Extensions
		uint32_t extensionCount;
		VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
		std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
		VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data()));

#ifdef _DEBUG
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		for(auto const extension : extensions) {
			if (!IsExtensionSupported(supportedExtensions, extension)) return false;
		}

#ifdef _DEBUG
		// Querying for the list of supported layers
		uint32_t layerCount;
		VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
		std::vector<VkLayerProperties> supportedLayers(layerCount);
		VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, supportedLayers.data()));

		std::vector<const char*> layers = { "VK_LAYER_LUNARG_standard_validation" };
		for(auto const layer : layers) {
			if (!IsLayerSupported(supportedLayers, layer)) return false;
		}
#endif

		vulkanInstance->enabledExtensions = extensions;
#ifdef _DEBUG
		vulkanInstance->enabledLayers = layers;
#endif

		VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.size() > 0 ? extensions.data() : nullptr;

#ifdef _DEBUG
		createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
		createInfo.ppEnabledLayerNames = layers.size() > 0 ? layers.data() : nullptr;
#endif

		VK_CHECK(vkCreateInstance(&createInfo, nullptr, &vulkanInstance->instance));
		DDD_ASSERT(vulkanInstance->instance);
		volkLoadInstance(vulkanInstance->instance);

#ifdef _DEBUG
		vulkanInstance->debugCallback = ddd::setupDebugCallback(vulkanInstance->instance, nullptr);
		DDD_ASSERT(vulkanInstance->debugCallback);
#endif

		return true;
	}

	void destroyVulkanInstance(VulkanInstance* vulkanInstance)
	{
		DDD_ASSERT(vulkanInstance->instance != VK_NULL_HANDLE);
#ifdef _DEBUG
		ddd::destroyDebugCallback(vulkanInstance->instance, vulkanInstance->debugCallback, nullptr);
#endif

		vkDestroyInstance(vulkanInstance->instance, nullptr);
		vulkanInstance->instance = VK_NULL_HANDLE;
	}

	bool IsExtensionSupported(const std::vector<VkExtensionProperties> extensions, const char* extensionName)
	{
		for (const auto extension : extensions)
		{
			if (strstr(extension.extensionName, extensionName))
			{
				return true;
			}
		}

		return false;
	}

	bool IsLayerSupported(const std::vector<VkLayerProperties> layers, const char* layerName)
	{
		for (const auto layer : layers)
		{
			if (strstr(layer.layerName, layerName))
			{
				return true;
			}
		}

		return false;
	}
}