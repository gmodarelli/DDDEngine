#include <assert.h>
#include "volk.h"
#include <vector>
#include <iostream>

namespace Renderer {
	namespace Vulkan {

		// Macro to check Vulkan function results
		#define VK_CHECK(call) \
			do { \
				VkResult result = call; \
				assert(result == VK_SUCCESS); \
			} while (0);

		void Initialize() {
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

#ifdef _DEBUG

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
			std::cerr << "[validation layer]: " << pCallbackData->pMessage << std::endl;
			return VK_FALSE;
		}

		void destroyDebugCallback(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
			vkDestroyDebugUtilsMessengerEXT(instance, callback, pAllocator);
		}

		VkDebugUtilsMessengerEXT setupDebugCallback(VkInstance instance, const VkAllocationCallbacks* pAllocator) {
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = Renderer::Vulkan::debugCallback;
			createInfo.pUserData = nullptr;

			VkDebugUtilsMessengerEXT callback = VK_NULL_HANDLE;
			VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &createInfo, pAllocator, &callback));
			return callback;
		}
#endif

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

		struct Queue {
			VkQueue queue;
			uint32_t familyIndex;
			uint32_t queueIndex;
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
					float featuresSupported = true;
					if (features.geometryShader != VK_TRUE) {
						featuresSupported = false;
					}
					else {
						// NOTE: We're only interested in the geometry shader feature, so
						// we're disabling all the other ones. features will be used when
						// creating a logical device aftwards
						features = {};
						features.geometryShader = VK_TRUE;
					}

					std::vector<VkExtensionProperties> extensions = getPhysicalDeviceExtensions(physicalDevice);

					float extensionsSupported = true;

					for (const auto desiredExtension : desiredExtensions) {
						if (!IsExtensionSupported(extensions, desiredExtension)) {
							extensionsSupported = false;
						}
					}

					if (featuresSupported && extensionsSupported) {
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

		QueueInfo findQueueFamily(DiscreteGPU gpu, uint32_t queueCount, VkQueueFlags desiredCapabilities) {
			assert(queueCount > 0);
			uint32_t queueFamiliesCount;
			vkGetPhysicalDeviceQueueFamilyProperties(gpu.physicalDevice, &queueFamiliesCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamiliesCount);
			vkGetPhysicalDeviceQueueFamilyProperties(gpu.physicalDevice, &queueFamiliesCount, queueFamilies.data());

		#ifdef _DEBUG
			std::cout << gpu.properties.deviceName << " has " << queueFamiliesCount << " queue families" << std::endl;
			std::cout << std::endl;
		#endif

			QueueInfo queueInfo;

			for (uint32_t index = 0; index < queueFamiliesCount; ++index)
			{
				if (queueFamilies[index].queueCount >= queueCount && queueFamilies[index].queueFlags & desiredCapabilities) {
					// NOTE: I'm not sure I need to take all queues
					/*
					std::vector<float> priorities(queueFamilies[index].queueCount);
					// TODO: Document why properties are assigned this way
					for (uint32_t i = 0; i < queueFamilies[index].queueCount; ++i) {
						priorities.push_back((float)i / (float)queueFamilies[index].queueCount);
					}
					queueInfos.push_back({ index, queueFamilies[index].queueCount, priorities, queueFamilies[index].queueFlags });
					*/
					// NOTE: I'm only taking queueCount queues for a family
					std::vector<float> priorities(queueCount);
					for (uint32_t i = 0; i < queueCount; ++i) {
						priorities.push_back((float)i / (float)queueCount);
					}

					queueInfo = { index, queueCount, priorities, queueFamilies[index].queueFlags };
					break;
				}
			}

			assert(queueInfo.familyIndex >= 0);
			return queueInfo;
		}

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
					// NOTE: I'm not sure I need to take all queues
					/*
					std::vector<float> priorities(queueFamilies[index].queueCount);
					// TODO: Document why properties are assigned this way
					for (uint32_t i = 0; i < queueFamilies[index].queueCount; ++i) {
						priorities.push_back((float)i / (float)queueFamilies[index].queueCount);
					}
					queueInfos.push_back({ index, queueFamilies[index].queueCount, priorities, queueFamilies[index].queueFlags });
					*/
					// NOTE: I'm only taking one queue for a family
					queueInfos.push_back({ index, 1, { 1.0f }, queueFamilies[index].queueFlags });
				}
			}

			assert(queueInfos.size() > 0);
			return queueInfos;
		}

		VkDevice createDevice(const DiscreteGPU &gpu, std::vector<const char*> deviceExtensions, Queue &graphicsQueue, Queue &computeQueue) {
			QueueInfo queueInfo = findQueueFamily(gpu, 2, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

		#ifdef _DEBUG
			std::cout << "Graphics and Compute queue index: " << queueInfo.familyIndex << std::endl;
		#endif

			VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			queueCreateInfo.queueFamilyIndex = queueInfo.familyIndex;
			queueCreateInfo.queueCount = queueInfo.queueCount;
			queueCreateInfo.pQueuePriorities = queueInfo.priorities.data();

			/*
			* NOTE: At this stage we don't need to find and request all queues
			std::vector<QueueInfo> queueInfos = findQueueFamilies(gpu, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);
			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			for (auto &info : queueInfos) {
				VkDeviceQueueCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
				createInfo.queueFamilyIndex = info.familyIndex;
				createInfo.queueCount = info.queueCount;
				createInfo.pQueuePriorities = info.priorities.data();
				queueCreateInfos.push_back(createInfo);
			}
			*/

			VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
			// createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			createInfo.queueCreateInfoCount = 1;
			// createInfo.pQueueCreateInfos = queueCreateInfos.data();
			createInfo.pQueueCreateInfos = &queueCreateInfo;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();
			createInfo.pEnabledFeatures = &gpu.features;
			VkDevice device = 0;
			VK_CHECK(vkCreateDevice(gpu.physicalDevice, &createInfo, nullptr, &device));
			assert(device);

			volkLoadDevice(device);

			VkQueue graphics = VK_NULL_HANDLE;
			vkGetDeviceQueue(device, queueInfo.familyIndex, 0, &graphics);
			assert(graphics != VK_NULL_HANDLE);
			graphicsQueue = { graphics, queueInfo.familyIndex, 0 };

			VkQueue compute = VK_NULL_HANDLE;
			vkGetDeviceQueue(device, queueInfo.familyIndex, 1, &compute);
			assert(compute != VK_NULL_HANDLE);
			computeQueue = { compute, queueInfo.familyIndex, 1 };
			
			return device;
		}

		VkSurfaceKHR createSurface(VkInstance instance, const DiscreteGPU &gpu, const Queue &queue, const HINSTANCE hinstance, const HWND windowHandle, VkPresentModeKHR *desiredPresentMode) {
			VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
			createInfo.hinstance = hinstance;
			createInfo.hwnd = windowHandle;
			VkSurfaceKHR surface = VK_NULL_HANDLE;
			VK_CHECK(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface));
			assert(surface != VK_NULL_HANDLE);

			VkBool32 supported = false;
			VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(gpu.physicalDevice, queue.familyIndex, surface, &supported));
			assert(supported == VK_TRUE);

			uint32_t presentModeCount = 0;
			VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.physicalDevice, surface, &presentModeCount, nullptr));
			std::vector<VkPresentModeKHR> presentModes(presentModeCount);
			VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.physicalDevice, surface, &presentModeCount, presentModes.data()));
			assert(presentModeCount > 0);

			float desiredModeSupported = false;
			for (const auto presentMode : presentModes) {
				if (presentMode == *desiredPresentMode) {

				#ifdef _DEBUG
					std::cout << "The selected GPU supports the desired present mode" << std::endl;
					std::cout << std::endl;
				#endif
					desiredModeSupported = true;
					break;
				}
			}

			if (!desiredModeSupported) {
			#ifdef _DEBUG
				*desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
				std::cout << "The selected GPU does not support the desired present mode. Falling back to FIFO." << std::endl;
				std::cout << std::endl;
			#endif
			}

			return surface;
		}

		VkSurfaceFormatKHR chooseSwapchainFormat(const DiscreteGPU &gpu, VkSurfaceKHR presentationSurface, VkSurfaceFormatKHR desiredFormat) {
			uint32_t formatsCount;
			VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.physicalDevice, presentationSurface, &formatsCount, nullptr));
			std::vector<VkSurfaceFormatKHR> surfaceFormats(formatsCount);
			VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.physicalDevice, presentationSurface, &formatsCount, surfaceFormats.data()));

			if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
		#ifdef _DEBUG
				std::cout << "No restrictions for image format and colorspace. Assigning the desired ones." << std::endl;
				std::cout << std::endl;
		#endif
				return desiredFormat;
			}

			for (const auto surfaceFormat : surfaceFormats) {
				if (surfaceFormat.format == desiredFormat.format && surfaceFormat.colorSpace == desiredFormat.colorSpace) {
				#ifdef _DEBUG
					std::cout << "Found a matching format and colorspace pair. Assigning the desired ones." << std::endl;
					std::cout << std::endl;
				#endif
					return desiredFormat;
				}
			}

			for (const auto surfaceFormat : surfaceFormats) {
				if (surfaceFormat.format == desiredFormat.format) {
				#ifdef _DEBUG
					std::cout << "Found a matching format. Assigning the desired format and matching color space." << std::endl;
					std::cout << std::endl;
				#endif
					return { desiredFormat.format, surfaceFormat.colorSpace };
				}
			}

		#ifdef _DEBUG
			std::cout << "A matching format was not found. Assigning the first format and color space pair." << std::endl;
			std::cout << std::endl;
		#endif
			return surfaceFormats[0];
		}

		uint32_t chooseNumberOfImages(VkSurfaceCapabilitiesKHR surfaceCapabilities) {
			uint32_t numberOfImages = surfaceCapabilities.minImageCount + 1;
			if (surfaceCapabilities.maxImageCount > 0 && numberOfImages > surfaceCapabilities.maxImageCount) {
				numberOfImages = surfaceCapabilities.maxImageCount;
			}

			return numberOfImages;
		}

		VkExtent2D chooseExtent2D(VkSurfaceCapabilitiesKHR surfaceCapabilities, uint32_t windowWidth, uint32_t windowHeight) {
			if (surfaceCapabilities.currentExtent.width != 0xFFFFFFFF) {
				return surfaceCapabilities.currentExtent;
			}
			else {
				VkExtent2D extent;
				extent.width = windowWidth >= surfaceCapabilities.minImageExtent.width && windowWidth <= surfaceCapabilities.maxImageExtent.width ? windowWidth : surfaceCapabilities.maxImageExtent.width;
				extent.height = windowHeight >= surfaceCapabilities.minImageExtent.height && windowHeight <= surfaceCapabilities.maxImageExtent.height ? windowHeight : surfaceCapabilities.maxImageExtent.height;

				return extent;
			}
		}

		VkSwapchainKHR createSwapchain(VkDevice device, DiscreteGPU &gpu, VkSurfaceKHR presentationSurface, VkPresentModeKHR presentMode, uint32_t windowWidth, uint32_t windowHeight) {
			VkSurfaceCapabilitiesKHR surfaceCapabilities;
			VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu.physicalDevice, presentationSurface, &surfaceCapabilities));

			uint32_t numberOfImages = chooseNumberOfImages(surfaceCapabilities);
			VkExtent2D imagesSize = chooseExtent2D(surfaceCapabilities, windowWidth, windowHeight);

			VkImageUsageFlags desiredUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			bool supported = desiredUsage == (desiredUsage & surfaceCapabilities.supportedUsageFlags);
			assert(supported == true);

			VkSurfaceTransformFlagBitsKHR surfaceTransform;
			VkSurfaceTransformFlagBitsKHR desiredTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			if (surfaceCapabilities.supportedTransforms & desiredTransform) {
				surfaceTransform = desiredTransform;
			}
			else {
				surfaceTransform = surfaceCapabilities.currentTransform;
			}

			VkSurfaceFormatKHR surfaceFormat = chooseSwapchainFormat(gpu, presentationSurface, { VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR });

			VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
			createInfo.surface = presentationSurface;
			createInfo.minImageCount = numberOfImages;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = imagesSize;
			createInfo.imageArrayLayers = 1;
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.imageUsage = desiredUsage;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
			createInfo.preTransform = surfaceTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = VK_NULL_HANDLE;

			VkSwapchainKHR swapchain;
			VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
			assert(result == VK_SUCCESS);

			return swapchain;
		}

		void destroySwapchain(VkDevice device, VkSwapchainKHR &swapchain) {
			assert(device != VK_NULL_HANDLE);
			assert(swapchain != VK_NULL_HANDLE);
			vkDestroySwapchainKHR(device, swapchain, nullptr);
			swapchain = VK_NULL_HANDLE;
		}

		void destroyDevice(VkDevice &device) {
			assert(device != VK_NULL_HANDLE);
			vkDestroyDevice(device, nullptr);
			device = VK_NULL_HANDLE;
		}

		void destroyInstance(VkInstance &instance) {
			assert(instance != VK_NULL_HANDLE);
			vkDestroyInstance(instance, nullptr);
			instance = VK_NULL_HANDLE;
		}
	}
}