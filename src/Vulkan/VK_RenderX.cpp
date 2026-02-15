#include "VK_RenderX.h"
#include "VK_Common.h"
#include "RenderX/DebugProfiler.h"

namespace Rx {
namespace RxVK {

	void VKBackendInit(const InitDesc& window) {
		PROFILE_FUNCTION();

		RENDERX_ASSERT_MSG(window.nativeWindowHandle != nullptr, "VKInit: window.nativeHandle is null");
		RENDERX_ASSERT_MSG(window.extensionCount >= 0, "VKInit: extensionCount is negative");
		if (window.extensionCount > 0) {
			RENDERX_ASSERT_MSG(window.instanceExtensions != nullptr, "VKInit: instanceExtensions is null but extensionCount > 0");
		}

		VulkanContext& ctx = GetVulkanContext();
		ctx.window = window.nativeWindowHandle;
		ctx.instance = new VulkanInstance(window);
		ctx.device = new VulkanDevice(ctx.instance->getInstance(), ctx.instance->getSurface(), std::vector<const char*>(g_RequestedDeviceExtensions.begin(), g_RequestedDeviceExtensions.end()), std::vector<const char*>(g_RequestedValidationLayers.begin(), g_RequestedValidationLayers.end()));
		ctx.swapchain = new VulkanSwapchain();
		ctx.graphicsQueue = new VulkanCommandQueue(ctx.device->logical(), ctx.device->graphicsQueue(), ctx.device->graphicsFamily(), QueueType::GRAPHICS);
		ctx.computeQueue = new VulkanCommandQueue(ctx.device->logical(), ctx.device->computeQueue(), ctx.device->computeFamily(), QueueType::COMPUTE);
		ctx.transferQueue = new VulkanCommandQueue(ctx.device->logical(), ctx.device->transferQueue(), ctx.device->transferFamily(), QueueType::TRANSFER);
		ctx.allocator = new VulkanAllocator(ctx.instance->getInstance(), ctx.device->physical(), ctx.device->logical());
		ctx.descriptorPoolManager = new VulkanDescriptorPoolManager(ctx);
		ctx.descriptorSetManager = new VulkanDescriptorManager(ctx);
		ctx.stagingAllocator = new VulkanStagingAllocator(ctx);
		ctx.immediateUploader = new VulkanImmediateUploader(ctx);
		ctx.deferredUploader = new VulkanDeferredUploader(ctx);
	}

	void VKBackendShutdown() {
		RENDERX_INFO("Shutting down Vulkan backend resources");
		VKShutdownCommon();
	}


} // namespace RxVK
} // namespace Rx
