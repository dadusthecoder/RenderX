#pragma once

#include "RenderX/RenderXTypes.h"
#include "RenderX/Log.h"
#include "ProLog/ProLog.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <limits>
#include <cstdint>
#include <cstring>
#include <string>

// Internal Vulkan backend helpers. These are NOT part of the public RenderX API.
// They are kept inline so they can be used privately from backend sources without
// exposing device/swapchain creation details to library users.
namespace RenderX {

namespace RenderXVK {

	struct VulkanContext {
		VkInstance instance = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		uint32_t graphicsQueueFamilyIndex = 0;

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkSwapchainKHR swapchain = VK_NULL_HANDLE;
		VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D swapchainExtent{ 0, 0 };
	};

	inline VulkanContext& GetVulkanContext()
	{
		static VulkanContext s_Context{};
		return s_Context;
	}

	inline bool CheckVk(VkResult result, const char* message)
	{
		if (result != VK_SUCCESS) {
			RENDERX_ERROR("[Vulkan] {} (VkResult = {})", message, static_cast<int>(result));
			return false;
		}
		return true;
	}

	inline bool CreateInstance(VkInstance* outInstance)
	{
		PROFILE_FUNCTION();

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
		createInfo.enabledLayerCount = 0; // validation layers can be added later

		VkResult result = vkCreateInstance(&createInfo, nullptr, outInstance);
		return CheckVk(result, "Failed to create Vulkan instance");
	}

	inline bool PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
		VkPhysicalDevice* outPhysicalDevice, uint32_t* outGraphicsQueueFamily)
	{
		PROFILE_FUNCTION();

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			RENDERX_ERROR("[Vulkan] No physical devices with Vulkan support found.");
			return false;
		}

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

				bool graphics = (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
				if (graphics && presentSupport) {
					*outPhysicalDevice = device;
					*outGraphicsQueueFamily = i;
					return true;
				}
			}
		}

		RENDERX_ERROR("[Vulkan] Failed to find suitable graphics+present queue family.");
		return false;
	}

	inline bool CreateLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamily,
		VkDevice* outDevice, VkQueue* outGraphicsQueue)
	{
		PROFILE_FUNCTION();

		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = graphicsQueueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		// Swapchain extension is required for presentation.
		const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.enabledExtensionCount = 1;
		createInfo.ppEnabledExtensionNames = deviceExtensions;

		VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, outDevice);
		if (!CheckVk(result, "Failed to create Vulkan logical device"))
			return false;

		vkGetDeviceQueue(*outDevice, graphicsQueueFamily, 0, outGraphicsQueue);
		return true;
	}

	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	inline SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
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

	inline VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& format : availableFormats) {
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
				format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return format;
			}
		}
		return availableFormats.empty() ? VkSurfaceFormatKHR{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
			: availableFormats[0];
	}

	inline VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		// Prefer mailbox if available; otherwise FIFO (always supported).
		for (auto mode : availablePresentModes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	inline VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}

		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		actualExtent.width = std::max(capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}

	inline bool CreateSwapchain(VulkanContext& ctx, GLFWwindow* window)
	{
		PROFILE_FUNCTION();

		SwapchainSupportDetails support = QuerySwapchainSupport(ctx.physicalDevice, ctx.surface);
		if (support.formats.empty() || support.presentModes.empty()) {
			RENDERX_ERROR("[Vulkan] Swapchain support is incomplete (no formats/present modes).");
			return false;
		}

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
		VkPresentModeKHR presentMode = ChoosePresentMode(support.presentModes);
		VkExtent2D extent = ChooseSwapExtent(support.capabilities, window);

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
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.preTransform = support.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VkResult result = vkCreateSwapchainKHR(ctx.device, &createInfo, nullptr, &ctx.swapchain);
		if (!CheckVk(result, "Failed to create Vulkan swapchain"))
			return false;

		ctx.swapchainImageFormat = surfaceFormat.format;
		ctx.swapchainExtent = extent;
		return true;
	}

	// High-level initialization entry point for the Vulkan backend.
	// This is intended to be called only from backend code (e.g., when wiring
	// RenderXAPI::Vulkan into the dispatch table), not by engine users.
	inline bool VKInit(GLFWwindow* window)
	{
		PROFILE_FUNCTION();

		VulkanContext& ctx = GetVulkanContext();

		if (!CreateInstance(&ctx.instance))
			return false;

		VkResult surfaceResult = glfwCreateWindowSurface(ctx.instance, window, nullptr, &ctx.surface);
		if (!CheckVk(surfaceResult, "Failed to create Vulkan surface from GLFW window"))
			return false;

		if (!PickPhysicalDevice(ctx.instance, ctx.surface, &ctx.physicalDevice, &ctx.graphicsQueueFamilyIndex))
			return false;

		if (!CreateLogicalDevice(ctx.physicalDevice, ctx.graphicsQueueFamilyIndex, &ctx.device, &ctx.graphicsQueue))
			return false;

		if (!CreateSwapchain(ctx, window))
			return false;

		RENDERX_INFO("[Vulkan] Backend initialized successfully.");
		return true;
	}

} // namespace RenderXVK

} // namespace RenderX
