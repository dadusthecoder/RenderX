#include "RenderXVK.h"
#include "CommonVK.h"
#include "RenderX/DebugProfiler.h"

namespace Rx {
namespace RxVK {

	void VKInit(const Window& window) {
		PROFILE_FUNCTION();
	
		VulkanContext& ctx = GetVulkanContext();
		ctx.window = window.nativeHandle;

		{
			PROFILE_GPU_CALL("InitInstance");
			if (!InitInstance(window.extensionCount, window.instanceExtensions)) {
				RENDERX_ERROR("VKInit failed: InitInstance");
				return;
			}
		}

		if (!window.nativeHandle) {
			RENDERX_ERROR("VKInit failed: nativeHandle is null");
			return;
		}

		{
			PROFILE_GPU_CALL("CreateSurface");
			CreateSurface(window);
		}

		{
			PROFILE_GPU_CALL("PickPhysicalDevice");
			if (!PickPhysicalDevice(
					ctx.instance,
					ctx.surface,
					&ctx.physicalDevice,
					&ctx.graphicsQueueFamilyIndex)) {
				RENDERX_ERROR("VKInit failed: PickPhysicalDevice");
				return;
			}
		}
	
		{
			PROFILE_GPU_CALL("InitLogicalDevice");
			if (!InitLogicalDevice(
					ctx.physicalDevice,
					ctx.graphicsQueueFamilyIndex,
					&ctx.device,
					&ctx.graphicsQueue)) {
				RENDERX_ERROR("VKInit failed: InitLogicalDevice");
				return;
			}
		}

		{
			PROFILE_GPU_CALL("CreateSwapchain");
			if (!CreateSwapchain(ctx, window)) {
				RENDERX_ERROR("VKInit failed: CreateSwapchain");
				return;
			}
		}

		{
			PROFILE_GPU_CALL("InitFrameContext");
			InitFrameContext();
		}

		RENDERX_ASSERT_MSG(ctx.instance != VK_NULL_HANDLE, "Vulkan instance is invalid");
		RENDERX_ASSERT_MSG(ctx.surface != VK_NULL_HANDLE, "Vulkan surface is invalid");

		if (!ctx.window) {
			RENDERX_WARN(
				"ctx.window is null. "
				"Backend still relies on GLFW window elsewhere.");
		}

		RENDERX_ASSERT_MSG(ctx.physicalDevice != VK_NULL_HANDLE, "Physical device is invalid");
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "Logical device is invalid");

		RENDERX_INFO("Vulkan Init Completed Successfully");
        RENDERX_INFO("Running Test Functions");

        TempTest();
		return;
	}
	

} // namespace RxVK
} // namespace Rx
