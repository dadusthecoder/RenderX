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

	constexpr uint32_t MAX_FRAMES_IN_FLIGHT =  3 ;
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
		GLFWwindow* window = nullptr;
		VkInstance instance = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		uint32_t graphicsQueueFamilyIndex = 0;

		VkSurfaceKHR surface = VK_NULL_HANDLE;

		// Swapchain related resources
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

	extern std::unordered_map<uint32_t, VulkanBuffer> s_Buffers;
	extern std::unordered_map<uint32_t, VulkanShader> s_Shaders;
	extern std::unordered_map<uint32_t, VkRenderPass> s_RenderPasses;

	// ===================== CONTEXT ACCESS =====================

	VulkanContext& GetVulkanContext();
	FrameContex& GetCurrentFrameContex();

	// ===================== INITIALIZATION FUNCTIONS =====================

	bool InitInstance(VkInstance* outInstance);

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

	bool InitSwapchain(VulkanContext& ctx, GLFWwindow* window);

	void CreateSwapchainFramebuffers(RenderPassHandle renderPass);

	void InitFrameContex();

	// ===================== VULKAN HELPERS =====================

	VkRenderPass GetVulkanRenderPass(RenderPassHandle handle);

	// Format conversion helpers
	VkFormat ToVkFormat(DataFormat format);
	VkFormat ToVkTextureFormat(TextureFormat format);

	// Shader and pipeline helpers
	VkShaderStageFlagBits ToVkShaderStage(ShaderType type);
	VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveType type);
	VkCullModeFlags ToVkCullMode(CullMode mode);
	VkCompareOp ToVkCompareOp(CompareFunc func);
	VkPolygonMode ToVkPolygonMode(FillMode mode);

} // namespace RenderXVK
} // namespace RenderX