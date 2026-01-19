#include "CommonVK.h"
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
	
	std::vector<const char*> GetEnabledValidationLayers() {
		if constexpr (!enableValidationLayers) {
			return {};
		}

		uint32_t count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);
		if (count == 0) {
			RENDERX_WARN("No validation layers available");
			return {};
		}

		std::vector<VkLayerProperties> props(count);
		vkEnumerateInstanceLayerProperties(&count, props.data());

		std::vector<const char*> enabledLayers;
		enabledLayers.reserve(g_RequestedValidationLayers.size());

		for (const char* requested : g_RequestedValidationLayers) {
			auto it = std::find_if(props.begin(), props.end(),
				[requested](const VkLayerProperties& p) {
					return strcmp(requested, p.layerName) == 0;
				});
			
			if (it != props.end()) {
				enabledLayers.push_back(requested);
			} else {
				RENDERX_WARN("Validation layer not available: {}", requested);
			}
		}

		return enabledLayers;
	}

	std::vector<const char*> GetEnabledExtensionsVk(VkPhysicalDevice device) {
		uint32_t count = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
		if (count == 0) {
			RENDERX_ERROR("No device extensions available");
			return {};
		}

		std::vector<VkExtensionProperties> props(count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, props.data());

		std::vector<const char*> enabledExtensions;
		enabledExtensions.reserve(g_RequestedDeviceExtensions.size());

		for (const char* requested : g_RequestedDeviceExtensions) {
			auto it = std::find_if(props.begin(), props.end(),
				[requested](const VkExtensionProperties& p) {
					return strcmp(requested, p.extensionName) == 0;
				});
			
			if (it != props.end()) {
				enabledExtensions.push_back(requested);
			} else {
				RENDERX_ERROR("Required device extension not available: {}", requested);
			}
		}

		return enabledExtensions;
	}

	// ===================== INSTANCE =====================

	bool InitInstance(VkInstance* outInstance) {
		PROFILE_FUNCTION();
		
		if (!outInstance) {
			RENDERX_ERROR("Null output instance pointer");
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

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		if (!glfwExtensions) {
			RENDERX_ERROR("Failed to get required GLFW extensions");
			return false;
		}

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.empty() ? nullptr : validationLayers.data();

		const VkResult result = vkCreateInstance(&createInfo, nullptr, outInstance);
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("Failed to create Vulkan instance: {}", static_cast<int>(result));
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
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; ++i) {
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}
		}

		return indices;
	}

	bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
		const auto indices = FindQueueFamilies(device, surface);
		if (!indices.isComplete()) {
			return false;
		}

		// Check for required extensions
		const auto extensions = GetEnabledExtensionsVk(device);
		if (extensions.size() != g_RequestedDeviceExtensions.size()) {
			return false;
		}

		// Verify swapchain support
		const auto swapchainSupport = QuerySwapchainSupport(device, surface);
		if (swapchainSupport.formats.empty() || swapchainSupport.presentModes.empty()) {
			return false;
		}

		return true;
	}

	bool PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
		VkPhysicalDevice* outPhysicalDevice, uint32_t* outGraphicsQueueFamily) {
		PROFILE_FUNCTION();

		if (!outPhysicalDevice || !outGraphicsQueueFamily) {
			RENDERX_ERROR("Null output pointers");
			return false;
		}

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			RENDERX_ERROR("No Vulkan-capable devices found");
			return false;
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

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
		RENDERX_INFO("Selected device: {}", props.deviceName);

		const auto indices = FindQueueFamilies(selectedDevice, surface);
		*outPhysicalDevice = selectedDevice;
		*outGraphicsQueueFamily = indices.graphicsFamily.value();

		return true;
	}

	// ===================== LOGICAL DEVICE =====================

	bool InitLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamily,
		VkDevice* outDevice, VkQueue* outGraphicsQueue) {
		PROFILE_FUNCTION();

		if (!outDevice || !outGraphicsQueue) {
			RENDERX_ERROR("Null output pointers");
			return false;
		}

		constexpr float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = graphicsQueueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

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
			RENDERX_ERROR("Failed to create logical device: {}", static_cast<int>(result));
			return false;
		}

		vkGetDeviceQueue(*outDevice, graphicsQueueFamily, 0, outGraphicsQueue);
		RENDERX_INFO("Logical device created successfully");
		return true;
	}

	// ===================== SWAPCHAIN =====================

	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
		PROFILE_FUNCTION();
		
		SwapchainSupportDetails details{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
		PROFILE_FUNCTION();
		
		// Prefer SRGB if available for better color accuracy
		for (const auto& format : formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && 
				format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return format;
			}
		}

		// Fallback to UNORM
		for (const auto& format : formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM) {
				return format;
			}
		}

		return formats[0];
	}

	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
		PROFILE_FUNCTION();
		
		// Prefer mailbox for low latency triple buffering
		for (const auto mode : modes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return mode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;

		// FIFO is guaranteed to be available
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window) {
		PROFILE_FUNCTION();
		
		if (caps.currentExtent.width != UINT32_MAX) {
			return caps.currentExtent;
		}

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, 
			caps.minImageExtent.width, caps.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height,
			caps.minImageExtent.height, caps.maxImageExtent.height);

		return actualExtent;
	}

	bool InitSwapchain(VulkanContext& ctx, GLFWwindow* window) {
		PROFILE_FUNCTION();

		if (!window) {
			RENDERX_ERROR("Null window pointer");
			return false;
		}

		const auto support = QuerySwapchainSupport(ctx.physicalDevice, ctx.surface);
		if (support.formats.empty() || support.presentModes.empty()) {
			RENDERX_ERROR("Insufficient swapchain support");
			return false;
		}

		const auto surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
		const auto presentMode = ChoosePresentMode(support.presentModes);
		const auto extent = ChooseSwapExtent(support.capabilities, window);

		// Request one more than minimum for better performance
		uint32_t imageCount = support.capabilities.minImageCount + 1;
		if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
			imageCount = support.capabilities.maxImageCount;
		}

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
			RENDERX_ERROR("Failed to create swapchain: {}", static_cast<int>(result));
			return false;
		}

		ctx.swapchainImageFormat = surfaceFormat.format;
		ctx.swapchainExtent = extent;

		// Retrieve swapchain images
		uint32_t swapchainImageCount = 0;
		vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &swapchainImageCount, nullptr);
		ctx.swapchainImages.resize(swapchainImageCount);
		vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &swapchainImageCount, ctx.swapchainImages.data());

		// Create image views
		ctx.swapchainImageviews.reserve(ctx.swapchainImages.size());
		for (const auto& image : ctx.swapchainImages) {
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = ctx.swapchainImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			VK_CHECK(vkCreateImageView(ctx.device, &viewInfo, nullptr, &imageView));
			ctx.swapchainImageviews.push_back(imageView);
		}

		RENDERX_INFO("Swapchain created: {}x{}, {} images", extent.width, extent.height, swapchainImageCount);
		return true;
	}

	void CreateSwapchainFramebuffers(RenderPassHandle renderPass) {
		VulkanContext& ctx = GetVulkanContext();
		
		auto it = s_RenderPasses.find(renderPass.id);
		if (it == s_RenderPasses.end()) {
			RENDERX_ERROR("Invalid render pass handle");
			return;
		}
		const VkRenderPass vkPass = it->second;

		ctx.swapchainFramebuffers.resize(ctx.swapchainImageviews.size());

		for (size_t i = 0; i < ctx.swapchainImageviews.size(); i++) {
			const VkImageView attachments[] = { ctx.swapchainImageviews[i] };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = vkPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = ctx.swapchainExtent.width;
			framebufferInfo.height = ctx.swapchainExtent.height;
			framebufferInfo.layers = 1;

			VK_CHECK(vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, 
				&ctx.swapchainFramebuffers[i]));
		}
	}

	void InitFrameContex() {
		auto& ctx = GetVulkanContext();

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = ctx.graphicsQueueFamilyIndex;
    
		ctx.swapchainImageSync.resize(ctx.swapchainImageviews.size());
		
		for (auto& iSync : ctx.swapchainImageSync ){

			VK_CHECK(vkCreateSemaphore(ctx.device , &semaphoreInfo , nullptr , &iSync.imageAvailableSemaphore));
			VK_CHECK(vkCreateSemaphore(ctx.device , &semaphoreInfo , nullptr , &iSync.renderFinishedSemaphore));

		}
		for (auto& frame : g_Frames) {
			VK_CHECK(vkCreateFence(ctx.device, &fenceInfo, nullptr, &frame.fence));
			VK_CHECK(vkCreateSemaphore(ctx.device , &semaphoreInfo , nullptr , &frame.presentSemaphore));
			VK_CHECK(vkCreateSemaphore(ctx.device , &semaphoreInfo , nullptr , &frame.renderSemaphore));
			VK_CHECK(vkCreateCommandPool(ctx.device, &poolInfo, nullptr, &frame.commandPool));
		}

		RENDERX_INFO("Frame contexts initialized: {} frames", MAX_FRAMES_IN_FLIGHT);
	}

	// ===================== VULKAN HELPERS =====================

	VkRenderPass GetVulkanRenderPass(RenderPassHandle handle) {
		const auto it = s_RenderPasses.find(handle.id);
		return (it != s_RenderPasses.end()) ? it->second : VK_NULL_HANDLE;
	}

	VkFormat ToVkFormat(DataFormat format) {
		switch (format) {
		case DataFormat::R8:         return VK_FORMAT_R8_UNORM;
		case DataFormat::RG8:        return VK_FORMAT_R8G8_UNORM;
		case DataFormat::RGBA8:      return VK_FORMAT_R8G8B8A8_UNORM;
		case DataFormat::R16F:       return VK_FORMAT_R16_SFLOAT;
		case DataFormat::RG16F:      return VK_FORMAT_R16G16_SFLOAT;
		case DataFormat::RGBA16F:    return VK_FORMAT_R16G16B16A16_SFLOAT;
		case DataFormat::R32F:       return VK_FORMAT_R32_SFLOAT;
		case DataFormat::RG32F:      return VK_FORMAT_R32G32_SFLOAT;
		case DataFormat::RGBA32F:    return VK_FORMAT_R32G32B32A32_SFLOAT;
		case DataFormat::RGB32F:     return VK_FORMAT_R32G32B32_SFLOAT; // Vertex only

		// Unsupported formats
		case DataFormat::RGB8:
			RENDERX_ERROR("RGB8 not supported in Vulkan. Use RGBA8");
			return VK_FORMAT_UNDEFINED;
		case DataFormat::RGB16F:
			RENDERX_ERROR("RGB16F not supported in Vulkan. Use RGBA16F");
			return VK_FORMAT_UNDEFINED;

		default:
			RENDERX_ERROR("Unknown DataFormat: {}", static_cast<int>(format));
			return VK_FORMAT_UNDEFINED;
		}
	}

VkFormat ToVkTextureFormat(TextureFormat format)
{
    switch (format)
    {
        case TextureFormat::R8:               return VK_FORMAT_R8_UNORM;
        case TextureFormat::R16F:             return VK_FORMAT_R16_SFLOAT;
        case TextureFormat::R32F:             return VK_FORMAT_R32_SFLOAT;

        case TextureFormat::RGBA8:            return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::RGBA8_SRGB:       return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureFormat::RGBA16F:          return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureFormat::RGBA32F:          return VK_FORMAT_R32G32B32A32_SFLOAT;

        case TextureFormat::BGRA8:            return VK_FORMAT_B8G8R8A8_UNORM;
        case TextureFormat::BGRA8_SRGB:       return VK_FORMAT_B8G8R8A8_SRGB;

        case TextureFormat::Depth32F:         return VK_FORMAT_D32_SFLOAT;
        case TextureFormat::Depth24Stencil8:  return VK_FORMAT_D24_UNORM_S8_UINT;

        case TextureFormat::BC1:              return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case TextureFormat::BC1_SRGB:         return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        case TextureFormat::BC3:              return VK_FORMAT_BC3_UNORM_BLOCK;
        case TextureFormat::BC3_SRGB:         return VK_FORMAT_BC3_SRGB_BLOCK;

        default:
            RENDERX_ERROR("Unknown TextureFormat: {}", static_cast<int>(format));
            return VK_FORMAT_UNDEFINED;
    }
}


	VkShaderStageFlagBits ToVkShaderStage(ShaderType type) {
		switch (type) {
		case ShaderType::Vertex:         return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderType::Fragment:       return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderType::Compute:        return VK_SHADER_STAGE_COMPUTE_BIT;
		case ShaderType::Geometry:       return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderType::TessControl:    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case ShaderType::TessEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

		default:
			RENDERX_ERROR("Unknown ShaderType: {}", static_cast<int>(type));
			return VK_SHADER_STAGE_VERTEX_BIT;
		}
	}

	VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveType type) {
		switch (type) {
		case PrimitiveType::Points:        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case PrimitiveType::Lines:         return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case PrimitiveType::LineStrip:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case PrimitiveType::Triangles:     return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case PrimitiveType::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case PrimitiveType::TriangleFan:   return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;

		default:
			RENDERX_ERROR("Unknown PrimitiveType: {}", static_cast<int>(type));
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}

	VkCullModeFlags ToVkCullMode(CullMode mode) {
		switch (mode) {
		case CullMode::None:         return VK_CULL_MODE_NONE;
		case CullMode::Front:        return VK_CULL_MODE_FRONT_BIT;
		case CullMode::Back:         return VK_CULL_MODE_BACK_BIT;
		case CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;

		default:
			RENDERX_ERROR("Unknown CullMode: {}", static_cast<int>(mode));
			return VK_CULL_MODE_BACK_BIT;
		}
	}

	VkCompareOp ToVkCompareOp(CompareFunc func) {
		switch (func) {
		case CompareFunc::Never:        return VK_COMPARE_OP_NEVER;
		case CompareFunc::Less:         return VK_COMPARE_OP_LESS;
		case CompareFunc::Equal:        return VK_COMPARE_OP_EQUAL;
		case CompareFunc::LessEqual:    return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CompareFunc::Greater:      return VK_COMPARE_OP_GREATER;
		case CompareFunc::NotEqual:     return VK_COMPARE_OP_NOT_EQUAL;
		case CompareFunc::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case CompareFunc::Always:       return VK_COMPARE_OP_ALWAYS;

		default:
			RENDERX_ERROR("Unknown CompareFunc: {}", static_cast<int>(func));
			return VK_COMPARE_OP_ALWAYS;
		}
	}

	VkPolygonMode ToVkPolygonMode(FillMode mode) {
		switch (mode) {
		case FillMode::Solid:     return VK_POLYGON_MODE_FILL;
		case FillMode::Wireframe: return VK_POLYGON_MODE_LINE;
		case FillMode::Point:     return VK_POLYGON_MODE_POINT;

		default:
			RENDERX_ERROR("Unknown FillMode: {}", static_cast<int>(mode));
			return VK_POLYGON_MODE_FILL;
		}
	}

} // namespace RenderXVK
} // namespace RenderX