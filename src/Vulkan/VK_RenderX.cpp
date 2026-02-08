#include "VK_RenderX.h"
#include "VK_Common.h"
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

		ctx.instance = std::make_unique<VulkanInstance>(window);
		ctx.device = std::make_unique<VulkanDevice>(
			ctx.instance->getInstance(),
			ctx.instance->getSurface(),
			std::vector<const char*>(g_RequestedDeviceExtensions.begin(), g_RequestedDeviceExtensions.end()),
			std::vector<const char*>(g_RequestedValidationLayers.begin(), g_RequestedValidationLayers.end()));

		ctx.swapchain = std::make_unique<VulkanSwapchain>();

		ctx.graphicsQueue = std::make_unique<VulkanCommandQueue>(ctx.device->logical(), ctx.device->graphicsQueue(),
			ctx.device->graphicsFamily(), QueueType::GRAPHICS);

		ctx.computeQueue = std::make_unique<VulkanCommandQueue>(ctx.device->logical(), ctx.device->computeQueue(),
			ctx.device->computeFamily(), QueueType::COMPUTE);

		ctx.transferQueue = std::make_unique<VulkanCommandQueue>(ctx.device->logical(), ctx.device->transferQueue(),
			ctx.device->transferFamily(), QueueType::TRANSFER);

		ctx.descriptorPoolManager = std::make_unique<VulkanDescriptorPoolManager>(ctx);
		ctx.descriptorSetManager = std::make_unique<VulkanDescriptorManager>(ctx);
	}

	void VKShutdown() {
		RENDERX_WARN("Destroying all vulkan resource");
		VKShutdownCommon();
	}


} // namespace RxVK
} // namespace Rx
