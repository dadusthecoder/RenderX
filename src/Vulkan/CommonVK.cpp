#include "CommonVK.h"
#include "RenderXVK.h"
#include "Windows.h"
#include <algorithm>
#include <vulkan/vulkan_win32.h>

// Define VMA implementation - must be included in exactly one source file
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#ifdef RENDERX_DEBUG
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

namespace Rx {
namespace RxVK {

	uint32_t MAX_FRAMES_IN_FLIGHT;
	static std::vector<FrameContex> g_Frames;

	// ResourcePool instances for resource management
	ResourcePool<VulkanTexture, TextureHandle> g_TexturePool;
	ResourcePool<VulkanBuffer, BufferHandle> g_BufferPool;
	ResourcePool<VulkanShader, ShaderHandle> g_ShaderPool;
	ResourcePool<VulkanPipeline, PipelineHandle> g_PipelinePool;
	ResourcePool<VkRenderPass, RenderPassHandle> g_RenderPassPool;

	// // Legacy unordered_map declarations (deprecated, kept for compatibility)
	// std::unordered_map<uint64_t, VulkanBuffer> g_Buffers;
	// std::unordered_map<uint64_t, VulkanShader> g_Shaders;
	// std::unordered_map<uint64_t, VkRenderPass> g_RenderPasses;

	uint32_t g_CurrentFrame = 0;

	VulkanContext& GetVulkanContext() {
		static VulkanContext g_Context;
		return g_Context;
	}

	FrameContex& GetFrameContex(uint32_t index) {
		return g_Frames[index];
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

	inline std::vector<const char*> GetEnabledValidationLayers() {
		if constexpr (!enableValidationLayers) {
			RENDERX_INFO("Validation layers disabled (Release build)");
			return {};
		}

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
				RENDERX_INFO("Requested validation layer: {}", requested);
			}
			else {
				RENDERX_WARN("Validation layer not available: {}", requested);
			}
		}

		return enabledLayers;
	}

	inline std::vector<const char*> GetEnabledExtensionsVk(VkPhysicalDevice device) {
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

	// ==== INSTANCE ====

	bool InitInstance(uint32_t extCount, const char** extentions) {
		PROFILE_FUNCTION();

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

	// ==== PHYSICAL DEVICE ====

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() const {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	inline QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
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

	inline bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
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
			RENDERX_ERROR("No capable devices found");
			return false;
		}

		RENDERX_INFO("Found {} capable device(s)", deviceCount);
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

	// ==== LOGICAL DEVICE ====

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

	bool InitVulkanMemoryAllocator(
		VkInstance instance,
		VkPhysicalDevice physicalDevice,
		VkDevice device) {
		PROFILE_FUNCTION();
		RENDERX_INFO("Initializing Vulkan Memory Allocator (VMA)...");

		if (instance == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid Vulkan instance for VMA initialization");
			return false;
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid physical device for VMA initialization");
			return false;
		}

		if (device == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid logical device for VMA initialization");
			return false;
		}

		VulkanContext& ctx = GetVulkanContext();

		// Initialize device limits
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(physicalDevice, &props);
		ctx.deviceLimits.minUniformBufferOffsetAlignment = static_cast<uint32_t>(props.limits.minUniformBufferOffsetAlignment);
		ctx.deviceLimits.minStorageBufferOffsetAlignment = static_cast<uint32_t>(props.limits.minStorageBufferOffsetAlignment);
		// minDrawIndirectBufferOffsetAlignment may not be available in all Vulkan versions, use minUniformBufferOffsetAlignment as fallback
		ctx.deviceLimits.minDrawIndirectBufferOffsetAlignment = static_cast<uint32_t>(props.limits.minUniformBufferOffsetAlignment);

		RENDERX_INFO("Device limits - Uniform: {}, Storage: {}, Indirect (fallback): {}",
			ctx.deviceLimits.minUniformBufferOffsetAlignment,
			ctx.deviceLimits.minStorageBufferOffsetAlignment,
			ctx.deviceLimits.minDrawIndirectBufferOffsetAlignment);

		VmaAllocatorCreateInfo allocatorInfo{};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
		allocatorInfo.instance = instance;
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = device;

		VkResult result = vmaCreateAllocator(&allocatorInfo, &ctx.allocator);
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to create VMA allocator: {}", VkResultToString(result));
			ctx.allocator = VK_NULL_HANDLE;
			return false;
		}

		if (ctx.allocator == VK_NULL_HANDLE) {
			RENDERX_ERROR("VMA allocator creation succeeded but returned VK_NULL_HANDLE");
			return false;
		}

		RENDERX_INFO("Vulkan Memory Allocator (VMA) initialized successfully");
		return true;
	}

	void CreateSurface(Window window) {
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

	// ==== SWAPCHAIN ====

	inline SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
		PROFILE_FUNCTION();
		SwapchainSupportDetails details{};

		// Removed unintended sleep to avoid adding latency during swapchain queries

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

	inline VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
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

	inline VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
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

	inline VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps, Window window) {
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

	inline void CreateSwapchainFramebuffers(RenderPassHandle renderPass) {
		VulkanContext& ctx = GetVulkanContext();

		if (ctx.device == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid device");
			return;
		}

		const VkRenderPass* vkPass = g_RenderPassPool.get(renderPass);
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
			framebufferInfo.renderPass = *vkPass;
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

	bool CreateSwapchain(VulkanContext& ctx, Window window) {
		PROFILE_FUNCTION();
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
			return false;
		}

		if (ctx.swapchain == VK_NULL_HANDLE) {
			RENDERX_ERROR("Swapchain creation succeeded but returned VK_NULL_HANDLE");
			RENDERX_ASSERT(false);
			return false;
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

		Rx::RenderPassDesc renderPassDesc{};
		Rx::AttachmentDesc colorAttach;
		colorAttach.format = Rx::TextureFormat::BGRA8_SRGB;
		colorAttach.finalState = Rx::ResourceState::Present;
		renderPassDesc.colorAttachments.push_back(colorAttach);
		// temp
		//  renderPassDesc.hasDepthStencil = true;

		auto renderPass = VKCreateRenderPass(renderPassDesc);
		CreateSwapchainFramebuffers(renderPass);
		ctx.swapchainRenderPassHandle = renderPass;
		ctx.swapchainRenderPass = GetVulkanRenderPass(renderPass);

		return true;
	}

	bool CreatePerFrameUploadBuffer(FrameContex& frame, uint32_t bufferSize) {
		PROFILE_FUNCTION();
		VulkanContext& ctx = GetVulkanContext();

		if (bufferSize == 0) {
			RENDERX_ERROR("Upload buffer size must be greater than 0");
			return false;
		}

		if (ctx.allocator == VK_NULL_HANDLE) {
			RENDERX_ERROR("VMA allocator not initialized");
			return false;
		}

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
						  VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VkResult result = vmaCreateBuffer(ctx.allocator, &bufferInfo, &allocInfo,
			&frame.upload.buffer, &frame.upload.allocation, nullptr);

		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to create upload buffer: {}", VkResultToString(result));
			return false;
		}

		if (frame.upload.buffer == VK_NULL_HANDLE) {
			RENDERX_ERROR("Upload buffer creation succeeded but returned VK_NULL_HANDLE");
			return false;
		}

		// Map the buffer for writing
		VmaAllocationInfo allocInfo2{};
		vmaGetAllocationInfo(ctx.allocator, frame.upload.allocation, &allocInfo2);
		frame.upload.mappedPtr = static_cast<uint8_t*>(allocInfo2.pMappedData);

		if (frame.upload.mappedPtr == nullptr) {
			RENDERX_ERROR("Failed to map upload buffer");
			vmaDestroyBuffer(ctx.allocator, frame.upload.buffer, frame.upload.allocation);
			return false;
		}

		frame.upload.size = bufferSize;
		frame.upload.offset = 0;

		RENDERX_INFO("Created per-frame upload buffer: {} bytes", bufferSize);
		return true;
	}


	void InitFrameContext() {
		auto& ctx = GetVulkanContext();
		g_Frames.resize(MAX_FRAMES_IN_FLIGHT);

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

		VkCommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cmdPoolInfo.queueFamilyIndex = ctx.graphicsQueueFamilyIndex;

		VkDescriptorPoolCreateInfo descPoolInfo{};
		descPoolInfo.poolSizeCount = 1;
		// descPoolInfo.pPoolSizes = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1};


		ctx.swapchainImageSync.resize(ctx.swapchainImageviews.size());


		// Create per-swapchain-image semaphores; ensure cleanup on partial failure
		for (size_t i = 0; i < ctx.swapchainImageSync.size(); ++i) {
			auto& iSync = ctx.swapchainImageSync[i];

			VkResult result = vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &iSync.imageAvailableSemaphore);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create imageAvailableSemaphore for image {}: {}", i, VkResultToString(result));
				// cleanup previously created semaphores
				for (size_t j = 0; j < i; ++j) {
					if (ctx.swapchainImageSync[j].imageAvailableSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, ctx.swapchainImageSync[j].imageAvailableSemaphore, nullptr);
					if (ctx.swapchainImageSync[j].renderFinishedSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, ctx.swapchainImageSync[j].renderFinishedSemaphore, nullptr);
					ctx.swapchainImageSync[j].imageAvailableSemaphore = VK_NULL_HANDLE;
					ctx.swapchainImageSync[j].renderFinishedSemaphore = VK_NULL_HANDLE;
				}
				return;
			}

			result = vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &iSync.renderFinishedSemaphore);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create renderFinishedSemaphore for image {}: {}", i, VkResultToString(result));
				// destroy the imageAvailableSemaphore created for this index and previous ones
				if (iSync.imageAvailableSemaphore != VK_NULL_HANDLE)
					vkDestroySemaphore(ctx.device, iSync.imageAvailableSemaphore, nullptr);
				for (size_t j = 0; j < i; ++j) {
					if (ctx.swapchainImageSync[j].imageAvailableSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, ctx.swapchainImageSync[j].imageAvailableSemaphore, nullptr);
					if (ctx.swapchainImageSync[j].renderFinishedSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, ctx.swapchainImageSync[j].renderFinishedSemaphore, nullptr);
					ctx.swapchainImageSync[j].imageAvailableSemaphore = VK_NULL_HANDLE;
					ctx.swapchainImageSync[j].renderFinishedSemaphore = VK_NULL_HANDLE;
				}
				return;
			}

			if (iSync.imageAvailableSemaphore == VK_NULL_HANDLE || iSync.renderFinishedSemaphore == VK_NULL_HANDLE) {
				RENDERX_ERROR("Semaphore creation for image {} returned VK_NULL_HANDLE", i);
				// cleanup created semaphores
				for (size_t j = 0; j <= i; ++j) {
					if (ctx.swapchainImageSync[j].imageAvailableSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, ctx.swapchainImageSync[j].imageAvailableSemaphore, nullptr);
					if (ctx.swapchainImageSync[j].renderFinishedSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, ctx.swapchainImageSync[j].renderFinishedSemaphore, nullptr);
					ctx.swapchainImageSync[j].imageAvailableSemaphore = VK_NULL_HANDLE;
					ctx.swapchainImageSync[j].renderFinishedSemaphore = VK_NULL_HANDLE;
				}
				return;
			}
		}


		// Create per-frame resources; ensure cleanup on partial failures
		for (size_t i = 0; i < g_Frames.size(); ++i) {
			auto& frame = g_Frames[i];

			VkResult result = vkCreateFence(ctx.device, &fenceInfo, nullptr, &frame.fence);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create fence for frame {}: {}", i, VkResultToString(result));
				// cleanup previously created frames
				for (size_t j = 0; j < i; ++j) {
					if (g_Frames[j].fence != VK_NULL_HANDLE)
						vkDestroyFence(ctx.device, g_Frames[j].fence, nullptr);
					if (g_Frames[j].presentSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, g_Frames[j].presentSemaphore, nullptr);
					if (g_Frames[j].renderSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, g_Frames[j].renderSemaphore, nullptr);
					if (g_Frames[j].commandPool != VK_NULL_HANDLE)
						vkDestroyCommandPool(ctx.device, g_Frames[j].commandPool, nullptr);
					g_Frames[j].fence = VK_NULL_HANDLE;
					g_Frames[j].presentSemaphore = VK_NULL_HANDLE;
					g_Frames[j].renderSemaphore = VK_NULL_HANDLE;
					g_Frames[j].commandPool = VK_NULL_HANDLE;
				}
				// also cleanup swapchain image semaphores
				for (auto& s : ctx.swapchainImageSync) {
					if (s.imageAvailableSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, s.imageAvailableSemaphore, nullptr);
					if (s.renderFinishedSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, s.renderFinishedSemaphore, nullptr);
					s.imageAvailableSemaphore = VK_NULL_HANDLE;
					s.renderFinishedSemaphore = VK_NULL_HANDLE;
				}
				return;
			}

			result = vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &frame.presentSemaphore);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create presentSemaphore for frame {}: {}", i, VkResultToString(result));
				// cleanup created resources for previous frames and this one
				if (frame.fence != VK_NULL_HANDLE) {
					vkDestroyFence(ctx.device, frame.fence, nullptr);
					frame.fence = VK_NULL_HANDLE;
				}
				for (size_t j = 0; j < i; ++j) {
					if (g_Frames[j].fence != VK_NULL_HANDLE)
						vkDestroyFence(ctx.device, g_Frames[j].fence, nullptr);
					if (g_Frames[j].presentSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, g_Frames[j].presentSemaphore, nullptr);
					if (g_Frames[j].renderSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, g_Frames[j].renderSemaphore, nullptr);
					if (g_Frames[j].commandPool != VK_NULL_HANDLE)
						vkDestroyCommandPool(ctx.device, g_Frames[j].commandPool, nullptr);
					g_Frames[j].fence = VK_NULL_HANDLE;
					g_Frames[j].presentSemaphore = VK_NULL_HANDLE;
					g_Frames[j].renderSemaphore = VK_NULL_HANDLE;
					g_Frames[j].commandPool = VK_NULL_HANDLE;
				}
				// cleanup swapchain image semaphores
				for (auto& s : ctx.swapchainImageSync) {
					if (s.imageAvailableSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, s.imageAvailableSemaphore, nullptr);
					if (s.renderFinishedSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, s.renderFinishedSemaphore, nullptr);
					s.imageAvailableSemaphore = VK_NULL_HANDLE;
					s.renderFinishedSemaphore = VK_NULL_HANDLE;
				}
				return;
			}

			result = vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &frame.renderSemaphore);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create renderSemaphore for frame {}: {}", i, VkResultToString(result));
				// cleanup resources for this and previous frames
				if (frame.presentSemaphore != VK_NULL_HANDLE) {
					vkDestroySemaphore(ctx.device, frame.presentSemaphore, nullptr);
					frame.presentSemaphore = VK_NULL_HANDLE;
				}
				if (frame.fence != VK_NULL_HANDLE) {
					vkDestroyFence(ctx.device, frame.fence, nullptr);
					frame.fence = VK_NULL_HANDLE;
				}
				for (size_t j = 0; j < i; ++j) {
					if (g_Frames[j].fence != VK_NULL_HANDLE)
						vkDestroyFence(ctx.device, g_Frames[j].fence, nullptr);
					if (g_Frames[j].presentSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, g_Frames[j].presentSemaphore, nullptr);
					if (g_Frames[j].renderSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, g_Frames[j].renderSemaphore, nullptr);
					if (g_Frames[j].commandPool != VK_NULL_HANDLE)
						vkDestroyCommandPool(ctx.device, g_Frames[j].commandPool, nullptr);
					g_Frames[j].fence = VK_NULL_HANDLE;
					g_Frames[j].presentSemaphore = VK_NULL_HANDLE;
					g_Frames[j].renderSemaphore = VK_NULL_HANDLE;
					g_Frames[j].commandPool = VK_NULL_HANDLE;
				}
				// cleanup swapchain image semaphores
				for (auto& s : ctx.swapchainImageSync) {
					if (s.imageAvailableSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, s.imageAvailableSemaphore, nullptr);
					if (s.renderFinishedSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, s.renderFinishedSemaphore, nullptr);
					s.imageAvailableSemaphore = VK_NULL_HANDLE;
					s.renderFinishedSemaphore = VK_NULL_HANDLE;
				}
				return;
			}

			result = vkCreateCommandPool(ctx.device, &cmdPoolInfo, nullptr, &frame.commandPool);
			if (result != VK_SUCCESS) {
				RENDERX_ERROR("Failed to create command pool for frame {}: {}", i, VkResultToString(result));
				// cleanup this frame and previous frames
				if (frame.renderSemaphore != VK_NULL_HANDLE) {
					vkDestroySemaphore(ctx.device, frame.renderSemaphore, nullptr);
					frame.renderSemaphore = VK_NULL_HANDLE;
				}
				if (frame.presentSemaphore != VK_NULL_HANDLE) {
					vkDestroySemaphore(ctx.device, frame.presentSemaphore, nullptr);
					frame.presentSemaphore = VK_NULL_HANDLE;
				}
				if (frame.fence != VK_NULL_HANDLE) {
					vkDestroyFence(ctx.device, frame.fence, nullptr);
					frame.fence = VK_NULL_HANDLE;
				}
				for (size_t j = 0; j < i; ++j) {
					if (g_Frames[j].fence != VK_NULL_HANDLE)
						vkDestroyFence(ctx.device, g_Frames[j].fence, nullptr);
					if (g_Frames[j].presentSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, g_Frames[j].presentSemaphore, nullptr);
					if (g_Frames[j].renderSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, g_Frames[j].renderSemaphore, nullptr);
					if (g_Frames[j].commandPool != VK_NULL_HANDLE)
						vkDestroyCommandPool(ctx.device, g_Frames[j].commandPool, nullptr);
					g_Frames[j].fence = VK_NULL_HANDLE;
					g_Frames[j].presentSemaphore = VK_NULL_HANDLE;
					g_Frames[j].renderSemaphore = VK_NULL_HANDLE;
					g_Frames[j].commandPool = VK_NULL_HANDLE;
				}
				// cleanup swapchain image semaphores
				for (auto& s : ctx.swapchainImageSync) {
					if (s.imageAvailableSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, s.imageAvailableSemaphore, nullptr);
					if (s.renderFinishedSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, s.renderFinishedSemaphore, nullptr);
					s.imageAvailableSemaphore = VK_NULL_HANDLE;
					s.renderFinishedSemaphore = VK_NULL_HANDLE;
				}
				return;
			}

			if (frame.fence == VK_NULL_HANDLE || frame.presentSemaphore == VK_NULL_HANDLE ||
				frame.renderSemaphore == VK_NULL_HANDLE || frame.commandPool == VK_NULL_HANDLE) {
				RENDERX_ERROR("Frame {} resource creation returned VK_NULL_HANDLE", i);
				// cleanup already created resources
				for (size_t j = 0; j <= i; ++j) {
					if (g_Frames[j].fence != VK_NULL_HANDLE)
						vkDestroyFence(ctx.device, g_Frames[j].fence, nullptr);
					if (g_Frames[j].presentSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, g_Frames[j].presentSemaphore, nullptr);
					if (g_Frames[j].renderSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, g_Frames[j].renderSemaphore, nullptr);
					if (g_Frames[j].commandPool != VK_NULL_HANDLE)
						vkDestroyCommandPool(ctx.device, g_Frames[j].commandPool, nullptr);
					g_Frames[j].fence = VK_NULL_HANDLE;
					g_Frames[j].presentSemaphore = VK_NULL_HANDLE;
					g_Frames[j].renderSemaphore = VK_NULL_HANDLE;
					g_Frames[j].commandPool = VK_NULL_HANDLE;
				}
				// cleanup swapchain image semaphores
				for (auto& s : ctx.swapchainImageSync) {
					if (s.imageAvailableSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, s.imageAvailableSemaphore, nullptr);
					if (s.renderFinishedSemaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(ctx.device, s.renderFinishedSemaphore, nullptr);
					s.imageAvailableSemaphore = VK_NULL_HANDLE;
					s.renderFinishedSemaphore = VK_NULL_HANDLE;
				}
				return;
			}

			// temp
			DescriptorPoolSizes kTransientPoolSizes;
			kTransientPoolSizes.uniformBufferCount = 12000;
			kTransientPoolSizes.storageBufferCount = 6000;
			kTransientPoolSizes.sampledImageCount = 24000;
			kTransientPoolSizes.storageImageCount = 1000;
			kTransientPoolSizes.samplerCount = 12000;
			kTransientPoolSizes.combinedImageSamplerCount = 6000;
			kTransientPoolSizes.maxSets = 12000;

			frame.DescriptorPool = CreateDescriptorPool(kTransientPoolSizes, true);
			if (frame.DescriptorPool == VK_NULL_HANDLE) {
				RENDERX_ERROR("perFrame Pool Creation failed {} {}", __LINE__, __FILE__);
				VK_NULL_HANDLE;
				return;
			}

			// Create per-frame upload buffer (256 MB per frame)
			constexpr uint32_t uploadBufferSize = 256 * 1024 * 1024; // 256 MB
			if (!CreatePerFrameUploadBuffer(frame, uploadBufferSize)) {
				RENDERX_ERROR("Failed to create upload buffer for frame {}", i);
				return;
			}
		}

		RENDERX_INFO("Frame contexts initialized successfully: {} frames", MAX_FRAMES_IN_FLIGHT);
	}

	// ==== VULKAN HELPERS ====

	VkRenderPass GetVulkanRenderPass(RenderPassHandle handle) {
		return *g_RenderPassPool.get(handle);
	}

	void freeAllVulkanResources() {
		auto g_Device = GetVulkanContext().device;
		auto ctx = GetVulkanContext();
		vkDeviceWaitIdle(ctx.device);


		// Resource Groups (Descriptor Sets)
		g_TransientResourceGroupPool.ForEach([](VulkanResourceGroup& group) {
			group.set = VK_NULL_HANDLE;
		});

		// Buffer Views
		g_BufferViewPool.ForEach([](VulkanBufferView& view) {
			view.isValid = false;
		});

		// Buffers
		g_BufferPool.ForEach([&](VulkanBuffer& buffer) {
			if (buffer.buffer != VK_NULL_HANDLE) {
				vmaDestroyBuffer(ctx.allocator, buffer.buffer, buffer.allocation);
				buffer.buffer = VK_NULL_HANDLE;
				buffer.allocation = VK_NULL_HANDLE;
			}

			buffer.bindingCount = 0;
			buffer.allocInfo = {};
		});

		// Textures
		g_TexturePool.ForEach([&](VulkanTexture& texture) {
			if (texture.view != VK_NULL_HANDLE) {
				vkDestroyImageView(g_Device, texture.view, nullptr);
				texture.view = VK_NULL_HANDLE;
			}

			if (texture.image != VK_NULL_HANDLE) {
				vkDestroyImage(g_Device, texture.image, nullptr);
				texture.image = VK_NULL_HANDLE;
			}

			if (texture.memory != VK_NULL_HANDLE) {
				vkFreeMemory(g_Device, texture.memory, nullptr);
				texture.memory = VK_NULL_HANDLE;
			}

			texture.format = VK_FORMAT_UNDEFINED;
			texture.width = 0;
			texture.height = 0;
			texture.mipLevels = 1;
			texture.isValid = false;
		});

		// Shaders
		g_ShaderPool.ForEach([&](VulkanShader& shader) {
			if (shader.shaderModule != VK_NULL_HANDLE) {
				vkDestroyShaderModule(g_Device, shader.shaderModule, nullptr);
				shader.shaderModule = VK_NULL_HANDLE;
			}

			shader.entryPoint.clear();
		});

		// Pipeline Layouts
		g_LayoutPool.ForEach([&](VulkanPipelineLayout& layout) {
			if (layout.layout != VK_NULL_HANDLE && layout.setlayouts.size() != 0) {
				for (auto setlayout : layout.setlayouts)
					vkDestroyDescriptorSetLayout(g_Device, setlayout, nullptr);

				vkDestroyPipelineLayout(g_Device, layout.layout, nullptr);
				layout.layout = VK_NULL_HANDLE;
			}
		});

		// Pipelines
		g_PipelinePool.ForEach([&](VulkanPipeline& pipeline) {
			if (pipeline.pipeline != VK_NULL_HANDLE) {
				vkDestroyPipeline(g_Device, pipeline.pipeline, nullptr);
				pipeline.pipeline = VK_NULL_HANDLE;
			}
		});

		// Render Passes
		g_RenderPassPool.ForEach([&](VkRenderPass& rp) {
			if (rp != VK_NULL_HANDLE) {
				vkDestroyRenderPass(g_Device, rp, nullptr);
				rp = VK_NULL_HANDLE;
			}
		});

		// Clear pools last
		g_CommandListPool.clear();
		g_TransientResourceGroupPool.clear();
		g_BufferViewPool.clear();
		g_BufferPool.clear();
		g_TexturePool.clear();
		g_ShaderPool.clear();
		g_LayoutPool.clear();
		g_PipelinePool.clear();
		g_RenderPassPool.clear();

		// clear all caches
		g_BufferViewCache.clear();

		freeResourceGroups();
	}

	void VKShutdownCommon() {
		auto& ctx = GetVulkanContext();
		if (ctx.device == VK_NULL_HANDLE) return;
		// Ensure device is idle before destroying resources
		vkDeviceWaitIdle(ctx.device);

		// Destroy per-frame upload buffers BEFORE destroying the allocator
		// This is critical - VMA will assert if allocator is destroyed with unfreed allocations
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			auto& frame = GetFrameContex(i);

			// Destroy upload buffer
			if (frame.upload.buffer != VK_NULL_HANDLE) {
				vmaDestroyBuffer(ctx.allocator, frame.upload.buffer, frame.upload.allocation);
				frame.upload.buffer = VK_NULL_HANDLE;
				frame.upload.allocation = VK_NULL_HANDLE;
				frame.upload.mappedPtr = nullptr;
			}

			// Destroy per-frame synchronization primitives
			if (frame.fence != VK_NULL_HANDLE) {
				vkDestroyFence(ctx.device, frame.fence, nullptr);
				frame.fence = VK_NULL_HANDLE;
			}
			if (frame.presentSemaphore != VK_NULL_HANDLE) {
				vkDestroySemaphore(ctx.device, frame.presentSemaphore, nullptr);
				frame.presentSemaphore = VK_NULL_HANDLE;
			}
			if (frame.renderSemaphore != VK_NULL_HANDLE) {
				vkDestroySemaphore(ctx.device, frame.renderSemaphore, nullptr);
				frame.renderSemaphore = VK_NULL_HANDLE;
			}
			if (frame.commandPool != VK_NULL_HANDLE) {
				vkDestroyCommandPool(ctx.device, frame.commandPool, nullptr);
				frame.commandPool = VK_NULL_HANDLE;
			}
		}

		// Destroy swapchain image semaphores
		for (auto& s : ctx.swapchainImageSync) {
			if (s.imageAvailableSemaphore != VK_NULL_HANDLE)
				vkDestroySemaphore(ctx.device, s.imageAvailableSemaphore, nullptr);
			if (s.renderFinishedSemaphore != VK_NULL_HANDLE)
				vkDestroySemaphore(ctx.device, s.renderFinishedSemaphore, nullptr);
			s.imageAvailableSemaphore = VK_NULL_HANDLE;
			s.renderFinishedSemaphore = VK_NULL_HANDLE;
		}

		// Now destroy VMA allocator (all VMA-managed resources must be freed first)
		if (ctx.allocator != VK_NULL_HANDLE) {
			// Log VMA allocation info before destroying allocator
			RENDERX_INFO("=== VMA Cleanup ===");
			RENDERX_INFO("Destroying VMA allocator and all managed resources");
			RENDERX_INFO("===================");

			vmaDestroyAllocator(ctx.allocator);
			ctx.allocator = VK_NULL_HANDLE;
			RENDERX_INFO("VMA allocator destroyed successfully");
		}

		// Destroy swapchain framebuffers
		for (auto fb : ctx.swapchainFramebuffers) {
			if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(ctx.device, fb, nullptr);
		}
		ctx.swapchainFramebuffers.clear();

		// Destroy image views
		for (auto iv : ctx.swapchainImageviews) {
			if (iv != VK_NULL_HANDLE) vkDestroyImageView(ctx.device, iv, nullptr);
		}
		ctx.swapchainImageviews.clear();

		// Destroy swapchain
		if (ctx.swapchain != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(ctx.device, ctx.swapchain, nullptr);
			ctx.swapchain = VK_NULL_HANDLE;
		}

		// Destroy surface
		if (ctx.surface != VK_NULL_HANDLE && ctx.instance != VK_NULL_HANDLE) {
			vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
			ctx.surface = VK_NULL_HANDLE;
		}

		// Finally destroy logical device
		if (ctx.device != VK_NULL_HANDLE) {
			vkDestroyDevice(ctx.device, nullptr);
			ctx.device = VK_NULL_HANDLE;
		}

		// Destroy instance
		if (ctx.instance != VK_NULL_HANDLE) {
			vkDestroyInstance(ctx.instance, nullptr);
			ctx.instance = VK_NULL_HANDLE;
		}
	}

} // namespace RxVK
} // namespace Rx