#include "CommonVK.h"
#include "RenderXVK.h"
#include "Windows.h"
#include "vulkan/vulkan_win32.h"
#include <algorithm>

#ifndef NDEBUG
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

namespace RenderX {
namespace RenderXVK {

	static std::array<FrameContex, MAX_FRAMES_IN_FLIGHT> g_Frames;
	uint32_t g_CurrentFrame = 0;

	VulkanContext& GetVulkanContext() {
		static VulkanContext g_Context;
		return g_Context;
	}

	FrameContex& GetCurrentFrameContex() {
		return g_Frames[g_CurrentFrame];
	}

	constexpr std::array<VkDynamicState, 2> g_DynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	constexpr std::array<const char*, 1> g_RequestedValidationLayers{
		"VK_LAYER_KHRONOS_validation"
	};

	constexpr std::array<const char*, 1> g_RequestedDeviceExtensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	// -------------------- helpers to query layers/extensions --------------------

	const char* VkResultToString(VkResult result) {
		switch (result) {
		case VK_SUCCESS: return "VK_SUCCESS";
		case VK_NOT_READY: return "VK_NOT_READY";
		case VK_TIMEOUT: return "VK_TIMEOUT";
		case VK_EVENT_SET: return "VK_EVENT_SET";
		case VK_EVENT_RESET: return "VK_EVENT_RESET";
		case VK_INCOMPLETE: return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		default: return "VK_UNKNOWN_ERROR";
		}
	}

	std::vector<const char*> GetEnabledValidationLayers() {
		if constexpr (!enableValidationLayers) {
			RENDERX_INFO("Validation layers disabled (Release build)");
			return {};
		}

		RENDERX_INFO("Querying available validation layers...");
		uint32_t count = 0;
		VkResult result = vkEnumerateInstanceLayerProperties(&count, nullptr);
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to enumerate instance layer properties: {}", VkResultToString(result));
			return {};
		}

		if (count == 0) {
			RENDERX_WARN("No validation layers available");
			return {};
		}

		RENDERX_INFO("Found {} available validation layers", count);
		std::vector<VkLayerProperties> props(count);
		result = vkEnumerateInstanceLayerProperties(&count, props.data());
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to get instance layer properties: {}", VkResultToString(result));
			return {};
		}

		std::vector<const char*> enabledLayers;
		enabledLayers.reserve(g_RequestedValidationLayers.size());

		for (const char* requested : g_RequestedValidationLayers) {
			auto it = std::find_if(props.begin(), props.end(),
				[requested](const VkLayerProperties& p) {
					return strcmp(requested, p.layerName) == 0;
				});

			if (it != props.end()) {
				enabledLayers.push_back(requested);
				RENDERX_INFO("Enabled validation layer: {}", requested);
			}
			else {
				RENDERX_WARN("Validation layer not available: {}", requested);
			}
		}

		return enabledLayers;
	}

	std::vector<const char*> GetEnabledExtensionsVk(VkPhysicalDevice device) {
		if (device == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid physical device (VK_NULL_HANDLE)");
			return {};
		}

		RENDERX_INFO("Querying device extensions...");
		uint32_t count = 0;
		VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to enumerate device extension properties: {}", VkResultToString(result));
			return {};
		}

		if (count == 0) {
			RENDERX_ERROR("No device extensions available");
			return {};
		}

		RENDERX_INFO("Found {} available device extensions", count);
		std::vector<VkExtensionProperties> props(count);
		result = vkEnumerateDeviceExtensionProperties(device, nullptr, &count, props.data());
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to get device extension properties: {}", VkResultToString(result));
			return {};
		}

		std::vector<const char*> enabledExtensions;
		enabledExtensions.reserve(g_RequestedDeviceExtensions.size());

		for (const char* requested : g_RequestedDeviceExtensions) {
			auto it = std::find_if(props.begin(), props.end(),
				[requested](const VkExtensionProperties& p) {
					return strcmp(requested, p.extensionName) == 0;
				});

			if (it != props.end()) {
				enabledExtensions.push_back(requested);
				RENDERX_INFO("Enabled device extension: {}", requested);
			}
			else {
				RENDERX_ERROR("Required device extension not available: {}", requested);
			}
		}

		return enabledExtensions;
	}

	// ===================== INSTANCE =====================

	bool InitInstance(uint32_t extCount, const char** extentions) {
		PROFILE_FUNCTION();
		RENDERX_INFO("Initializing Vulkan instance...");

		if (extCount > 0 && !extentions) {
			RENDERX_ERROR("Extension count is {}, but extensions pointer is null", extCount);
			return false;
		}

		const auto validationLayers = GetEnabledValidationLayers();

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "RenderX";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "RenderX";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_1;

		std::vector<const char*> instanceExtensions(extentions, extentions + extCount);

		// REQUIRED when validation layers are enabled
		if constexpr (enableValidationLayers) {
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		RENDERX_INFO("Requesting {} instance extensions:", instanceExtensions.size());
		for (auto ext : instanceExtensions) {
			if (ext) {
				RENDERX_INFO("  - {}", ext);
			}
			else {
				RENDERX_WARN("  - NULL extension pointer detected");
			}
		}

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
		createInfo.ppEnabledExtensionNames = instanceExtensions.data();
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.empty() ? nullptr : validationLayers.data();

		const VkResult result = vkCreateInstance(&createInfo, nullptr, &GetVulkanContext().instance);
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to create Vulkan instance: {}", VkResultToString(result));
			return false;
		}

		if (GetVulkanContext().instance == VK_NULL_HANDLE) {
			RENDERX_ERROR("Vulkan instance creation succeeded but returned VK_NULL_HANDLE");
			return false;
		}

		RENDERX_INFO("Vulkan instance created successfully");
		return true;
	}

	// ===================== PHYSICAL DEVICE =====================

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() const {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
		RENDERX_INFO("Finding queue families...");
		QueueFamilyIndices indices;

		if (device == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid physical device (VK_NULL_HANDLE)");
			return indices;
		}

		if (surface == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid surface (VK_NULL_HANDLE)");
			return indices;
		}

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		if (queueFamilyCount == 0) {
			RENDERX_ERROR("No queue families found");
			return indices;
		}

		RENDERX_INFO("Found {} queue families", queueFamilyCount);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; ++i) {
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
				RENDERX_INFO("Graphics queue family found at index {}", i);
			}

			VkBool32 presentSupport = VK_FALSE;
			VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (result != VK_SUCCESS) {
				RENDERX_WARN("Failed to query surface support for queue family {}: {}", i, VkResultToString(result));
				continue;
			}

			if (presentSupport) {
				indices.presentFamily = i;
				RENDERX_INFO("Present queue family found at index {}", i);
			}

			if (indices.isComplete()) {
				break;
			}
		}

		if (!indices.isComplete()) {
			RENDERX_WARN("Could not find complete queue families (graphics: {}, present: {})",
				indices.graphicsFamily.has_value() ? "found" : "missing",
				indices.presentFamily.has_value() ? "found" : "missing");
		}

		return indices;
	}

	bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
		if (device == VK_NULL_HANDLE) {
			RENDERX_ERROR("Cannot check suitability: invalid device");
			return false;
		}

		if (surface == VK_NULL_HANDLE) {
			RENDERX_ERROR("Cannot check suitability: invalid surface");
			return false;
		}

		const auto indices = FindQueueFamilies(device, surface);
		if (!indices.isComplete()) {
			RENDERX_INFO("Device rejected: incomplete queue families");
			return false;
		}

		// Check for required extensions
		const auto extensions = GetEnabledExtensionsVk(device);
		if (extensions.size() != g_RequestedDeviceExtensions.size()) {
			RENDERX_INFO("Device rejected: missing required extensions ({}/{})",
				extensions.size(), g_RequestedDeviceExtensions.size());
			return false;
		}

		// Verify swapchain support
		const auto swapchainSupport = QuerySwapchainSupport(device, surface);
		if (swapchainSupport.formats.empty()) {
			RENDERX_INFO("Device rejected: no surface formats available");
			return false;
		}
		if (swapchainSupport.presentModes.empty()) {
			RENDERX_INFO("Device rejected: no present modes available");
			return false;
		}

		RENDERX_INFO("Device is suitable");
		return true;
	}

	bool PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
		VkPhysicalDevice* outPhysicalDevice, uint32_t* outGraphicsQueueFamily) {
		PROFILE_FUNCTION();
		RENDERX_INFO("Selecting physical device...");

		if (!outPhysicalDevice || !outGraphicsQueueFamily) {
			RENDERX_ERROR("Null output pointers");
			return false;
		}

		if (instance == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid Vulkan instance");
			return false;
		}

		if (surface == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid surface");
			return false;
		}

		uint32_t deviceCount = 0;
		VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to enumerate physical devices: {}", VkResultToString(result));
			return false;
		}

		if (deviceCount == 0) {
			RENDERX_ERROR("No Vulkan-capable devices found");
			return false;
		}

		RENDERX_INFO("Found {} Vulkan-capable device(s)", deviceCount);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to get physical devices: {}", VkResultToString(result));
			return false;
		}

		// Prefer discrete GPU
		VkPhysicalDevice discreteGPU = VK_NULL_HANDLE;
		VkPhysicalDevice fallbackGPU = VK_NULL_HANDLE;

		for (const auto device : devices) {
			if (!IsDeviceSuitable(device, surface)) {
				continue;
			}

			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(device, &props);
			RENDERX_INFO("Found suitable device: {}", props.deviceName);

			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				discreteGPU = device;
			}

			if (fallbackGPU == VK_NULL_HANDLE) {
				fallbackGPU = device;
			}
		}

		const VkPhysicalDevice selectedDevice = discreteGPU != VK_NULL_HANDLE ? discreteGPU : fallbackGPU;

		if (selectedDevice == VK_NULL_HANDLE) {
			RENDERX_ERROR("No suitable physical device found");
			return false;
		}

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(selectedDevice, &props);
		RENDERX_INFO("Selected device: {} ", props.deviceName);

		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(selectedDevice, &memProps);

		uint64_t totalMemory = 0;
		for (uint32_t i = 0; i < memProps.memoryHeapCount; i++) {
			if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				totalMemory += memProps.memoryHeaps[i].size;
			}
		}
		RENDERX_INFO("Device local memory: {} MB", totalMemory / (1024 * 1024));

		const auto indices = FindQueueFamilies(selectedDevice, surface);
		if (!indices.graphicsFamily.has_value()) {
			RENDERX_ERROR("Selected device has no graphics queue family");
			return false;
		}

		*outPhysicalDevice = selectedDevice;
		*outGraphicsQueueFamily = indices.graphicsFamily.value();

		return true;
	}

	// ===================== LOGICAL DEVICE =====================

	bool InitLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamily,
		VkDevice* outDevice, VkQueue* outGraphicsQueue) {
		PROFILE_FUNCTION();
		RENDERX_INFO("Creating logical device...");

		if (!outDevice || !outGraphicsQueue) {
			RENDERX_ERROR("Null output pointers");
			return false;
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid physical device");
			return false;
		}

		constexpr float queuePriority = 1.0f;

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = graphicsQueueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		RENDERX_INFO("Using queue family index: {}", graphicsQueueFamily);

		const auto deviceExtensions = GetEnabledExtensionsVk(physicalDevice);
		if (deviceExtensions.empty()) {
			RENDERX_ERROR("No device extensions available");
			return false;
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		// Enable any required features here

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		const VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, outDevice);
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to create logical device: {}", VkResultToString(result));
			return false;
		}

		if (*outDevice == VK_NULL_HANDLE) {
			RENDERX_ERROR("Logical device creation succeeded but returned VK_NULL_HANDLE");
			return false;
		}

		vkGetDeviceQueue(*outDevice, graphicsQueueFamily, 0, outGraphicsQueue);

		if (*outGraphicsQueue == VK_NULL_HANDLE) {
			RENDERX_ERROR("Failed to retrieve graphics queue");
			vkDestroyDevice(*outDevice, nullptr);
			*outDevice = VK_NULL_HANDLE;
			return false;
		}

		RENDERX_INFO("Logical device created successfully");
		return true;
	}

	void CreateSurface(Window window) {
		RENDERX_INFO("Creating window surface...");

		if (!window.nativeHandle) {
			RENDERX_ERROR("Null window pointer");
			return;
		}

#if 1
		VkWin32SurfaceCreateInfoKHR ci{};
		ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		ci.hwnd = static_cast<HWND>(window.nativeHandle);
		ci.hinstance = static_cast<HINSTANCE>(window.displayHandle);
		auto& ctx = GetVulkanContext();
		if (!ci.hwnd) {
			RENDERX_ERROR("Invalid HWND");
			return;
		}

		if (!ci.hinstance) {
			RENDERX_ERROR("Failed to get module handle");
			return;
		}

		VkResult result = vkCreateWin32SurfaceKHR(ctx.instance, &ci, nullptr, &ctx.surface);

		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to create Win32 surface: {}", VkResultToString(result));
			return;
		}

		if (ctx.surface == VK_NULL_HANDLE) {
			RENDERX_ERROR("Surface creation succeeded but returned VK_NULL_HANDLE");
			return;
		}

		RENDERX_INFO("Win32 surface created successfully");
#else
		auto& ctx = GetVulkanContext();
		auto result = glfwCreateWindowSurface(ctx.instance, static_cast<GLFWwindow*>(window), nullptr, &ctx.surface);
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to create Win32 surface: {}", VkResultToString(result));
			return;
		}
		RENDERX_ERROR("Platform not supported for surface creation");
#endif
	}

	// ===================== SWAPCHAIN =====================

	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
		PROFILE_FUNCTION();
		SwapchainSupportDetails details{};

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		if (device == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid physical device");
			return details;
		}

		if (surface == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid surface");
			return details;
		}

		VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to get surface capabilities: {}", VkResultToString(result));
			return details;
		}

		uint32_t formatCount = 0;
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to query surface formats: {}", VkResultToString(result));
			return details;
		}

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to get surface formats: {}", VkResultToString(result));
				details.formats.clear();
			}
		}

		uint32_t presentModeCount = 0;
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to query present modes: {}", VkResultToString(result));
			return details;
		}

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
				details.presentModes.data());
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to get present modes: {}", VkResultToString(result));
				details.presentModes.clear();
			}
		}

		return details;
	}

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
		PROFILE_FUNCTION();

		if (formats.empty()) {
			RENDERX_ERROR("No surface formats available");
			return {};
		}

		// Prefer SRGB if available for better color accuracy
		for (const auto& format : formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
				format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				RENDERX_INFO("Selected surface format: B8G8R8A8_SRGB");
				return format;
			}
		}

		// Fallback to UNORM
		for (const auto& format : formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM) {
				RENDERX_INFO("Selected surface format: B8G8R8A8_UNORM");
				return format;
			}
		}

		RENDERX_WARN("Using first available format as fallback");
		return formats[0];
	}

	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
		PROFILE_FUNCTION();

		if (modes.empty()) {
			RENDERX_ERROR("No present modes available, defaulting to FIFO");
			return VK_PRESENT_MODE_FIFO_KHR;
		}

		// Prefer mailbox for low latency triple buffering
		for (const auto mode : modes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				RENDERX_INFO("Selected present mode: MAILBOX");
				return mode;
			}
		}

		RENDERX_INFO("Selected present mode: FIFO (guaranteed)");
		return VK_PRESENT_MODE_FIFO_KHR; // FIFO is guaranteed to be available
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps, Window window) {
		PROFILE_FUNCTION();

		if (caps.currentExtent.width != UINT32_MAX) {
			RENDERX_INFO("Using current extent: {}x{}", caps.currentExtent.width, caps.currentExtent.height);
			return caps.currentExtent;
		}

		if (window.width == 0 || window.height == 0) {
			RENDERX_WARN("Invalid window dimensions: {}x{}, using minimum extent", window.width, window.height);
			return caps.minImageExtent;
		}

		VkExtent2D actualExtent = {
			window.width,
			window.height
		};

		actualExtent.width = std::clamp(actualExtent.width,
			caps.minImageExtent.width, caps.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height,
			caps.minImageExtent.height, caps.maxImageExtent.height);

		RENDERX_INFO("Chosen swap extent: {}x{} (clamped from {}x{})",
			actualExtent.width, actualExtent.height, window.width, window.height);

		return actualExtent;
	}

	bool CreateSwapchain(VulkanContext& ctx, Window window) {
		PROFILE_FUNCTION();
		RENDERX_INFO("Creating swapchain...");

		if (!window.nativeHandle) {
			RENDERX_ERROR("Null window pointer");
			return false;
		}

		if (ctx.device == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid device");
			return false;
		}

		if (ctx.surface == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid surface");
			return false;
		}

		if (ctx.physicalDevice == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid physical device");
			return false;
		}

		const auto support = QuerySwapchainSupport(ctx.physicalDevice, ctx.surface);
		if (support.formats.empty() || support.presentModes.empty()) {
			RENDERX_ERROR("Insufficient swapchain support (formats: {}, modes: {})",
				support.formats.size(), support.presentModes.size());
			return false;
		}

		const auto surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
		const auto presentMode = ChoosePresentMode(support.presentModes);
		const auto extent = ChooseSwapExtent(support.capabilities, window);

		// Request one more than minimum for better performance
		uint32_t imageCount = support.capabilities.minImageCount + 1;
		if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
			imageCount = support.capabilities.maxImageCount;
			RENDERX_INFO("Clamped image count to max: {}", imageCount);
		}

		RENDERX_INFO("Requesting {} swapchain images (min: {}, max: {})",
			imageCount, support.capabilities.minImageCount, support.capabilities.maxImageCount);

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = ctx.surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.preTransform = support.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		const VkResult result = vkCreateSwapchainKHR(ctx.device, &createInfo, nullptr, &ctx.swapchain);
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to create swapchain: {}", VkResultToString(result));
			RENDERX_ASSERT(false);
		}

		if (ctx.swapchain == VK_NULL_HANDLE) {
			RENDERX_ERROR("Swapchain creation succeeded but returned VK_NULL_HANDLE");
			RENDERX_ASSERT(false);
		}

		ctx.swapchainImageFormat = surfaceFormat.format;
		ctx.swapchainExtent = extent;

		// Retrieve swapchain images
		uint32_t swapchainImageCount = 0;
		VkResult imgResult = vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &swapchainImageCount, nullptr);
		if (imgResult != VK_SUCCESS) {
			RENDERX_ERROR("Failed to query swapchain image count: {}", static_cast<int>(imgResult));
			return false;
		}

		if (swapchainImageCount == 0) {
			RENDERX_ERROR("Swapchain has no images");
			return false;
		}

		ctx.swapchainImages.resize(swapchainImageCount);
		imgResult = vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &swapchainImageCount,
			ctx.swapchainImages.data());
		if (imgResult != VK_SUCCESS) {
			RENDERX_ERROR("Failed to retrieve swapchain images: {}", static_cast<int>(imgResult));
			return false;
		}

		// Create image views
		ctx.swapchainImageviews.reserve(ctx.swapchainImages.size());
		for (size_t i = 0; i < ctx.swapchainImages.size(); ++i) {
			if (ctx.swapchainImages[i] == VK_NULL_HANDLE) {
				RENDERX_ERROR("Swapchain image {} is VK_NULL_HANDLE", i);
				return false;
			}

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = ctx.swapchainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = ctx.swapchainImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			VkResult viewResult = vkCreateImageView(ctx.device, &viewInfo, nullptr, &imageView);
			if (viewResult != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create image view {}: {}", i, static_cast<int>(viewResult));
				return false;
			}

			if (imageView == VK_NULL_HANDLE) {
				RENDERX_ERROR("Image view {} creation succeeded but returned VK_NULL_HANDLE", i);
				return false;
			}

			ctx.swapchainImageviews.push_back(imageView);
		}

		RENDERX_INFO("Swapchain created: {}x{}, {} images", extent.width, extent.height, swapchainImageCount);
		return true;
	}

	void CreateSwapchainFramebuffers(RenderPassHandle renderPass) {
		RENDERX_INFO("Creating swapchain framebuffers...");
		VulkanContext& ctx = GetVulkanContext();

		if (ctx.device == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid device");
			return;
		}

		auto it = g_RenderPasses.find(renderPass.id);
		if (it == g_RenderPasses.end()) {
			RENDERX_ERROR("Invalid render pass handle: {}", renderPass.id);
			return;
		}

		const VkRenderPass vkPass = it->second;
		if (vkPass == VK_NULL_HANDLE) {
			RENDERX_ERROR("Render pass is VK_NULL_HANDLE");
			return;
		}

		if (ctx.swapchainImageviews.empty()) {
			RENDERX_ERROR("No swapchain image views available");
			return;
		}

		if (ctx.swapchainExtent.width == 0 || ctx.swapchainExtent.height == 0) {
			RENDERX_ERROR("Invalid swapchain extent: {}x{}", ctx.swapchainExtent.width, ctx.swapchainExtent.height);
			return;
		}

		ctx.swapchainFramebuffers.resize(ctx.swapchainImageviews.size());

		for (size_t i = 0; i < ctx.swapchainImageviews.size(); i++) {
			if (ctx.swapchainImageviews[i] == VK_NULL_HANDLE) {
				RENDERX_ERROR("Image view {} is VK_NULL_HANDLE", i);
				return;
			}

			const VkImageView attachments[] = { ctx.swapchainImageviews[i] };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = vkPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = ctx.swapchainExtent.width;
			framebufferInfo.height = ctx.swapchainExtent.height;
			framebufferInfo.layers = 1;

			VkResult result = vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, &ctx.swapchainFramebuffers[i]);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create framebuffer {}: {}", i, VkResultToString(result));
				return;
			}

			if (ctx.swapchainFramebuffers[i] == VK_NULL_HANDLE) {
				RENDERX_ERROR("Framebuffer {} creation succeeded but returned VK_NULL_HANDLE", i);
				return;
			}
		}

		RENDERX_INFO("Created {} framebuffers ({}x{})", ctx.swapchainFramebuffers.size(),
			ctx.swapchainExtent.width, ctx.swapchainExtent.height);
	}

	void InitFrameContex() {
		RENDERX_INFO("Initializing frame contexts...");
		auto& ctx = GetVulkanContext();

		if (ctx.device == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid device");
			return;
		}

		if (ctx.swapchainImageviews.empty()) {
			RENDERX_ERROR("No swapchain image views available");
			return;
		}

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = ctx.graphicsQueueFamilyIndex;

		RENDERX_INFO("Creating synchronization objects for {} swapchain images", ctx.swapchainImageviews.size());
		ctx.swapchainImageSync.resize(ctx.swapchainImageviews.size());

		for (size_t i = 0; i < ctx.swapchainImageSync.size(); ++i) {
			auto& iSync = ctx.swapchainImageSync[i];

			VkResult result = vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &iSync.imageAvailableSemaphore);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create imageAvailableSemaphore for image {}: {}", i, VkResultToString(result));
				return;
			}

			result = vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &iSync.renderFinishedSemaphore);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create renderFinishedSemaphore for image {}: {}", i, VkResultToString(result));
				return;
			}

			if (iSync.imageAvailableSemaphore == VK_NULL_HANDLE || iSync.renderFinishedSemaphore == VK_NULL_HANDLE) {
				RENDERX_ERROR("Semaphore creation for image {} returned VK_NULL_HANDLE", i);
				return;
			}
		}

		RENDERX_INFO("Creating frame resources for {} frames in flight", MAX_FRAMES_IN_FLIGHT);
		for (size_t i = 0; i < g_Frames.size(); ++i) {
			auto& frame = g_Frames[i];

			VkResult result = vkCreateFence(ctx.device, &fenceInfo, nullptr, &frame.fence);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create fence for frame {}: {}", i, VkResultToString(result));
				return;
			}

			result = vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &frame.presentSemaphore);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create presentSemaphore for frame {}: {}", i, VkResultToString(result));
				return;
			}

			result = vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &frame.renderSemaphore);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create renderSemaphore for frame {}: {}", i, VkResultToString(result));
				return;
			}

			result = vkCreateCommandPool(ctx.device, &poolInfo, nullptr, &frame.commandPool);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create command pool for frame {}: {}", i, VkResultToString(result));
				return;
			}

			if (frame.fence == VK_NULL_HANDLE || frame.presentSemaphore == VK_NULL_HANDLE ||
				frame.renderSemaphore == VK_NULL_HANDLE || frame.commandPool == VK_NULL_HANDLE) {
				RENDERX_ERROR("Frame {} resource creation returned VK_NULL_HANDLE", i);
				return;
			}
		}

		RENDERX_INFO("Frame contexts initialized successfully: {} frames", MAX_FRAMES_IN_FLIGHT);
	}

	// ===================== VULKAN HELPERS =====================

	VkRenderPass GetVulkanRenderPass(RenderPassHandle handle) {
		if (handle.id == 0) {
			RENDERX_WARN("Invalid render pass handle: id is 0");
			return VK_NULL_HANDLE;
		}

		const auto it = g_RenderPasses.find(handle.id);
		if (it == g_RenderPasses.end()) {
			RENDERX_WARN("Render pass not found for handle: {}", handle.id);
			return VK_NULL_HANDLE;
		}

		return it->second;
	}

} // namespace RenderXVK
} // namespace RenderX