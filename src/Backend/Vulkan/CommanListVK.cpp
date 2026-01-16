#include "RenderX/Log.h"
#include "RenderXVK.h"
#include "CommonVK.h"

namespace RenderX {

namespace RenderXVK {

	std::unordered_map<uint32_t, VulkanCommandList> s_CommandLists;

	void OpenCommandListVK(CommandList& cmdList) {
		PROFILE_FUNCTION();
		auto it = s_CommandLists.find(cmdList.handle.id);
		if (it == s_CommandLists.end())
			return;
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(it->second.commandBuffer, &beginInfo);
		it->second.isRecording = true;
	}

	void CloseCommandListVK(CommandList& cmdList) {
		PROFILE_FUNCTION();
		auto it = s_CommandLists.find(cmdList.handle.id);
		if (it == s_CommandLists.end())
			return;
		vkEndCommandBuffer(it->second.commandBuffer);
		it->second.isRecording = false;
	}

	void DrawVK(CommandList& cmdList, uint32_t vertexCount, uint32_t instanceCount,
		uint32_t firstVertex, uint32_t firstInstance) {
		PROFILE_FUNCTION();
		vkCmdDraw(s_CommandLists[cmdList.handle.id].commandBuffer,
			vertexCount, instanceCount, firstVertex, firstInstance);
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
		cmdList.handle.id = static_cast<uint32_t>(s_CommandLists.size() + 1);
		s_CommandLists[cmdList.handle.id] = vulkanCmdList;
		return cmdList;
	}

	void VKDestroyCommandList(const CommandList& cmdList) {
		PROFILE_FUNCTION();
		VulkanContext& ctx = GetVulkanContext();
		auto it = s_CommandLists.find(cmdList.handle.id);
		if (it == s_CommandLists.end())
			return;

		vkFreeCommandBuffers(ctx.device, ctx.graphicsCommandPool, 1, &it->second.commandBuffer);
		s_CommandLists.erase(it);
	}

	void VKExecuteCommandList(const CommandList& cmdList) {
		PROFILE_FUNCTION();
		glfwPollEvents();
		auto it = s_CommandLists.find(cmdList.handle.id);
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