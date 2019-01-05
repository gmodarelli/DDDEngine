#ifndef VULKAN_DEBUG_H_
#define VULKAN_DEBUG_H_

#ifdef _DEBUG

#include <assert.h>

#define VK_CHECK(call) \
	do { \
		VkResult result = call; \
		assert(result == VK_SUCCESS); \
	} while (0);

#define R_ASSERT(x) assert(x)

void LogPhysicalDeviceProperties(const std::vector<VkPhysicalDeviceProperties>* physicalDevicesProperties)
{
	printf("\n\n======= Available Physical Devices =======\n\n");

	for (const auto deviceProperties : *physicalDevicesProperties)
	{
		printf("\tDevice Name: %s\n", deviceProperties.deviceName);
		printf("\tDevice Type: %d\n", deviceProperties.deviceType);
		printf("\tDriver Version: %d\n", deviceProperties.driverVersion);
		printf("\tAPI Version: %d.%d.%d\n\n", VK_VERSION_MAJOR(deviceProperties.apiVersion), VK_VERSION_MINOR(deviceProperties.apiVersion), VK_VERSION_PATCH(deviceProperties.apiVersion));
	}
}

void LogInstanceLayers(const std::vector<VkLayerProperties>* availableLayers)
{
	printf("\n\n======= Available Instance Layers =======\n\n");
	for (const auto layer : *availableLayers)
	{
		printf("\t%s\n", layer.layerName);
	}
}

void LogInstanceExtensions(const std::vector<VkExtensionProperties>* availableExtensions)
{
	printf("\n\n======= Available Instance Extensions =======\n\n");
	for (const auto extension : *availableExtensions)
	{
		printf("\t%s\n", extension.extensionName);
	}
}

void LogDeviceExtensions(const std::vector<VkExtensionProperties>* availableExtensions)
{
	printf("\n\n======= Available Device Extensions =======\n\n");
	for (const auto extension : *availableExtensions)
	{
		printf("\t%s\n", extension.extensionName);
	}
}

VkBool32 debugReportErrorCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	char message[4096];
	snprintf(message, ARRAYSIZE(message), "\n [ Vulkan Validation Error ]\n%s\n\n", pMessage);

	printf(message);

	// TODO: Instead of assert we should call a function to 
	// cleanup and exit
	R_ASSERT(!"Validation error encountered!");
	return VK_FALSE;
}

VkBool32 debugReportWarningCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	const char* type =
		(flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
		? "Warning"
		: "Info";

	char message[4096];

	snprintf(message, ARRAYSIZE(message), "\n [ Vulkan Validation %s ]\n%s\n\n", type, pMessage);
	printf(message);

	return VK_FALSE;
}

#else

#define VK_CHECK(call) \
	do { \
		VkResult result = call; \
	} while (0);

#define R_ASSERT(x)

void LogPhysicalDeviceProperties(const std::vector<VkPhysicalDeviceProperties>* physicalDevicesProperties) {}
void LogInstanceLayers(const std::vector<VkLayerProperties>* availableLayers) {}
void LogInstanceExtensions(const std::vector<VkExtensionProperties>* availableExtensions) {}
void LogDeviceExtensions(const std::vector<VkExtensionProperties>* availableExtensions) {}

VkBool32 debugReportErrorCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {}
VkBool32 debugReportWarningCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {}

#endif

#endif // VULKAN_DEBUG_H_
