#include "RenderX/Log.h"
#include "RenderXVK.h"
#include "CommonVK.h"

namespace RenderX {

namespace RenderXVK {

	std::unordered_map<uint32_t, VulkanCommandList> s_CommandLists;
	static uint32_t s_NextCommandlistId = 1;

	void VKCmdBegin(CommandList& cmdList) {
		PROFILE_FUNCTION();

		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid Command List");

		auto ctx = GetVulkanContext();
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(s_CommandLists[cmdList.id].commandBuffer, &beginInfo);
		s_CommandLists[cmdList.id].isRecording = true;

		VkRenderPassBeginInfo rpbi{};
		rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		VkClearValue clear{};
		clear.color = { { 1.0f,
			0.0f,
			0.0f,
			1.0f } };

		rpbi.clearValueCount = 1;
		rpbi.pClearValues = &clear;

		rpbi.renderArea.offset = { 0, 0 };
		rpbi.renderArea.extent = ctx.swapchainExtent;

		// temp
		rpbi.renderPass = ctx.RenderPass;

		// temp
		rpbi.framebuffer = ctx.swapchainFrambuffers;

		vkCmdBeginRenderPass(s_CommandLists[cmdList.id].commandBuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VKCmdEnd(CommandList& cmdList) {
		PROFILE_FUNCTION();
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid CommandList");
		vkCmdEndRenderPass(s_CommandLists[cmdList.id].commandBuffer);
		vkEndCommandBuffer(s_CommandLists[cmdList.id].commandBuffer);
		s_CommandLists[cmdList.id].isRecording = false;
	}

	void VKCmdDraw(CommandList& cmdList, uint32_t vertexCount, uint32_t instanceCount,
		uint32_t firstVertex, uint32_t firstInstance) {
		PROFILE_FUNCTION();
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid CommandList");
		vkCmdDraw(s_CommandLists[cmdList.id].commandBuffer,
			vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VKCmdDrawIndexed(CommandList& cmdList,
		uint32_t indexCount, int32_t vertexOffset,
		uint32_t instanceCount, uint32_t firstIndex, uint32_t firstInstance) {
		PROFILE_FUNCTION();
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid CommandList")
		vkCmdDrawIndexed(s_CommandLists[cmdList.id].commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	CommandList VKCreateCommandList() {
		PROFILE_FUNCTION();
		VulkanContext& ctx = GetVulkanContext();
		CommandList cmdList;
		VulkanCommandList vulkanCmdList{};

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = ctx.graphicsCommandPool;
		allocInfo.commandBufferCount = 1;
		VkResult result = vkAllocateCommandBuffers(ctx.device, &allocInfo, &vulkanCmdList.commandBuffer);
		if (!CheckVk(result, "Failed to allocate command buffer")) {
			return CommandList{};
		}
		vulkanCmdList.isRecording = false;
		cmdList.id = s_NextCommandlistId++;
		s_CommandLists[cmdList.id] = vulkanCmdList;
		return cmdList;
	}

	void VKDestroyCommandList(CommandList& cmdList) {
		PROFILE_FUNCTION();
		VulkanContext& ctx = GetVulkanContext();

#ifdef _DEBUG
		auto it = s_CommandLists.find(cmdList.id);
		RENDERX_ASSERT(it != s_CommandLists.end());
#endif
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid CommandList")
		vkFreeCommandBuffers(ctx.device, ctx.graphicsCommandPool, 1, &s_CommandLists[cmdList.id].commandBuffer);
		s_CommandLists.erase(cmdList.id);
		cmdList.id = 0;
	}

	void VKExecuteCommandList(CommandList& cmdList) {
		PROFILE_FUNCTION();
		glfwPollEvents();
		auto it = s_CommandLists.find(cmdList.id);
		if (it == s_CommandLists.end())
			return;
		VulkanContext& ctx = GetVulkanContext();
		VkSubmitInfo submitinfo{};
		submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitinfo.commandBufferCount = 1;
		submitinfo.pCommandBuffers = &it->second.commandBuffer;
		vkQueueSubmit(ctx.graphicsQueue, 1, &submitinfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(ctx.graphicsQueue);
		// TODO : Use fences instead of waiting idle later
	}


} // namespace RenderXVK

} // namespace RenderX