#include "RenderX/Log.h"
#include "RenderX/DebugProfiler.h"
#include "VK_RenderX.h"
#include "VK_Common.h"

namespace Rx {

namespace RxVK {

	ResourcePool<VulkanCommandList, CommandList> g_CommandListPool;

	void VKBegin(uint32_t frameIndex) {
		PROFILE_COMMAND_BUFFER("VKBegin");
		RENDERX_ASSERT_MSG(frameIndex < MAX_FRAMES_IN_FLIGHT, "VKBegin: frameIndex out of bounds");

		g_CurrentFrame = frameIndex;
		auto& frame = GetFrameContex(g_CurrentFrame);
		auto& ctx = GetVulkanContext();

		RENDERX_ASSERT_MSG(frame.fence != VK_NULL_HANDLE, "VKBegin: frame fence is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "VKBegin: device is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(ctx.swapchain != VK_NULL_HANDLE, "VKBegin: swapchain is VK_NULL_HANDLE");

		{
			PROFILE_SYNC("vkWaitForFences");
			VkResult result = vkWaitForFences(ctx.device, 1, &frame.fence, VK_TRUE, UINT64_MAX);
			if (!CheckVk(result, "VKBegin: Failed to wait for fence")) {
				return;
			}
		}
		{
			PROFILE_SYNC("vkResetFences");
			VkResult result = vkResetFences(ctx.device, 1, &frame.fence);
			if (!CheckVk(result, "VKBegin: Failed to reset fence")) {
				return;
			}
		}

		// get the next Image
		{
			PROFILE_SWAPCHAIN("vkAcquireNextImageKHR");
			VkResult result = vkAcquireNextImageKHR(ctx.device, ctx.swapchain,
				UINT64_MAX, frame.presentSemaphore, VK_NULL_HANDLE, &frame.swapchainImageIndex);
			if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
				CheckVk(result, "VKBegin: Failed to acquire swapchain image");
				return;
			}
			RENDERX_ASSERT_MSG(frame.swapchainImageIndex < ctx.swapchainImages.size(),
				"VKBegin: swapchain image index out of bounds");
		}

		// Reset
		{
			RENDERX_ASSERT_MSG(frame.commandPool != VK_NULL_HANDLE, "VKBegin: command pool is VK_NULL_HANDLE");
			RENDERX_ASSERT_MSG(frame.DescriptorPool != VK_NULL_HANDLE, "VKBegin: descriptor pool is VK_NULL_HANDLE");

			// reseting commadpool dose not free the commandbuffer memory
			VkResult result = vkResetCommandPool(ctx.device, frame.commandPool, 0);
			if (!CheckVk(result, "VKBegin: Failed to reset command pool")) {
				return;
			}
			result = vkResetDescriptorPool(ctx.device, frame.DescriptorPool, 0);
			if (!CheckVk(result, "VKBegin: Failed to reset descriptor pool")) {
				return;
			}
			g_TransientResourceGroupPool.clear();
		}
	}

	void VKEnd(uint32_t frameIndex) {
		PROFILE_COMMAND_BUFFER("VKEnd");
		RENDERX_ASSERT_MSG(frameIndex < MAX_FRAMES_IN_FLIGHT, "VKEnd: frameIndex out of bounds");

		auto& frame = GetFrameContex(frameIndex);
		auto& ctx = GetVulkanContext();

		RENDERX_ASSERT_MSG(frame.swapchainImageIndex < ctx.swapchainImageSync.size(),
			"VKEnd: swapchain image index out of bounds");
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "VKEnd: device is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(ctx.swapchain != VK_NULL_HANDLE, "VKEnd: swapchain is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(ctx.graphicsQueue != VK_NULL_HANDLE, "VKEnd: graphics queue is VK_NULL_HANDLE");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &ctx.swapchainImageSync[frame.swapchainImageIndex].renderFinishedSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &ctx.swapchain;
		presentInfo.pImageIndices = &frame.swapchainImageIndex;

		{
			PROFILE_SWAPCHAIN("vkQueuePresentKHR");
			auto res = vkQueuePresentKHR(ctx.graphicsQueue, &presentInfo);

			if (res == VK_ERROR_OUT_OF_DATE_KHR) {
				RENDERX_WARN("VKEnd: Swapchain is out of date, recreation needed");
				// RecreateSwapchain
				return;
			}
			else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
				CheckVk(res, "VKEnd: Failed to present swapchain image");
			}
		}
	}

	void VKCmdOpen(CommandList& cmdList) {
		PROFILE_COMMAND_BUFFER("VKCmdOpen");

		RENDERX_ASSERT_MSG(cmdList.IsValid(), "VKCmdOpen: Invalid Command List");
		auto* vkCmdList = g_CommandListPool.get(cmdList);

		RENDERX_ASSERT_MSG(vkCmdList != nullptr, "VKCmdOpen: retrieved null command list");
		RENDERX_ASSERT_MSG(vkCmdList->cmdBuffer != VK_NULL_HANDLE, "VKCmdOpen: command buffer is VK_NULL_HANDLE");

		vkCmdList->isOpen = true;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VkResult result = vkBeginCommandBuffer(vkCmdList->cmdBuffer, &beginInfo);
		if (!CheckVk(result, "VKCmdOpen: Failed to begin command buffer")) {
			vkCmdList->isOpen = false;
		}
	}

	void VKCmdClose(CommandList& cmdList) {
		PROFILE_COMMAND_BUFFER("VKCmdClose");
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "VKCmdClose: Invalid CommandList");
		auto* vkCmdList = g_CommandListPool.get(cmdList);
		RENDERX_ASSERT_MSG(vkCmdList != nullptr, "VKCmdClose: retrieved null command list");
		RENDERX_ASSERT_MSG(vkCmdList->isOpen, "VKCmdClose: To close a CommandList it must be created and opened first");
		RENDERX_ASSERT_MSG(vkCmdList->cmdBuffer != VK_NULL_HANDLE, "VKCmdClose: command buffer is VK_NULL_HANDLE");

		VkResult result = vkEndCommandBuffer(vkCmdList->cmdBuffer);
		if (!CheckVk(result, "VKCmdClose: Failed to end command buffer")) {
			return;
		}
		vkCmdList->isOpen = false;
	}

	void VKCmdDraw(CommandList& cmdList, uint32_t vertexCount, uint32_t instanceCount,
		uint32_t firstVertex, uint32_t firstInstance) {
		PROFILE_GPU_CALL_WITH_SIZE("VKCmdDraw", vertexCount * instanceCount);
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "VKCmdDraw: Invalid CommandList");
		RENDERX_ASSERT_MSG(vertexCount > 0, "VKCmdDraw: vertex count is zero");
		RENDERX_ASSERT_MSG(instanceCount > 0, "VKCmdDraw: instance count is zero");

		auto* vkCmdList = g_CommandListPool.get(cmdList);
		RENDERX_ASSERT_MSG(vkCmdList != nullptr, "VKCmdDraw: retrieved null command list");
		RENDERX_ASSERT_MSG(vkCmdList->isOpen, "VKCmdDraw: CommandList is not in the open state");
		RENDERX_ASSERT_MSG(vkCmdList->cmdBuffer != VK_NULL_HANDLE, "VKCmdDraw: command buffer is VK_NULL_HANDLE");

		vkCmdDraw(vkCmdList->cmdBuffer,
			vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VKCmdDrawIndexed(CommandList& cmdList,
		uint32_t indexCount, int32_t vertexOffset,
		uint32_t instanceCount, uint32_t firstIndex, uint32_t firstInstance) {
		PROFILE_GPU_CALL_WITH_SIZE("VKCmdDrawIndexed", indexCount * instanceCount);
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "VKCmdDrawIndexed: Invalid CommandList");
		RENDERX_ASSERT_MSG(indexCount > 0, "VKCmdDrawIndexed: index count is zero");
		RENDERX_ASSERT_MSG(instanceCount > 0, "VKCmdDrawIndexed: instance count is zero");

		auto* vkCmdList = g_CommandListPool.get(cmdList);
		RENDERX_ASSERT_MSG(vkCmdList != nullptr, "VKCmdDrawIndexed: retrieved null command list");
		RENDERX_ASSERT_MSG(vkCmdList->isOpen, "VKCmdDrawIndexed: CommandList is not in the open state");
		RENDERX_ASSERT_MSG(vkCmdList->cmdBuffer != VK_NULL_HANDLE, "VKCmdDrawIndexed: command buffer is VK_NULL_HANDLE");

		vkCmdDrawIndexed(vkCmdList->cmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	CommandList VKCreateCommandList(uint32_t frameIndex) {
		PROFILE_COMMAND_BUFFER("VKCreateCommandList");
		RENDERX_ASSERT_MSG(frameIndex < MAX_FRAMES_IN_FLIGHT, "VKCreateCommandList: frameIndex out of bounds");

		VulkanContext& ctx = GetVulkanContext();
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "VKCreateCommandList: device is VK_NULL_HANDLE");

		auto& frame = GetFrameContex(frameIndex);
		RENDERX_ASSERT_MSG(frame.commandPool != VK_NULL_HANDLE, "VKCreateCommandList: command pool is VK_NULL_HANDLE");

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = frame.commandPool;
		allocInfo.commandBufferCount = 1;
		VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
		VkResult result = vkAllocateCommandBuffers(ctx.device, &allocInfo, &cmdBuffer);
		if (!CheckVk(result, "VKCreateCommandList: Failed to allocate command buffer")) {
			return CommandList();
		}
		RENDERX_ASSERT_MSG(cmdBuffer != VK_NULL_HANDLE, "VKCreateCommandList: allocated command buffer is null");

		VulkanCommandList vkCmdList{};
		vkCmdList.cmdBuffer = cmdBuffer;
		return g_CommandListPool.allocate(vkCmdList);
	}

	void VKDestroyCommandList(CommandList& cmdList, uint32_t frameIndex) {
		// Free the underlying Vulkan command buffer and mark slot as null
		if (!cmdList.IsValid()) return;

		RENDERX_ASSERT_MSG(frameIndex < MAX_FRAMES_IN_FLIGHT, "VKDestroyCommandList: frameIndex out of bounds");

		auto& frame = GetFrameContex(frameIndex);
		auto& ctx = GetVulkanContext();

		RENDERX_ASSERT_MSG(frame.commandPool != VK_NULL_HANDLE, "VKDestroyCommandList: command pool is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "VKDestroyCommandList: device is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(frame.fence != VK_NULL_HANDLE, "VKDestroyCommandList: fence is VK_NULL_HANDLE");

		auto* vkCmdList = g_CommandListPool.get(cmdList);
		RENDERX_ASSERT_MSG(vkCmdList != nullptr, "VKDestroyCommandList: retrieved null command list");

		if (vkCmdList->cmdBuffer != VK_NULL_HANDLE) {
			VkResult result = vkWaitForFences(ctx.device, 1, &frame.fence, VK_TRUE, UINT64_MAX);
			if (!CheckVk(result, "VKDestroyCommandList: Failed to wait for fence")) {
				RENDERX_WARN("VKDestroyCommandList: Fence wait failed, continuing with cleanup");
			}
			vkFreeCommandBuffers(ctx.device, frame.commandPool, 1, &vkCmdList->cmdBuffer);
			g_CommandListPool.free(cmdList);
		}
		else {
			RENDERX_WARN("VKDestroyCommandList: command buffer already null");
		}
	}

	void VKExecuteCommandList(CommandList& cmdList) {
		PROFILE_COMMAND_BUFFER("VKExecuteCommandList");
		auto& frame = GetFrameContex(g_CurrentFrame);
		VulkanContext& ctx = GetVulkanContext();
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "VKExecuteCommandList: Invalid CommandList");
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "VKExecuteCommandList: device is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(ctx.graphicsQueue != VK_NULL_HANDLE, "VKExecuteCommandList: graphics queue is VK_NULL_HANDLE");

		auto* vkCmdList = g_CommandListPool.get(cmdList);
		RENDERX_ASSERT_MSG(vkCmdList != nullptr, "VKExecuteCommandList: retrieved null command list");
		RENDERX_ASSERT_MSG(!vkCmdList->isOpen, "VKExecuteCommandList: CommandList must be closed before execution");
		RENDERX_ASSERT_MSG(vkCmdList->cmdBuffer != VK_NULL_HANDLE, "VKExecuteCommandList: command buffer is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(frame.fence != VK_NULL_HANDLE, "VKExecuteCommandList: fence is VK_NULL_HANDLE");

		VkSubmitInfo submitinfo{};
		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitinfo.pWaitDstStageMask = &waitStage;
		submitinfo.commandBufferCount = 1;
		submitinfo.pCommandBuffers = &vkCmdList->cmdBuffer;
		submitinfo.signalSemaphoreCount = 1;
		submitinfo.pSignalSemaphores = &ctx.swapchainImageSync[frame.swapchainImageIndex].renderFinishedSemaphore;
		submitinfo.waitSemaphoreCount = 1;
		submitinfo.pWaitSemaphores = &frame.presentSemaphore;

		{
			PROFILE_SYNC("vkQueueSubmit");
			VkResult submitResult = vkQueueSubmit(ctx.graphicsQueue, 1, &submitinfo, frame.fence);
			if (submitResult != VK_SUCCESS) {
				RENDERX_ERROR("vkQueueSubmit failed with result: {}", VkResultToString(submitResult));
				return;
			}
		}
	}


} // namespace RxVK

} // namespace RxRENDERX_ASSERT_MSG