#include "CommonVK.h"


namespace RenderX {
namespace RenderXVK {

	//  GLOBAL STORAGE 
	static VulkanContext s_Context;
	VulkanContext& GetVulkanContext() {
		return s_Context;
	}

	// INSTANCE 
	bool CreateInstance(VkInstance* outInstance) {
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
		createInfo.enabledLayerCount = 0;

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
					*outPhysicalDevice = device;
					*outGraphicsQueueFamily = i;
					return true;
				}
			}
		}
		return false;
	}

	// ===================== LOGICAL DEVICE =====================

	bool CreateLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamily,
		VkDevice* outDevice, VkQueue* outGraphicsQueue) {
        PROFILE_FUNCTION();
		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = graphicsQueueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.enabledExtensionCount = 1;
		createInfo.ppEnabledExtensionNames = deviceExtensions;

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

	bool CreateSwapchain(VulkanContext& ctx, GLFWwindow* window) {
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
		return true;
	}

	// ===================== COMMAND POOL =====================

	void CreateCommandPool() {
        PROFILE_FUNCTION();
		VulkanContext& ctx = GetVulkanContext();

		VkCommandPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.queueFamilyIndex = ctx.graphicsQueueFamilyIndex;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		vkCreateCommandPool(ctx.device, &info, nullptr, &ctx.graphicsCommandPool);
	}

} // namespace RenderXVK
} // namespace RenderX
