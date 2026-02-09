#include "RenderX/DebugProfiler.h"
#include "VK_RenderX.h"
#include "VK_Common.h"

namespace Rx {

namespace RxVK {

	CommandList* VulkanCommandAllocator::Allocate() {
		VkCommandBufferAllocateInfo allocInfo{};
		VkCommandBuffer buffer;
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandBufferCount = 1;
		allocInfo.commandPool = m_Pool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, &buffer));
		return new VulkanCommandList(buffer, m_QueueType);
	}

	void VulkanCommandAllocator::Free(CommandList* list) {
		VulkanCommandList* list1 = reinterpret_cast<VulkanCommandList*>(list);
		vkFreeCommandBuffers(m_device, m_Pool, 1, &list1->m_CommandBuffer);
		delete list;
	}

	void VulkanCommandAllocator::Reset(CommandList* list) {
		VulkanCommandList* list1 = reinterpret_cast<VulkanCommandList*>(list);
		VK_CHECK(vkResetCommandBuffer(list1->m_CommandBuffer, 0));
	}

	void VulkanCommandAllocator::Reset() {
		VK_CHECK(vkResetCommandPool(m_device, m_Pool, 0));
	}

	void VulkanCommandList::open() {
		PROFILE_FUNCTION();
		VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CHECK(vkBeginCommandBuffer(m_CommandBuffer, &bi));
	}

	void VulkanCommandList::close() {
		PROFILE_FUNCTION();
		VK_CHECK(vkEndCommandBuffer(m_CommandBuffer));
	}

	void VulkanCommandList::setPipeline(const PipelineHandle& pipeline) {
		PROFILE_FUNCTION();
		auto* p = g_PipelinePool.get(pipeline);
		if (p == nullptr || p->pipeline == VK_NULL_HANDLE) {
			RENDERX_WARN("VulkanCommandList::setPipeline: invalid pipeline");
			return;
		}
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p->pipeline);
		m_CurrentPipeline = pipeline;
		m_CurrentPipelineLayout = p->layout;
	}

	void VulkanCommandList::setVertexBuffer(const BufferHandle& buffer, uint64_t offset) {
		PROFILE_FUNCTION();
		m_VertexBuffer = buffer;
		m_VertexBufferOffset = offset;

		auto* vb = g_BufferPool.get(buffer);
		if (vb == nullptr || vb->buffer == VK_NULL_HANDLE) {
			RENDERX_WARN("VulkanCommandList::setVertexBuffer: invalid buffer");
			return;
		}

		VkDeviceSize offs = static_cast<VkDeviceSize>(offset);
		VkBuffer buf = vb->buffer;
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &buf, &offs);
	}

	void VulkanCommandList::setIndexBuffer(const BufferHandle& buffer, uint64_t offset) {
		PROFILE_FUNCTION();
		m_IndexBuffer = buffer;
		m_IndexBufferOffset = offset;

		auto* ib = g_BufferPool.get(buffer);
		if (ib == nullptr || ib->buffer == VK_NULL_HANDLE) {
			RENDERX_WARN("VulkanCommandList::setIndexBuffer: invalid buffer");
			return;
		}

		// Assume 32-bit indices for now
		vkCmdBindIndexBuffer(m_CommandBuffer, ib->buffer, static_cast<VkDeviceSize>(offset), VK_INDEX_TYPE_UINT32);
	}

	void VulkanCommandList::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
		PROFILE_FUNCTION();
		vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VulkanCommandList::drawIndexed(uint32_t indexCount, int32_t vertexOffset, uint32_t instanceCount, uint32_t firstIndex, uint32_t firstInstance) {
		PROFILE_FUNCTION();
		vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void VulkanCommandList::beginRenderPass(RenderPassHandle pass, const void* clearValues, uint32_t clearCount) {
		PROFILE_FUNCTION();
		// Renderpass/framebuffer creation and framebuffer lookup are handled elsewhere.
		// This is a placeholder; a full implementation should lookup the framebuffer
		// and call vkCmdBeginRenderPass with the correct VkRenderPassBeginInfo.m
	}

	void VulkanCommandList::endRenderPass() {
		PROFILE_FUNCTION();
		// Placeholder - see beginRenderPass
	}

	void VulkanCommandList::writeBuffer(BufferHandle handle, const void* data, uint32_t offset, uint32_t size) {
		PROFILE_FUNCTION();
		auto* buf = g_BufferPool.get(handle);
		if (buf == nullptr || buf->buffer == VK_NULL_HANDLE) {
			RENDERX_WARN("VulkanCommandList::writeBuffer: invalid buffer");
			return;
		}

		// Record a buffer update. Note: vkCmdUpdateBuffer has implementation limits.
		vkCmdUpdateBuffer(m_CommandBuffer, buf->buffer, static_cast<VkDeviceSize>(offset), size, data);
	}

	void VulkanCommandList::setResourceGroup(const ResourceGroupHandle& handle) {
		PROFILE_FUNCTION();
		if (!handle.IsValid()) return;

		auto* group = g_ResourceGroupPool.get(handle);
		if (group == nullptr) {
			RENDERX_WARN("VulkanCommandList::setResourceGroup: invalid resource group");
			return;
		}

		auto* layout = g_PipelineLayoutPool.get(m_CurrentPipelineLayout);
		if (layout == nullptr) {
			RENDERX_WARN("VulkanCommandList::setResourceGroup: no pipeline layout bound");
			return;
		}

		auto& ctx = GetVulkanContext();
		ctx.descriptorSetManager->bind(m_CommandBuffer, layout->layout, 0, *group);
	}

} // namespace RxVK

} // namespace Rx
