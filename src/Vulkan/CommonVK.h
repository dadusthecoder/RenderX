#pragma once
#include "RenderX/Log.h"
#include "ProLog/ProLog.h"
#include "RenderX/RenderXCommon.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <vector>
#include <array>
#include <optional>
#include <unordered_map>
#include <limits>
#include <random>
#include <cstdint>
#include <cstring>
#include <string>

namespace Rx {
namespace RxVK {

	// Internal Vulkan backend helpers. These are NOT part of the public RenderX API.
	// They are kept inline so they can be used privately from backend sources without
	// exposing device/swapchain creation details to library users.

	constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
	extern uint32_t g_CurrentFrame;

	inline const char* VkResultToString(VkResult result) {
		switch (result) {
		case 0: return "VK_SUCCESS";
		case 1: return "VK_NOT_READY";
		case 2: return "VK_TIMEOUT";
		case 3: return "VK_EVENT_SET";
		case 4: return "VK_EVENT_RESET";
		case 5: return "VK_INCOMPLETE";
		case -1: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case -2: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case -3: return "VK_ERROR_INITIALIZATION_FAILED";
		case -4: return "VK_ERROR_DEVICE_LOST";
		case -5: return "VK_ERROR_MEMORY_MAP_FAILED";
		case -6: return "VK_ERROR_LAYER_NOT_PRESENT";
		case -7: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case -8: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case -9: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case -10: return "VK_ERROR_TOO_MANY_OBJECTS";
		case -11: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case -12: return "VK_ERROR_FRAGMENTED_POOL";
		case -13: return "VK_ERROR_UNKNOWN";
		case -1000069000: return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case -1000072003: return "VK_ERROR_INVALID_EXTERNAL_HANDLE ";
		case -1000161000: return "VK_ERROR_FRAGMENTATION";
		case -1000257000: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case 1000297000: return "VK_PIPELINE_COMPILE_REQUIRED";
		case -1000174001: return "VK_ERROR_NOT_PERMITTED";
		case -1000000000: return "VK_ERROR_SURFACE_LOST_KHR";
		case -1000000001: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case 1000001003: return "VK_SUBOPTIMAL_KHR ";
		case -1000001004: return "VK_ERROR_OUT_OF_DATE_KHR";
		case -1000003001: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case -1000011001: return "VK_ERROR_VALIDATION_FAILED_EXT";
		case -1000012000: return "VK_ERROR_INVALID_SHADER_NV";
		case -1000023000: return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
		case -1000023001: return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
		case -1000023002: return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
		case -1000023003: return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
		case -1000023004: return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
		case -1000023005: return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
		case -1000158000: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case -1000255000: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case 1000268000: return "VK_THREAD_IDLE_KHR";
		case 1000268001: return "VK_THREAD_DONE_KHR";
		case 1000268002: return "VK_OPERATION_DEFERRED_KHR";
		case 1000268003: return "VK_OPERATION_NOT_DEFERRED_KHR";
		case -1000299000: return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
		case -1000338000: return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
		case 1000482000: return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
		case 1000483000: return "VK_PIPELINE_BINARY_MISSING_KHR";
		case -1000483000: return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
		case 0x7FFFFFFF: return "VK_RESULT_MAX_ENUM";
		default:
			return "Unknown VKResult";
		}
	}

	// Macro for checking Vulkan results with detailed error reporting
#define VK_CHECK(x)                                                                                      \
	do {                                                                                                 \
		VkResult err = x;                                                                                \
		if (err != VK_SUCCESS) {                                                                         \
			RENDERX_ERROR("[Vulkan] VkResult = {} at {}:{}", VkResultToString(err), __FILE__, __LINE__); \
		}                                                                                                \
	} while (0)

	// Inline function for checking Vulkan results with custom message
	inline bool
	CheckVk(VkResult result, const char* message) {
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("[Vulkan] {} (VkResult = {})", message, VkResultToString(result));
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
		VkDescriptorPool DescriptorPool;
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

	// struct VulkanBuffer {
	//	VkBuffer buffer = VK_NULL_HANDLE;
	//	VkDeviceMemory memory = VK_NULL_HANDLE;
	//	VkDeviceSize size = 0;
	//	void* mappedData = nullptr;
	//	bool isValid = false;

	//	VulkanBuffer() = default;
	//};

	/// Vulkan-specific texture resource
	struct VulkanTexture {
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkFormat format = VK_FORMAT_UNDEFINED;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t mipLevels = 1;
		bool isValid = false;

		VulkanTexture() = default;
	};

	struct VulkanShader {
		std::string entryPoint;
		ShaderStage type;
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

	// =====================  Vulkan Handle/Resource Pool=====================
	template <typename ResourceType, typename Tag>
	class ResourcePool {
	public:
		using ValueType = uint64_t;

		ResourcePool() {
			ResourceType invalidResource{};
			_My_resource.push_back(invalidResource);

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dest(1, 100);
			_My_key = dest(gen);
		}
		~ResourcePool() {
			_My_freelist.clear();
			_My_resource.clear();
		}

		Handle<Tag> allocate(ResourceType resource) {
			ValueType index;
			Handle<Tag> handle;
			if (_My_freelist.empty()) {
				index = static_cast<ValueType>(_My_resource.size());
				handle.id = index ^ _My_key;
				_My_resource.push_back(resource);
				return handle;
			}
			else {
				index = _My_freelist.back();
				_My_freelist.pop_back();
				_My_resource[index] = resource;
				handle.id = index ^ _My_key;
				return handle;
			}
		}

		bool IsAlive(const Handle<Tag>& handle) {
		}

		void free(Handle<Tag>& handle) {
			RENDERX_ASSERT_MSG(handle.IsValid(), "trying to free invalid handle");
			auto index = handle.id ^ _My_key;
			handle.id = 0;
			_My_resource[index] = ResourceType{};
			_My_freelist.push_back(index);
		}

		ResourceType& get(Handle<Tag> handle) {
			RENDERX_ASSERT_MSG(handle.IsValid(), "trying to get invalid handle");
			auto index = handle.id ^ _My_key;
			return _My_resource[index];
		}

		void clear() {
			_My_freelist.clear();
			_My_resource.clear();
			_My_generation.clear();
		}
	private:
		std::vector<ResourceType> _My_resource;
		std::vector<ValueType> _My_freelist;
		std::vector<ValueType> _My_generation;
		ValueType _My_key = 0;
	};

	// ===================== GLOBAL RESOURCE POOLS =====================

	// New ResourcePool instances for consistent resource management
	extern ResourcePool<VulkanBuffer, HandleType::Buffer> g_BufferPool;
	extern ResourcePool<VulkanShader, HandleType::Shader> g_ShaderPool;
	extern ResourcePool<VkPipeline, HandleType::Pipeline> g_PipelinePool;
	extern ResourcePool<VkRenderPass, HandleType::RenderPass> g_RenderPassPool;
	extern ResourcePool<VulkanTexture, HandleType::Texture> g_TexturePool;

	// ===================== CONTEXT ACCESS =====================

	VulkanContext& GetVulkanContext();
	FrameContex& GetCurrentFrameContex(uint32_t index);


	// Temp Testing Function (Executed Once Through Program)
	inline void TempTest() {
		VulkanTexture texture;
		auto texturehandle = g_TexturePool.allocate(texture);
		auto textureHandle1 = g_TexturePool.allocate(texture);
		g_TexturePool.free(texturehandle);
		auto texturehandle2 = g_TexturePool.allocate(texture);
		g_TexturePool.free(textureHandle1);
		auto textureHandle3 = g_TexturePool.allocate(texture);
		g_TexturePool.get(texturehandle2);
		g_TexturePool.get(textureHandle3);
	};

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
		Window window);
	bool CreateSwapchain(VulkanContext& ctx, Window window);
	void InitFrameContext();
	void CreateSurface(Window);

	// ===================== VULKAN HELPERS =====================

	VkRenderPass GetVulkanRenderPass(RenderPassHandle handle);

	// Format conversion helpers
	inline VkFormat ToVkFormat(DataFormat format) {
		switch (format) {
		case DataFormat::R8:
			return VK_FORMAT_R8_UNORM;
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


	inline VkShaderStageFlagBits ToVkShaderStage(ShaderStage type) {
		switch (type) {
		case ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
		case ShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderStage::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case ShaderStage::TessEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

		default:
			RENDERX_ERROR("Unknown ShaderStage: {}", static_cast<int>(type));
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
			RENDERX_ERROR("Unknown StoreOp");
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

	inline VkDescriptorType ToVkDescriptorType(ResourceType type) {
		switch (type) {
		case ResourceType::ConstantBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		case ResourceType::StorageBuffer:
		case ResourceType::RWStorageBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		case ResourceType::Texture_SRV:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

		case ResourceType::Texture_UAV:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		case ResourceType::Sampler:
			return VK_DESCRIPTOR_TYPE_SAMPLER;

		case ResourceType::CombinedTextureSampler:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		case ResourceType::AccelerationStructure:
			return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

		default:
			RENDERX_ERROR("Unknown ResourceType");
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}
	}

	inline VkShaderStageFlags ToVkShaderStageFlags(ShaderStage stages) {
		VkShaderStageFlags vkStages = 0;

		if (Has(stages, ShaderStage::Vertex))
			vkStages |= VK_SHADER_STAGE_VERTEX_BIT;

		if (Has(stages, ShaderStage::Fragment))
			vkStages |= VK_SHADER_STAGE_FRAGMENT_BIT;

		if (Has(stages, ShaderStage::Geometry))
			vkStages |= VK_SHADER_STAGE_GEOMETRY_BIT;

		if (Has(stages, ShaderStage::TessControl))
			vkStages |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

		if (Has(stages, ShaderStage::TessEvaluation))
			vkStages |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

		if (Has(stages, ShaderStage::Compute))
			vkStages |= VK_SHADER_STAGE_COMPUTE_BIT;

		return vkStages;
	}
} // namespace RxVK
} // namespace Rx