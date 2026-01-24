#include "RenderXVK.h"
#include "CommonVK.h"

namespace RenderX {
namespace RenderXVK {

	// ===================== INIT =====================
	void VKInit(const Window& window) {
		RENDERX_INFO("==== Vulkan Init Started ====");

		VulkanContext& ctx = GetVulkanContext();
		ctx.window = window.nativeHandle;


		if (!InitInstance(window.extensionCount, window.instanceExtensions)) {
			RENDERX_ERROR("VKInit failed: InitInstance");
			return;
		}

		CreateSurface(window);
		// -------------------- Surface --------------------
		if (!window.nativeHandle) {
			RENDERX_ERROR("VKInit failed: nativeHandle is null");
			return;
		}

		if (!PickPhysicalDevice(
				ctx.instance,
				ctx.surface,
				&ctx.physicalDevice,
				&ctx.graphicsQueueFamilyIndex)) {
			RENDERX_ERROR("VKInit failed: PickPhysicalDevice");
			return;
		}
		RENDERX_INFO(
			"Physical device selected (graphics queue family = {})",
			ctx.graphicsQueueFamilyIndex);

		if (!InitLogicalDevice(
				ctx.physicalDevice,
				ctx.graphicsQueueFamilyIndex,
				&ctx.device,
				&ctx.graphicsQueue)) {
			RENDERX_ERROR("VKInit failed: InitLogicalDevice");
			return;
		}

		if (!CreateSwapchain(ctx, window)) {
			RENDERX_ERROR("VKInit failed: CreateSwapchain");
			return;
		}

		InitFrameContex();

		RENDERX_ASSERT_MSG(ctx.instance != VK_NULL_HANDLE, "Vulkan instance is invalid");
		RENDERX_ASSERT_MSG(ctx.surface != VK_NULL_HANDLE, "Vulkan surface is invalid");

		if (!ctx.window) {
			RENDERX_WARN(
				"ctx.window is null. "
				"Backend still relies on GLFW window elsewhere.");
		}

		RENDERX_ASSERT_MSG(ctx.physicalDevice != VK_NULL_HANDLE, "Physical device is invalid");
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "Logical device is invalid");

		RENDERX_INFO("==== Vulkan Init Completed Successfully ====");
		return;
	}
	

} // namespace RenderXVK
} // namespace RenderX
