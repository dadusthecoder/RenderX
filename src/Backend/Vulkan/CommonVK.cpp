#include "CommonVK.h"


#ifndef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

namespace RenderX {
namespace RenderXVK {

	static FrameContex g_Frames[MAX_FRAMES_IN_FLIGHT];
	uint32_t g_CurrentFrame = 0;

	VulkanContext& GetVulkanContext() {
		static VulkanContext g_Context;
		return g_Context;
	}

	FrameContex& GetCurrentFrameContex() {
		return g_Frames[g_CurrentFrame];
	}

	std::vector<VkDynamicState> g_DynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	std::vector<const char*> g_RequestedValidationLayers{
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char*> g_RequestedDeviceExtensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	// -------------------- helpers to query layers/extensions --------------------
	void GetEnabledValidationLayers(std::vector<const char*>& out) {
		uint32_t count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);
		if (count == 0)
			return;

		std::vector<VkLayerProperties> props(count);
		vkEnumerateInstanceLayerProperties(&count, props.data());

		for (auto* req : g_RequestedValidationLayers) {
			for (const auto& p : props)
				if (strcmp(req, p.layerName) == 0)
					out.push_back(req);
		}
	}

	void GetEnabledExtensionsVk(VkPhysicalDevice device, std::vector<const char*>& out) {
		uint32_t count = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
		if (count == 0)
			return;

		std::vector<VkExtensionProperties> props(count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, props.data());

		for (auto* req : g_RequestedDeviceExtensions) {
			for (const auto& p : props)
				if (strcmp(req, p.extensionName) == 0)
					out.push_back(req);
		}
	}

	// INSTANCE
	bool InitInstance(VkInstance* outInstance) {
		PROFILE_FUNCTION();
		std::vector<const char*> validationLayers;
		if (enableValidationLayers)
			GetEnabledValidationLayers(validationLayers);

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "RenderX";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "RenderX";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_1;

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		createInfo.enabledLayerCount = 0;

		if (!validationLayers.empty()) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		return vkCreateInstance(&createInfo, nullptr, outInstance) == VK_SUCCESS;
	}

	// ===================== PHYSICAL DEVICE =====================

	bool PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
		VkPhysicalDevice* outPhysicalDevice, uint32_t* outGraphicsQueueFamily) {
		PROFILE_FUNCTION();
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (auto device : devices) {
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			for (uint32_t i = 0; i < queueFamilyCount; ++i) {
				VkBool32 presentSupport = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

				if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
					VkPhysicalDeviceProperties2 deviceProps{};
					deviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
					vkGetPhysicalDeviceProperties2(device, &deviceProps);

					RENDERX_INFO("Device: {}", deviceProps.properties.deviceName);
					*outPhysicalDevice = device;
					*outGraphicsQueueFamily = i;
					return true;
				}
			}
		}
		return false;
	}

	// ===================== LOGICAL DEVICE =====================

	bool InitLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamily,
		VkDevice* outDevice, VkQueue* outGraphicsQueue) {
		PROFILE_FUNCTION();
		auto ctx = GetVulkanContext();
		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = graphicsQueueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		std::vector<const char*> deviceExtensions;
		GetEnabledExtensionsVk(ctx.physicalDevice, deviceExtensions);

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.empty() ? nullptr : deviceExtensions.data();

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, outDevice) != VK_SUCCESS)
			return false;

		vkGetDeviceQueue(*outDevice, graphicsQueueFamily, 0, outGraphicsQueue);
		return true;
	}

	// ===================== SWAPCHAIN =====================

	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
		PROFILE_FUNCTION();
		SwapchainSupportDetails details{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
		details.formats.resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, details.formats.data());

		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
		details.presentModes.resize(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, details.presentModes.data());

		return details;
	}

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
		PROFILE_FUNCTION();
		for (const auto& f : formats) {
			if (f.format == VK_FORMAT_B8G8R8A8_UNORM)
				return f;
		}
		return formats[0];
	}

	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
		PROFILE_FUNCTION();
		for (auto m : modes) {
			if (m == VK_PRESENT_MODE_MAILBOX_KHR)
				return m;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window) {
		PROFILE_FUNCTION();
		if (caps.currentExtent.width != UINT32_MAX)
			return caps.currentExtent;

		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		return { (uint32_t)w, (uint32_t)h };
	}

	bool InitSwapchain(VulkanContext& ctx, GLFWwindow* window) {
		PROFILE_FUNCTION();
		auto support = QuerySwapchainSupport(ctx.physicalDevice, ctx.surface);
		auto surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
		auto presentMode = ChoosePresentMode(support.presentModes);
		auto extent = ChooseSwapExtent(support.capabilities, window);

		VkSwapchainCreateInfoKHR info{};
		info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		info.surface = ctx.surface;
		info.minImageCount = support.capabilities.minImageCount + 1;
		info.imageFormat = surfaceFormat.format;
		info.imageColorSpace = surfaceFormat.colorSpace;
		info.imageExtent = extent;
		info.imageArrayLayers = 1;
		info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.preTransform = support.capabilities.currentTransform;
		info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		info.presentMode = presentMode;
		info.clipped = VK_TRUE;

		if (vkCreateSwapchainKHR(ctx.device, &info, nullptr, &ctx.swapchain) != VK_SUCCESS)
			return false;

		ctx.swapchainImageFormat = surfaceFormat.format;
		ctx.swapchainExtent = extent;

		uint32_t imageCount;
		vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &imageCount, nullptr);
		ctx.swapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &imageCount, ctx.swapchainImages.data());

		for (auto& image : ctx.swapchainImages) {
			VkImageViewCreateInfo ivci{};
			ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ivci.image = image;
			ivci.format = ctx.swapchainImageFormat;
			ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ivci.subresourceRange.baseArrayLayer = 0;
			ivci.subresourceRange.layerCount = 1;
			ivci.subresourceRange.baseMipLevel = 0;
			ivci.subresourceRange.levelCount = 1;

			VkImageView iv;
			VK_CHECK(vkCreateImageView(ctx.device, &ivci, nullptr, &iv));
			ctx.swapchainImageviews.push_back(iv);
		}

		return true;
	}

	void CreateSwapchainFramebuffers(RenderPassHandle renderPass) {
		VulkanContext& ctx = GetVulkanContext();
		VkRenderPass& vkPass = s_RenderPasses[renderPass.id];

		ctx.swapchainFramebuffers.resize(ctx.swapchainImageviews.size());

		for (size_t i = 0; i < ctx.swapchainImageviews.size(); i++) {
			VkImageView attachments[] = {
				ctx.swapchainImageviews[i]
			};

			VkFramebufferCreateInfo ci{};
			ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			ci.renderPass = vkPass;
			ci.attachmentCount = 1;
			ci.pAttachments = attachments;
			ci.width = ctx.swapchainExtent.width;
			ci.height = ctx.swapchainExtent.height;
			ci.layers = 1;

			VK_CHECK(vkCreateFramebuffer(
				ctx.device,
				&ci,
				nullptr,
				&ctx.swapchainFramebuffers[i]));
		}
	}


	void InitFrameContex() {
		auto& ctx = GetVulkanContext();

		for (auto& frame : g_Frames) {
			VkFenceCreateInfo fci{};
			fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			VkSemaphoreCreateInfo sci{};
			sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VK_CHECK(vkCreateSemaphore(ctx.device, &sci, nullptr, &frame.presentSemaphore));
			VK_CHECK(vkCreateSemaphore(ctx.device, &sci, nullptr, &frame.renderSemaphore));
			VK_CHECK(vkCreateFence(ctx.device, &fci, nullptr, &frame.fence));

			VkCommandPoolCreateInfo cpci{};
			cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			cpci.queueFamilyIndex = ctx.graphicsQueueFamilyIndex;
			VK_CHECK(vkCreateCommandPool(ctx.device, &cpci, nullptr, &frame.commandPool));
		}
	}

	// vulkan helpers

	VkRenderPass GetVulkanRenderPass(RenderPassHandle handle) {
		auto it = s_RenderPasses.find(handle.id);
		if (it == s_RenderPasses.end())
			return VK_NULL_HANDLE;
		return it->second;
	}

	VkFormat ToVkFormat(DataFormat format) {
		switch (format) {
		case DataFormat::R8:
			return VK_FORMAT_R8_UNORM;

		case DataFormat::RG8:
			return VK_FORMAT_R8G8_UNORM;

		case DataFormat::RGBA8:
			return VK_FORMAT_R8G8B8A8_UNORM;

		// NOTE: Vulkan has NO RGB8 format for vertex / textures
		case DataFormat::RGB8:
			RENDERX_ERROR("RGB8 is not supported in Vulkan. Use RGBA8 instead.");
			return VK_FORMAT_UNDEFINED;

		case DataFormat::R16F:
			return VK_FORMAT_R16_SFLOAT;

		case DataFormat::RG16F:
			return VK_FORMAT_R16G16_SFLOAT;

		case DataFormat::RGBA16F:
			return VK_FORMAT_R16G16B16A16_SFLOAT;

		// No native RGB16F
		case DataFormat::RGB16F:
			RENDERX_ERROR("RGB16F is not supported in Vulkan. Use RGBA16F instead.");
			return VK_FORMAT_UNDEFINED;

		case DataFormat::R32F:
			return VK_FORMAT_R32_SFLOAT;

		case DataFormat::RG32F:
			return VK_FORMAT_R32G32_SFLOAT;

		case DataFormat::RGBA32F:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

		// Vulkan supports this ONLY for vertex buffers (not textures)
		case DataFormat::RGB32F:
			return VK_FORMAT_R32G32B32_SFLOAT;

		default:
			RENDERX_ERROR("Unknown DataFormat");
			return VK_FORMAT_UNDEFINED;
		}
	}


	VkFormat ToVkTextureFormat(TextureFormat format) {
		switch (format) {
		case TextureFormat::R8: return VK_FORMAT_R8_UNORM;
		case TextureFormat::RG8: return VK_FORMAT_R8G8_UNORM;
		case TextureFormat::RGB8: return VK_FORMAT_R8G8B8_UNORM;

		case TextureFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
		case TextureFormat::R16F: return VK_FORMAT_R16_SFLOAT;
		case TextureFormat::RG16F: return VK_FORMAT_R16G16_SFLOAT;
		case TextureFormat::RGB16F: return VK_FORMAT_R16G16B16_SFLOAT;
		case TextureFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case TextureFormat::R32F: return VK_FORMAT_R32_SFLOAT;
		case TextureFormat::RG32F: return VK_FORMAT_R32G32_SFLOAT;
		case TextureFormat::RGB32F: return VK_FORMAT_R32G32B32_SFLOAT;
		case TextureFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case TextureFormat::Depth16: return VK_FORMAT_D16_UNORM;
		case TextureFormat::Depth24: return VK_FORMAT_D24_UNORM_S8_UINT;
		case TextureFormat::Depth32F: return VK_FORMAT_D32_SFLOAT;
		case TextureFormat::Depth24Stencil8: return VK_FORMAT_D24_UNORM_S8_UINT;
		case TextureFormat::Depth32FStencil8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case TextureFormat::BGRA8: return VK_FORMAT_B8G8R8A8_UNORM;
		// Compressed formats
		case TextureFormat::BC1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case TextureFormat::BC2: return VK_FORMAT_BC2_UNORM_BLOCK;
		case TextureFormat::BC3: return VK_FORMAT_BC3_UNORM_BLOCK;
		case TextureFormat::BC4: return VK_FORMAT_BC4_UNORM_BLOCK;
		case TextureFormat::BC5: return VK_FORMAT_BC5_UNORM_BLOCK;
		default:
			RENDERX_ERROR("Unknown TextureFormat");
			return VK_FORMAT_UNDEFINED;
		}
	}

	VkShaderStageFlagBits ToVkShaderStage(ShaderType type) {
		switch (type) {
		case ShaderType::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderType::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderType::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
		case ShaderType::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderType::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case ShaderType::TessEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		default:
			RENDERX_ERROR("Unknown ShaderType");
			return VK_SHADER_STAGE_VERTEX_BIT;
		}
	}

	VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveType type) {
		switch (type) {
		case PrimitiveType::Points: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case PrimitiveType::Lines: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case PrimitiveType::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case PrimitiveType::Triangles: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case PrimitiveType::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case PrimitiveType::TriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		default:
			RENDERX_ERROR("Unknown PrimitiveType");
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}

	VkCullModeFlags ToVkCullMode(CullMode mode) {
		switch (mode) {
		case CullMode::None: return VK_CULL_MODE_NONE;
		case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
		case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
		case CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
		default:
			RENDERX_ERROR("Unknown CullMode");
			return VK_CULL_MODE_BACK_BIT;
		}
	}

	VkCompareOp ToVkCompareOp(CompareFunc func) {
		switch (func) {
		case CompareFunc::Never: return VK_COMPARE_OP_NEVER;
		case CompareFunc::Less: return VK_COMPARE_OP_LESS;
		case CompareFunc::Equal: return VK_COMPARE_OP_EQUAL;
		case CompareFunc::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CompareFunc::Greater: return VK_COMPARE_OP_GREATER;
		case CompareFunc::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
		case CompareFunc::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case CompareFunc::Always: return VK_COMPARE_OP_ALWAYS;
		default:
			RENDERX_ERROR("Unknown CompareFunc");
			return VK_COMPARE_OP_ALWAYS;
		}
	}

	VkPolygonMode ToVkPolygonMode(FillMode mode) {
		switch (mode) {
		case RenderX::FillMode::Solid:
			return VK_POLYGON_MODE_FILL;
			break;
		case RenderX::FillMode::Wireframe:
			return VK_POLYGON_MODE_LINE;
			break;
		case RenderX::FillMode::Point:
			return VK_POLYGON_MODE_POINT;
			break;
		default:
			return VK_POLYGON_MODE_FILL;
			break;
		}
	}

} // namespace RenderXVK
} // namespace RenderX
