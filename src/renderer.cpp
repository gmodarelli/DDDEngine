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

bool IsExtensionSupported(const std::vector<VkExtensionProperties> extensions, const char* extensionName)
{
	for (const auto extension : extensions)
	{
		if (strstr(extension.extensionName, extensionName))
		{
			return true;
		}
	}

#ifdef _DEBUG
	std::cout << extensionName << " is not supported" << std::endl;
	std::cout << std::endl;
#endif

	return false;
}

VkInstance createInstance(VkApplicationInfo appInfo, std::vector<const char*> desiredExtensions) {
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

	for(auto const extension : desiredExtensions) {
		assert(IsExtensionSupported(extensions, extension));
	}

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
	createInfo.enabledExtensionCount = static_cast<uint32_t>(desiredExtensions.size());
	createInfo.ppEnabledExtensionNames = desiredExtensions.size() > 0 ? desiredExtensions.data() : nullptr;

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

std::vector<VkExtensionProperties> getPhysicalDeviceExtensions(VkPhysicalDevice physicalDevice) {
	uint32_t extensionsCount;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, nullptr));
	std::vector<VkExtensionProperties> extensions(extensionsCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, extensions.data()));

#ifdef _DEBUG
	for (const auto extension : extensions) {
		std::cout << "\t" << extension.extensionName << std::endl;
	}
	std::cout << std::endl;
#endif

	return extensions;
}

struct DiscreteGPU {
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	std::vector<VkExtensionProperties> extensions;
};

DiscreteGPU findDiscreteGPU(VkInstance instance, std::vector<const char*> desiredExtensions) {
	uint32_t physicalDevicesCount;
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr));
	assert(physicalDevicesCount);
	std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, physicalDevices.data()));

	DiscreteGPU gpu = {};

#ifdef _DEBUG
	std::cout << "Found " << physicalDevicesCount << " GPUs: " << std::endl;
#endif

	for (const auto physicalDevice : physicalDevices) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		#ifdef _DEBUG
			std::cout << properties.deviceName << std::endl;
		#endif

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures(physicalDevice, &features);
			std::vector<VkExtensionProperties> extensions = getPhysicalDeviceExtensions(physicalDevice);

			float supported = true;

			for (const auto desiredExtension : desiredExtensions) {
				if (!IsExtensionSupported(extensions, desiredExtension)) {
					supported = false;
				}
			}

			if (supported) {
				gpu.physicalDevice = physicalDevice;
				gpu.properties = properties;
				gpu.features = features;
				gpu.extensions = extensions;

				return gpu;
			}
		}
	}

	return gpu;
}

struct QueueInfo {
	uint32_t familyIndex;
	uint32_t queueCount;
	std::vector<float> priorities;

	VkQueueFlags flags;
};

std::vector<QueueInfo> findQueueFamilies(DiscreteGPU gpu, VkQueueFlags desiredCapabilities) {
	uint32_t queueFamiliesCount;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu.physicalDevice, &queueFamiliesCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamiliesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu.physicalDevice, &queueFamiliesCount, queueFamilies.data());

#ifdef _DEBUG
	std::cout << gpu.properties.deviceName << " has " << queueFamiliesCount << " queue families" << std::endl;
	std::cout << std::endl;
#endif

	std::vector<QueueInfo> queueInfos;

	for (uint32_t index = 0; index < queueFamiliesCount; ++index)
	{
		if (queueFamilies[index].queueCount > 0 && queueFamilies[index].queueFlags & desiredCapabilities) {
			std::vector<float> priorities(queueFamilies[index].queueCount);
			// TODO: Document why properties are assigned this way
			for (uint32_t i = 0; i < queueFamilies[index].queueCount; ++i) {
				priorities.push_back((float)i / (float)queueFamilies[index].queueCount);
			}
			queueInfos.push_back({ index, queueFamilies[index].queueCount, priorities, queueFamilies[index].queueFlags });
		}
	}

	assert(queueInfos.size() > 0);
	return queueInfos;
}

int main()
{
	InitializeVulkan();
	VkApplicationInfo appInfo = createApplicationInfo("Vulkan Rendere", 0, 0, 1);

	std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
	VkInstance instance = createInstance(appInfo, instanceExtensions);

	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	DiscreteGPU gpu = findDiscreteGPU(instance, deviceExtensions);
	assert(gpu.physicalDevice);

	std::vector<QueueInfo> queueInfos = findQueueFamilies(gpu, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	for (auto &info : queueInfos) {
		VkDeviceQueueCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		createInfo.queueFamilyIndex = info.familyIndex;
		createInfo.queueCount = info.queueCount;
		createInfo.pQueuePriorities = info.priorities.data();
		queueCreateInfos.push_back(createInfo);
	}

	VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	VkDevice device = 0;
	VK_CHECK(vkCreateDevice(gpu.physicalDevice, &createInfo, nullptr, &device));
	assert(device);

	assert(glfwInit());

	GLFWwindow* window = glfwCreateWindow(1024, 768, "Renderer", 0, 0);
	assert(window);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
}