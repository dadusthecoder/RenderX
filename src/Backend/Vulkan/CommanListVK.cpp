#include "RenderX/Log.h"
#include "RenderXVK.h"
#include "CommonVK.h"

namespace RenderX {

namespace RenderXVK {

	void VKBegin() {
		auto& frame = GetCurrentFrameContex();
		auto& ctx = GetVulkanContext();

		VK_CHECK(vkWaitForFences(ctx.device, 1, &frame.fence, VK_TRUE, UINT64_MAX));
		VK_CHECK(vkResetFences(ctx.device, 1, &frame.fence));

		// get the next Image
		VK_CHECK(vkAcquireNextImageKHR(ctx.device, ctx.swapchain,
			UINT64_MAX, frame.presentSemaphore, VK_NULL_HANDLE, &frame.swapchainImageIndex));

		// Reset command pool (NOW it is safe)
		VK_CHECK(vkResetCommandPool(ctx.device, frame.commandPool, 0));

		// Clear frame-local command buffers
		frame.commandBuffers.clear();
		frame.commandBuffers.push_back(VkCommandBuffer{});
	}

	void VKEnd() {
		auto& frame = GetCurrentFrameContex();
		auto& ctx = GetVulkanContext();
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &frame.renderSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &ctx.swapchain;
		presentInfo.pImageIndices = &frame.swapchainImageIndex;
		auto res = vkQueuePresentKHR(ctx.graphicsQueue, &presentInfo);

		if (res == VK_ERROR_OUT_OF_DATE_KHR) {
			// RecreateSwapchain
			return;
		}
		else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
			VK_CHECK(res);
		}
		g_CurrentFrame = (g_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		glfwPollEvents();
	}
	void VKCmdOpen(CommandList& cmdList) {
		PROFILE_FUNCTION();
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid Command List");
		RENDERX_ASSERT_MSG(!cmdList.isOpen, "CommandList is already open");

		cmdList.isOpen = true;
		auto& frame = GetCurrentFrameContex();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CHECK(vkBeginCommandBuffer(frame.commandBuffers[cmdList.id], &beginInfo));
	}

	void VKCmdClose(CommandList& cmdList) {
		PROFILE_FUNCTION();
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid CommandList");
		auto& frame = GetCurrentFrameContex();
		RENDERX_ASSERT_MSG(cmdList.isOpen, "To close a CommnadList it must be created and opened first");
		vkEndCommandBuffer(frame.commandBuffers[cmdList.id]);
		cmdList.isOpen = false;
	}

	void VKCmdDraw(CommandList& cmdList, uint32_t vertexCount, uint32_t instanceCount,
		uint32_t firstVertex, uint32_t firstInstance) {
		PROFILE_FUNCTION();
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid CommandList");
		RENDERX_ASSERT_MSG(cmdList.isOpen, "CommadList is not in the open state");
		vkCmdDraw(GetCurrentFrameContex().commandBuffers[cmdList.id],
			vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VKCmdDrawIndexed(CommandList& cmdList,
		uint32_t indexCount, int32_t vertexOffset,
		uint32_t instanceCount, uint32_t firstIndex, uint32_t firstInstance) {
		PROFILE_FUNCTION();
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid CommandList")
		RENDERX_ASSERT_MSG(cmdList.isOpen, "CommadList is not in the open state");
		vkCmdDrawIndexed(GetCurrentFrameContex().commandBuffers[cmdList.id], indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	CommandList VKCreateCommandList() {
		PROFILE_FUNCTION();
		VulkanContext& ctx = GetVulkanContext();
		auto& frame = GetCurrentFrameContex();
		CommandList cmdList;
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = frame.commandPool;
		allocInfo.commandBufferCount = 1;
		VkCommandBuffer cmdBuffer;
		VkResult result = vkAllocateCommandBuffers(ctx.device, &allocInfo, &cmdBuffer);
		if (!CheckVk(result, "Failed to allocate command buffer")) {
			return CommandList{};
		}
		cmdList.id = frame.commandBuffers.size();
		frame.commandBuffers.push_back(cmdBuffer);
		return cmdList;
	}

	void VKDestroyCommandList(CommandList& cmdList) {
		// dont know what to do with this
	}

	void VKExecuteCommandList(CommandList& cmdList) {
		PROFILE_FUNCTION();
		auto& frame = GetCurrentFrameContex();
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid CommandList");
		RENDERX_ASSERT(cmdList.id < frame.commandBuffers.size());

		VulkanContext& ctx = GetVulkanContext();
		VkSubmitInfo submitinfo{};
		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitinfo.pWaitDstStageMask = &waitStage;
		submitinfo.commandBufferCount = 1;
		submitinfo.pCommandBuffers = &frame.commandBuffers[cmdList.id];
		submitinfo.signalSemaphoreCount = 1;
		submitinfo.pSignalSemaphores = &frame.renderSemaphore;
		submitinfo.waitSemaphoreCount = 1;
		submitinfo.pWaitSemaphores = &frame.presentSemaphore;
		vkQueueSubmit(ctx.graphicsQueue, 1, &submitinfo, frame.fence);
	}


} // namespace RenderXVK

} // namespace RenderX