#include "RenderXVK.h"
#include "CommonVK.h"
#include "RenderX/DebugProfiler.h"

namespace Rx {
namespace RxVK {

	void VKInit(const Window& window) {
		PROFILE_FUNCTION();

		RENDERX_ASSERT_MSG(window.nativeHandle != nullptr, "VKInit: window.nativeHandle is null");
		RENDERX_ASSERT_MSG(window.extensionCount >= 0, "VKInit: extensionCount is negative");
		if (window.extensionCount > 0) {
			RENDERX_ASSERT_MSG(window.instanceExtensions != nullptr, "VKInit: instanceExtensions is null but extensionCount > 0");
		}

		VulkanContext& ctx = GetVulkanContext();
		ctx.window = window.nativeHandle;
		MAX_FRAMES_IN_FLIGHT = window.maxFramesInFlight;
		RENDERX_ASSERT_MSG(MAX_FRAMES_IN_FLIGHT > 0, "VKInit: maxFramesInFlight must be > 0");

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
			RENDERX_ASSERT_MSG(ctx.surface != VK_NULL_HANDLE, "VKInit: surface creation failed");
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
			RENDERX_ASSERT_MSG(ctx.physicalDevice != VK_NULL_HANDLE, "VKInit: physicalDevice is null");
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
			RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "VKInit: device is null");
			RENDERX_ASSERT_MSG(ctx.graphicsQueue != VK_NULL_HANDLE, "VKInit: graphics queue is null");
		}

		{
			PROFILE_GPU_CALL("InitVulkanMemoryAllocator");
			if (!InitVulkanMemoryAllocator(ctx.instance, ctx.physicalDevice, ctx.device)) {
				RENDERX_ERROR("VKInit failed: InitVulkanMemoryAllocator");
				return;
			}
			RENDERX_ASSERT_MSG(ctx.allocator != VK_NULL_HANDLE, "VKInit: allocator is null");
		}

		{
			PROFILE_GPU_CALL("CreateSwapchain");
			if (!CreateSwapchain(ctx, window)) {
				RENDERX_ERROR("VKInit failed: CreateSwapchain");
				return;
			}
			RENDERX_ASSERT_MSG(ctx.swapchain != VK_NULL_HANDLE, "VKInit: swapchain is null");
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

		CreatePersistentDescriptorPool();


		RENDERX_ASSERT_MSG(ctx.physicalDevice != VK_NULL_HANDLE, "Physical device is invalid");
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "Logical device is invalid");
		RENDERX_ASSERT_MSG(ctx.allocator != VK_NULL_HANDLE, "VMA allocator is invalid");

		RENDERX_INFO("Vulkan Init Completed Successfully");
		return;
	}

	void VKShutdown() {
		// Ensure Vulkan resources from different modules are cleaned up in proper order
		RENDERX_WARN("Destroying all vulkan resource");
		freeAllVulkanResources();
		// Common resources (frames, swapchain, device, instance)
		VKShutdownCommon();
	}


} // namespace RxVK
} // namespace Rx
