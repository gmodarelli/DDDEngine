#include <assert.h>
#include "volk.h"
#include <vector>
#include <iostream>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// Macro to check Vulkan function results
#define VK_CHECK(call) \
	do { \
		VkResult result = call; \
		assert(result == VK_SUCCESS); \
	} while (0);

void InitializeVulkan() {
	// Loading the Vulkan Library via volk
	VK_CHECK(volkInitialize());
}

VkApplicationInfo createApplicationInfo(const char* applicationName, uint32_t appVersionMajor, uint32_t appVersionMinor, uint32_t appVersionPatch) {
	// Querying for the supported Vulkan API Version
	uint32_t supportedApiVersion;
	VK_CHECK(vkEnumerateInstanceVersion(&supportedApiVersion));

#ifdef _DEBUG
	std::cout << "Supported Vulkan API" << std::endl;
	std::cout << VK_VERSION_MAJOR(supportedApiVersion) << "." << VK_VERSION_MINOR(supportedApiVersion) << "." << VK_VERSION_PATCH(supportedApiVersion) << std::endl;
	std::cout << std::endl;
#endif

	// Creating a struct with info about our Vulkan Application
	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = supportedApiVersion;
	appInfo.applicationVersion = VK_MAKE_VERSION(appVersionMajor, appVersionMinor, appVersionPatch);
	appInfo.pApplicationName = applicationName;

	return appInfo;
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

#ifdef _DEBUG
	std::cout << layerName << " is not supported" << std::endl;
	std::cout << std::endl;
#endif

	return false;
}

// TODO: Pass the desired extensions and layers into this function
VkInstance createInstance(VkApplicationInfo appInfo) {
	// Querying for the supported Instance Extensions
	uint32_t extensionsCount;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr));
	std::vector<VkExtensionProperties> extensions(extensionsCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensions.data()));

#ifdef _DEBUG
	std::cout << "Available instance extensions" << std::endl;
	for (const auto extension : extensions)
	{
		std::cout << extension.extensionName << std::endl;
	}
	std::cout << std::endl;
#endif

	// Querying for the list of supported layers
	uint32_t layersCount;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layersCount, nullptr));
	std::vector<VkLayerProperties> layers(layersCount);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layersCount, layers.data()));

#ifdef _DEBUG
	std::cout << "Available layer extensions" << std::endl;
	for (const auto layer : layers)
	{
		std::cout << layer.layerName << ": " << layer.description << std::endl;
	}
	std::cout << std::endl;
#endif

	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

#ifdef _DEBUG
	// Enable validation layers when in DEBUG mode
	const char* debugLayers[] =
	{
		"VK_LAYER_LUNARG_standard_validation"
	};

	assert(IsLayerSupported(layers, debugLayers[0]));

	createInfo.ppEnabledLayerNames = debugLayers;
	createInfo.enabledLayerCount = sizeof(debugLayers) / sizeof(debugLayers[0]);
#endif

	VkInstance instance = 0;
	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
	assert(instance);

	// Load all required Vulkan entrypoints, including all extensions
	volkLoadInstance(instance);

	return instance;
}

int main()
{
	InitializeVulkan();
	VkApplicationInfo appInfo = createApplicationInfo("Vulkan Rendere", 0, 0, 1);
	VkInstance instance = createInstance(appInfo);

	assert(glfwInit());

	GLFWwindow* window = glfwCreateWindow(1024, 768, "Renderer", 0, 0);
	assert(window);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	vkDestroyInstance(instance, nullptr);
}