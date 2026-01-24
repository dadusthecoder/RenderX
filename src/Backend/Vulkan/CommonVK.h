#pragma once
#include "RenderX/Log.h"
#include "ProLog/ProLog.h"
#include "RenderX/RenderXCommon.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <vector>
#include <array>
#include <optional>
#include <unordered_map>
#include <limits>
#include <cstdint>
#include <cstring>
#include <string>

namespace RenderX {
namespace RenderXVK {

	// Internal Vulkan backend helpers. These are NOT part of the public RenderX API.
	// They are kept inline so they can be used privately from backend sources without
	// exposing device/swapchain creation details to library users.

	constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
	extern uint32_t g_CurrentFrame;

// Macro for checking Vulkan results with detailed error reporting
#define VK_CHECK(x)                                                                                      \
	do {                                                                                                 \
		VkResult err = x;                                                                                \
		if (err != VK_SUCCESS) {                                                                         \
			RENDERX_ERROR("[Vulkan] VkResult = {} at {}:{}", static_cast<int>(err), __FILE__, __LINE__); \
		}                                                                                                \
	} while (0)

	// Inline function for checking Vulkan results with custom message
	inline bool CheckVk(VkResult result, const char* message) {
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("[Vulkan] {} (VkResult = {})", message, static_cast<int>(result));
			return false;
		}
		return true;
	}

	// ===================== STRUCTURES =====================

	struct FrameContex {
		VkCommandPool commandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> commandBuffers;

		// Synchronization primitives
		VkSemaphore presentSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderSemaphore = VK_NULL_HANDLE;
		VkFence fence = VK_NULL_HANDLE;

		// Current swapchain image index for this frame
		uint32_t swapchainImageIndex = 0;
	};

	struct SwapchainImageSync {
		VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
	};

	struct VulkanCommandList {
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		bool isRecording = false;
	};

	struct VulkanBuffer {
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkBuffer buffer = VK_NULL_HANDLE;
		uint32_t bindingCount = 0;
		size_t size = 0;
	};

	struct VulkanShader {
		std::string entryPoint;
		ShaderType type;
		VkShaderModule shaderModule = VK_NULL_HANDLE;
	};

	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct VulkanContext {
		void* window = nullptr;
		VkInstance instance = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		uint32_t graphicsQueueFamilyIndex = 0;

		VkSurfaceKHR surface = VK_NULL_HANDLE;

		// Swapchain related resources
		RenderPassHandle swapchainRenderPassHandle;
		VkRenderPass swapchainRenderPass = VK_NULL_HANDLE; // Created from RenderPassDesc
		std::vector<VkFramebuffer> swapchainFramebuffers;
		std::vector<SwapchainImageSync> swapchainImageSync;

		VkSwapchainKHR swapchain = VK_NULL_HANDLE;
		VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D swapchainExtent{ 0, 0 };
		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageviews;
	};

	// ===================== GLOBAL RESOURCE MAPS =====================

	extern std::unordered_map<uint32_t, VulkanBuffer> g_Buffers;
	extern std::unordered_map<uint32_t, VulkanShader> g_Shaders;
	extern std::unordered_map<uint32_t, VkRenderPass> g_RenderPasses;

	// ===================== CONTEXT ACCESS =====================

	VulkanContext& GetVulkanContext();
	FrameContex& GetCurrentFrameContex();

	// ===================== INITIALIZATION FUNCTIONS =====================

	bool InitInstance(uint32_t extCount, const char** extentions);

	bool PickPhysicalDevice(
		VkInstance instance,
		VkSurfaceKHR surface,
		VkPhysicalDevice* outPhysicalDevice,
		uint32_t* outGraphicsQueueFamily);

	bool InitLogicalDevice(
		VkPhysicalDevice physicalDevice,
		uint32_t graphicsQueueFamily,
		VkDevice* outDevice,
		VkQueue* outGraphicsQueue);

	// ===================== SWAPCHAIN FUNCTIONS =====================

	SwapchainSupportDetails QuerySwapchainSupport(
		VkPhysicalDevice device,
		VkSurfaceKHR surface);

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR ChoosePresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D ChooseSwapExtent(
		const VkSurfaceCapabilitiesKHR& capabilities,
		GLFWwindow* window);

	bool CreateSwapchain(VulkanContext& ctx, Window window);

	void CreateSwapchainFramebuffers(RenderPassHandle renderPass);

	void InitFrameContex();
	void CreateSurface(Window);

	// ===================== VULKAN HELPERS =====================

	VkRenderPass GetVulkanRenderPass(RenderPassHandle handle);

	// Format conversion helpers
	inline VkFormat ToVkFormat(DataFormat format) {
		switch (format) {
		case DataFormat::R8: return VK_FORMAT_R8_UNORM;
		case DataFormat::RG8: return VK_FORMAT_R8G8_UNORM;
		case DataFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
		case DataFormat::R16F: return VK_FORMAT_R16_SFLOAT;
		case DataFormat::RG16F: return VK_FORMAT_R16G16_SFLOAT;
		case DataFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case DataFormat::R32F: return VK_FORMAT_R32_SFLOAT;
		case DataFormat::RG32F: return VK_FORMAT_R32G32_SFLOAT;
		case DataFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case DataFormat::RGB32F:
			return VK_FORMAT_R32G32B32_SFLOAT; // Vertex only

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

	inline VkFormat ToVkTextureFormat(TextureFormat format) {
		switch (format) {
		case TextureFormat::R8: return VK_FORMAT_R8_UNORM;
		case TextureFormat::R16F: return VK_FORMAT_R16_SFLOAT;
		case TextureFormat::R32F: return VK_FORMAT_R32_SFLOAT;

		case TextureFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
		case TextureFormat::RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
		case TextureFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case TextureFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;

		case TextureFormat::BGRA8: return VK_FORMAT_B8G8R8A8_UNORM;
		case TextureFormat::BGRA8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;

		case TextureFormat::Depth32F: return VK_FORMAT_D32_SFLOAT;
		case TextureFormat::Depth24Stencil8: return VK_FORMAT_D24_UNORM_S8_UINT;

		case TextureFormat::BC1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case TextureFormat::BC1_SRGB: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case TextureFormat::BC3: return VK_FORMAT_BC3_UNORM_BLOCK;
		case TextureFormat::BC3_SRGB: return VK_FORMAT_BC3_SRGB_BLOCK;

		default:
			RENDERX_ERROR("Unknown TextureFormat: {}", static_cast<int>(format));
			return VK_FORMAT_UNDEFINED;
		}
	}


	inline VkShaderStageFlagBits ToVkShaderStage(ShaderType type) {
		switch (type) {
		case ShaderType::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderType::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderType::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
		case ShaderType::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderType::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case ShaderType::TessEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

		default:
			RENDERX_ERROR("Unknown ShaderType: {}", static_cast<int>(type));
			return VK_SHADER_STAGE_VERTEX_BIT;
		}
	}

	inline VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveType type) {
		switch (type) {
		case PrimitiveType::Points: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case PrimitiveType::Lines: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case PrimitiveType::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case PrimitiveType::Triangles: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case PrimitiveType::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case PrimitiveType::TriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;

		default:
			RENDERX_ERROR("Unknown PrimitiveType: {}", static_cast<int>(type));
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}

	inline VkCullModeFlags ToVkCullMode(CullMode mode) {
		switch (mode) {
		case CullMode::None: return VK_CULL_MODE_NONE;
		case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
		case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
		case CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;

		default:
			RENDERX_ERROR("Unknown CullMode: {}", static_cast<int>(mode));
			return VK_CULL_MODE_BACK_BIT;
		}
	}

	inline VkCompareOp ToVkCompareOp(CompareFunc func) {
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
			RENDERX_ERROR("Unknown CompareFunc: {}", static_cast<int>(func));
			return VK_COMPARE_OP_ALWAYS;
		}
	}

	inline VkPolygonMode ToVkPolygonMode(FillMode mode) {
		switch (mode) {
		case FillMode::Solid: return VK_POLYGON_MODE_FILL;
		case FillMode::Wireframe: return VK_POLYGON_MODE_LINE;
		case FillMode::Point: return VK_POLYGON_MODE_POINT;

		default:
			RENDERX_ERROR("Unknown FillMode: {}", static_cast<int>(mode));
			return VK_POLYGON_MODE_FILL;
		}
	}
	//
	inline VkBlendFactor ToVkBlendFactor(BlendFunc func) {
		switch (func) {
		case BlendFunc::Zero: return VK_BLEND_FACTOR_ZERO;
		case BlendFunc::One: return VK_BLEND_FACTOR_ONE;
		case BlendFunc::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendFunc::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendFunc::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
		case BlendFunc::OneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendFunc::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendFunc::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BlendFunc::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
		case BlendFunc::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case BlendFunc::ConstantColor: return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case BlendFunc::OneMinusConstantColor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		default:
			RENDERX_ERROR("Unknown BlendFunc");
			return VK_BLEND_FACTOR_ONE;
		}
	}

	inline VkBlendOp ToVkBlendOp(BlendOp op) {
		switch (op) {
		case BlendOp::Add: return VK_BLEND_OP_ADD;
		case BlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
		case BlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
		case BlendOp::Min: return VK_BLEND_OP_MIN;
		case BlendOp::Max: return VK_BLEND_OP_MAX;
		default:
			RENDERX_ERROR("Unknown BlendOp");
			return VK_BLEND_OP_ADD;
		}
	}

	inline VkAttachmentLoadOp ToVkAttachmentLoadOp(LoadOp op) {
		switch (op) {
		case LoadOp::Load:
			return VK_ATTACHMENT_LOAD_OP_LOAD;

		case LoadOp::Clear:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;

		case LoadOp::DontCare:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;

		default:
			RENDERX_ERROR("Unknown LoadOp");
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		}
	}

	inline VkAttachmentStoreOp ToVkAttachmentStoreOp(StoreOp op) {
		switch (op) {
		case StoreOp::Store:
			return VK_ATTACHMENT_STORE_OP_STORE;

		case StoreOp::DontCare:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;

		default:
			RENDERX_ERROR("Unknown LoadOp");
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
	}

	inline VkImageLayout ToVkLayout(ResourceState state) {
		switch (state) {
		case ResourceState::Undefined:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case ResourceState::RenderTarget:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ResourceState::DepthWrite:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ResourceState::ShaderRead:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ResourceState::Present:
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		default:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}
} // namespace RenderXVK
} // namespace RenderX