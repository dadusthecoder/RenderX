#include "RenderX/DebugProfiler.h"
#include "VK_RenderX.h"
#include "VK_Common.h"

namespace Rx {

namespace RxVK {

	// command allocator
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

	// command list
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
			RENDERX_WARN("VulkanCommandList::setPipeline: invalid pipeline handle");
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
			RENDERX_WARN("invalid vertex buffer handle {}", buffer.id);
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
			RENDERX_WARN("invalid index buffer handle");
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
		RENDERX_ERROR("is not implemented for the Vulkan backend yet");
	}

	void VulkanCommandList::endRenderPass() {
		PROFILE_FUNCTION();
		RENDERX_ERROR("is not implemented for the Vulkan backend yet");
	}

	void VulkanCommandList::beginRendering(const RenderingDesc& desc) {
		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;

		std::vector<VkRenderingAttachmentInfo> atts;


		for (auto& rxAtt : desc.colorAttachments) {

			VkRenderingAttachmentInfo attinfo{};
			attinfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			attinfo.storeOp = ToVulkanStoreOp(rxAtt.storeOp);
			attinfo.loadOp = ToVulkanLoadOp(rxAtt.loadOp);
			auto* view = g_TextureViewPool.get(rxAtt.handle);
			RENDERX_ASSERT_MSG(view, "color attachment texture view is null");
			attinfo.imageView = view->view;

			auto* texture = g_TexturePool.get(view->texture);
			RENDERX_ASSERT_MSG(texture, "Invalid Texture");
			
			
			//TODO---------------------------------------------------------------------------------
			// for(auto sparse : texture->state.)
			// auto newState = ConvertState(texture->state)
			// if () {
			// 	auto barrierInfo = GetBarrierInfo(texture->state, ResourceState::RENDER_TARGET);
			// 	m_PendingBarriers.push_back(barrierInfo);
			// }
			//--------------------------------------------------------------------------------------

			attinfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attinfo.clearValue = { 1.0f, 1.0f, 1.0f, 1.0f };
			
			//TODO-------------------------------------------------------- 
			//MSAA
			// attinfo.resolveImageView
			// attinfo.resolveImageLayout
			//-------------------------------------------------------- 

			atts.push_back(attinfo);
		}

		VkRenderingAttachmentInfo depthinfo{};
		if (desc.hasDepthStencil) {
			depthinfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthinfo.storeOp = ToVulkanStoreOp(desc.depthStencilAttachment.depthStoreOp);
			depthinfo.loadOp = ToVulkanLoadOp(desc.depthStencilAttachment.depthLoadOp);
			auto* depthview = g_TextureViewPool.get(desc.depthStencilAttachment.handle);
			RENDERX_ASSERT_MSG(depthview, "depth attachment texture view is null");
			depthinfo.imageView = depthview->view;

			// auto* texture = g_TexturePool.get(depthview->texture);
			// RENDERX_ASSERT_MSG(texture, "Invalid Texture");
			// if (!Has(texture->state, ResourceState::DEPTH_WRITE)) {
			// 	auto barrierInfo = GetBarrierInfo(texture->state, ResourceState::DEPTH_WRITE);
			// 	m_PendingBarriers.push_back(barrierInfo);
			// }

			depthinfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			depthinfo.clearValue = { 1.0f, 0.0f };
		}

		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(atts.size());
		renderingInfo.pColorAttachments = atts.data();

		if (desc.hasDepthStencil)
			renderingInfo.pDepthAttachment = &depthinfo;

		renderingInfo.renderArea = { desc.width, desc.height };
		// TODO
		renderingInfo.viewMask = 0;
		renderingInfo.layerCount = 1;

		vkCmdBeginRendering(m_CommandBuffer, &renderingInfo);
	}

	void VulkanCommandList::endRendering() {
		vkCmdEndRendering(m_CommandBuffer);
	}

	void VulkanCommandList::writeBuffer(BufferHandle handle, const void* data, uint32_t offset, uint32_t size) {
		PROFILE_FUNCTION();
		auto* buf = g_BufferPool.get(handle);
		if (buf == nullptr || buf->buffer == VK_NULL_HANDLE) {
			RENDERX_WARN("VulkanCommandList::writeBuffer: invalid destination buffer handle");
			return;
		}

		// Record a buffer update. Note: vkCmdUpdateBuffer has implementation limits.
		RENDERX_WARN("writeBuffer has implementation limits");
		vkCmdUpdateBuffer(m_CommandBuffer, buf->buffer, static_cast<VkDeviceSize>(offset), size, data);
	}

	void VulkanCommandList::setResourceGroup(const ResourceGroupHandle& handle) {
		PROFILE_FUNCTION();
		if (!handle.IsValid()) return;
		auto* group = g_ResourceGroupPool.get(handle);
		if (!group) return;
		auto* layout = g_PipelineLayoutPool.get(m_CurrentPipelineLayout);
		if (!layout) return;
		auto& ctx = GetVulkanContext();
		ctx.descriptorSetManager->bind(m_CommandBuffer, layout->layout, 0, *group);
	}

	void VulkanCommandList::setFramebuffer(FramebufferHandle handle) {
		VulkanFramebuffer* framebuffer = g_FramebufferPool.get(handle);
		RENDERX_ASSERT_MSG(framebuffer, "framebuffer handle {} is invalid or was not created", handle.id);
		RENDERX_ERROR("Function not implemented for the vulkan backend yet");
	}

	void VulkanCommandList::copyBufferToTexture(BufferHandle srcBuffer, TextureHandle dstTexture, const TextureCopyRegion& region) {
		RENDERX_ERROR("is not implemented for the Vulkan backend yet");
	}

	void VulkanCommandList::copyTexture(TextureHandle srcTexture, TextureHandle dstTexture, const TextureCopyRegion& region) {
		RENDERX_ERROR("is not implemented for the Vulkan backend yet");
	}

	void VulkanCommandList::copyBuffer(BufferHandle src, BufferHandle dst, const BufferCopyRegion& region) {
		RENDERX_ERROR("is not implemented for the Vulkan backend yet");
	}

	void VulkanCommandList::copyTextureToBuffer(TextureHandle srcTexture, BufferHandle dstBuffer, const TextureCopyRegion& region) {
		RENDERX_ERROR("is not implemented for the Vulkan backend yet");
	}

	// void VulkanCommandList::flushBarriers() {

		
	// }

} // namespace RxVK

} // namespace Rx
