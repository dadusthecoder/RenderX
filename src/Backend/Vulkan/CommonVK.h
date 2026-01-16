
#pragma once
#include "RenderX/Log.h"
#include "ProLog/ProLog.h"
#include "RenderX/RenderXCommon.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <vector>
#include <limits>
#include <cstdint>
#include <cstring>
#include <string>

namespace RenderX {
namespace RenderXVK {

// Internal Vulkan backend helpers. These are NOT part of the public RenderX API.
// They are kept inline so they can be used privately from backend sources without
// exposing device/swapchain creation details to library users.

#define VK_CHECK(x)                                                                                      \
	do {                                                                                                 \
		VkResult err = x;                                                                                \
		if (err != VK_SUCCESS) {                                                                         \
			RENDERX_ERROR("[Vulkan] VkResult = {} at {}:{}", static_cast<int>(err), __FILE__, __LINE__); \
		}                                                                                                \
	} while (0)

	inline bool CheckVk(VkResult result, const char* message) {
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("[Vulkan] {} (VkResult = {})", message, static_cast<int>(result));
			return false;
		}
		return true;
	}
	struct VulkanContext {
		GLFWwindow* window = nullptr;
		VkInstance instance = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		uint32_t graphicsQueueFamilyIndex = 0;
		VkCommandPool graphicsCommandPool;

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkSwapchainKHR swapchain = VK_NULL_HANDLE;
		VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D swapchainExtent{ 0, 0 };
	};

	struct VulkanCommandList {
		VkCommandBuffer commandBuffer;
		bool isRecording;
	};

	struct VulkanBuffer {
		VkDeviceMemory memory;
		VkBuffer buffer;
		size_t size;
		VkDeviceSize offset;
	};

	struct VulkanShader {
		std::string entryPoint;
		ShaderType type;
		VkShaderModule shaderModule;
	};
	// storage for the global
	extern std::unordered_map<uint32_t, VulkanCommandList> s_CommandLists;
	extern std::unordered_map<uint32_t, VulkanBuffer> s_Buffers;
	extern std::unordered_map<uint32_t, VulkanShader> s_Shaders;

	VulkanContext& GetVulkanContext();

	bool CreateInstance(VkInstance* outInstance);
	bool PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
		VkPhysicalDevice* outPhysicalDevice, uint32_t* outGraphicsQueueFamily);
	bool CreateLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamily,
		VkDevice* outDevice, VkQueue* outGraphicsQueue);

	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
	bool CreateSwapchain(VulkanContext& ctx, GLFWwindow* window);

	void CreateCommandPool();

} // namespace  RenderXVK


} // namespace RenderX
