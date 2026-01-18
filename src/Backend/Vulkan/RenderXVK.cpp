#include "RenderXVK.h"
#include "CommonVK.h"

namespace RenderX {
namespace RenderXVK {

	// ===================== INIT =====================
	void VKInit() {
		VulkanContext& ctx = GetVulkanContext();

		// GLFW WINDOW Temp setup
		if (!glfwInit()) {
			RENDERX_ERROR("Failed to initialize GLFW");
			return;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Vulkan only
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		ctx.window = glfwCreateWindow(1280, 720, "RenderX", nullptr, nullptr);
		if (!ctx.window) {
			RENDERX_ERROR("Failed to create GLFW window");
			return;
		}

		// VULKAN SETUP
		if (!InitInstance(&ctx.instance)) return;

		if (glfwCreateWindowSurface(
				ctx.instance,
				ctx.window,
				nullptr,
				&ctx.surface) != VK_SUCCESS)
			return;

		if (!PickPhysicalDevice(
				ctx.instance,
				ctx.surface,
				&ctx.physicalDevice,
				&ctx.graphicsQueueFamilyIndex))
			return;

		if (!InitLogicalDevice(
				ctx.physicalDevice,
				ctx.graphicsQueueFamilyIndex,
				&ctx.device,
				&ctx.graphicsQueue))
			return;

		if (!InitSwapchain(ctx, ctx.window))
			return;

		InitFrameContex();

		return;
	}

	bool VKShouldClose() {
		VulkanContext& ctx = GetVulkanContext();
		return glfwWindowShouldClose(ctx.window);
	}

} // namespace RenderXVK
} // namespace RenderX
