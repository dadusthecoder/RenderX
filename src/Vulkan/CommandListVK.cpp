#include "RenderX/Log.h"
#include "RenderX/DebugProfiler.h"
#include "RenderXVK.h"
#include "CommonVK.h"

namespace Rx {

namespace RxVK {

	uint32_t VKBegin() {
		PROFILE_COMMAND_BUFFER("VKBegin");
		g_CurrentFrame = (g_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		auto& frame = GetCurrentFrameContex(g_CurrentFrame);
		auto& ctx = GetVulkanContext();

		{
			PROFILE_SYNC("vkWaitForFences");
			VK_CHECK(vkWaitForFences(ctx.device, 1, &frame.fence, VK_TRUE, UINT64_MAX));
		}
		{
			PROFILE_SYNC("vkResetFences");
			VK_CHECK(vkResetFences(ctx.device, 1, &frame.fence));
		}

		// get the next Image
		{
			PROFILE_SWAPCHAIN("vkAcquireNextImageKHR");
			VK_CHECK(vkAcquireNextImageKHR(ctx.device, ctx.swapchain,
				UINT64_MAX, frame.presentSemaphore, VK_NULL_HANDLE, &frame.swapchainImageIndex));
		}

		// Reset command pool (NOW it is safe)
		{
			PROFILE_COMMAND_BUFFER("vkResetCommandPool");
			VK_CHECK(vkResetCommandPool(ctx.device, frame.commandPool, 0));
		}

		// Clear frame-local command buffers
		frame.commandBuffers.clear();
		frame.commandBuffers.push_back(VkCommandBuffer{});

		return g_CurrentFrame;
	}

	void VKEnd( uint32_t frameIndex) {
		PROFILE_COMMAND_BUFFER("VKEnd");
		auto& frame = GetCurrentFrameContex(frameIndex);
		auto& ctx = GetVulkanContext();
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
				// RecreateSwapchain
				return;
			}
			else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
				VK_CHECK(res);
			}
		}
	}
	
	void VKCmdOpen(CommandList& cmdList) {
		PROFILE_COMMAND_BUFFER("VKCmdOpen");
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid Command List");
		RENDERX_ASSERT_MSG(!cmdList.isOpen, "VKCmdOpen : CommandList is already open");

		cmdList.isOpen = true;
		auto& frame = GetCurrentFrameContex(g_CurrentFrame);;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CHECK(vkBeginCommandBuffer(frame.commandBuffers[cmdList.id], &beginInfo));
	}

	void VKCmdClose(CommandList& cmdList) {
		PROFILE_COMMAND_BUFFER("VKCmdClose");
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "VKCmdClose : Invalid CommandList");
		auto& frame = GetCurrentFrameContex(g_CurrentFrame);;
		RENDERX_ASSERT_MSG(cmdList.isOpen, "VKCmdClose :To close a CommnadList it must be created and opened first");
		vkEndCommandBuffer(frame.commandBuffers[cmdList.id]);
		cmdList.isOpen = false;
	}

	void VKCmdDraw(CommandList& cmdList, uint32_t vertexCount, uint32_t instanceCount,
		uint32_t firstVertex, uint32_t firstInstance) {
		PROFILE_GPU_CALL_WITH_SIZE("VKCmdDraw", vertexCount * instanceCount);
		RENDERX_ASSERT_MSG(cmdList.IsValid(), " VKCmdDraw : Invalid CommandList");
		RENDERX_ASSERT_MSG(cmdList.isOpen, " VKCmdDraw : CommadList is not in the open state");
		vkCmdDraw(GetCurrentFrameContex(g_CurrentFrame).commandBuffers[cmdList.id],
			vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VKCmdDrawIndexed(CommandList& cmdList,
		uint32_t indexCount, int32_t vertexOffset,
		uint32_t instanceCount, uint32_t firstIndex, uint32_t firstInstance) {
		PROFILE_GPU_CALL_WITH_SIZE("VKCmdDrawIndexed", indexCount * instanceCount);
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "VKCmdDraw : Invalid CommandList")
		RENDERX_ASSERT_MSG(cmdList.isOpen, " VKCmdDrawIndexed : CommadList is not in the open state");
		vkCmdDrawIndexed(GetCurrentFrameContex(g_CurrentFrame).commandBuffers[cmdList.id], indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	CommandList VKCreateCommandList() {
		PROFILE_COMMAND_BUFFER("VKCreateCommandList");
		VulkanContext& ctx = GetVulkanContext();
		auto& frame = GetCurrentFrameContex(g_CurrentFrame);
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
		cmdList.id = static_cast<uint64_t>(frame.commandBuffers.size());
		frame.commandBuffers.push_back(cmdBuffer);
		return cmdList;
	}

	void VKDestroyCommandList(CommandList& cmdList) {
		// dont know what to do with this
	}

	void VKExecuteCommandList(CommandList& cmdList) {
		PROFILE_COMMAND_BUFFER("VKExecuteCommandList");
		auto& frame = GetCurrentFrameContex(g_CurrentFrame);
		RENDERX_ASSERT_MSG(cmdList.IsValid(), " VKExecuteCommandList : Invalid CommandList");
		RENDERX_ASSERT(cmdList.id < frame.commandBuffers.size());

		
		VulkanContext& ctx = GetVulkanContext();
		VkSubmitInfo submitinfo{};
		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitinfo.pWaitDstStageMask = &waitStage;
		submitinfo.commandBufferCount = 1;
		submitinfo.pCommandBuffers = &frame.commandBuffers[cmdList.id];
		submitinfo.signalSemaphoreCount = 1;
		submitinfo.pSignalSemaphores = &ctx.swapchainImageSync[frame.swapchainImageIndex].renderFinishedSemaphore;
		submitinfo.waitSemaphoreCount = 1;
		submitinfo.pWaitSemaphores = &frame.presentSemaphore;
		
		{
			PROFILE_SYNC("vkQueueSubmit");
			vkQueueSubmit(ctx.graphicsQueue, 1, &submitinfo, frame.fence);
		}
	}


} // namespace RxVK

} // namespace Rx