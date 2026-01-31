#include "RenderX/Log.h"
#include "RenderXVK.h"
#include "CommonVK.h"


namespace Rx {
namespace RxVK {

	static uint32_t s_NextRenderPassId = 1;

	RenderPassHandle VKCreateRenderPass(const RenderPassDesc& desc) {
		auto& ctx = GetVulkanContext();
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "VKCreateRenderPass: device is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(desc.colorAttachments.size() > 0 || desc.hasDepthStencil, 
			"VKCreateRenderPass: at least one attachment is required");

		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> colorRefs;

		// --- Color attachments ---
		for (uint32_t i = 0; i < desc.colorAttachments.size(); i++) {
			const auto& a = desc.colorAttachments[i];

			VkAttachmentDescription att{};
			att.format = ToVkTextureFormat(a.format);
			att.samples = VK_SAMPLE_COUNT_1_BIT;
			att.loadOp = ToVkAttachmentLoadOp(a.loadOp);
			att.storeOp = ToVkAttachmentStoreOp(a.storeOp);
			att.initialLayout = ToVkLayout(a.initialState);
			att.finalLayout = ToVkLayout(a.finalState);

			attachments.push_back(att);

			VkAttachmentReference ref{};
			ref.attachment = i;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorRefs.push_back(ref);
		}

		VkAttachmentReference depthRef{};
		if (desc.hasDepthStencil) {
			const auto& d = desc.depthStencilAttachment;

			VkAttachmentDescription depth{};
			// temp
			depth.format = ToVkTextureFormat(d.format);
			depth.samples = VK_SAMPLE_COUNT_1_BIT;
			depth.loadOp = ToVkAttachmentLoadOp(d.depthLoadOp);
			depth.storeOp = ToVkAttachmentStoreOp(d.depthStoreOp);
			depth.stencilLoadOp = ToVkAttachmentLoadOp(d.stencilLoadOp);
			depth.stencilStoreOp = ToVkAttachmentStoreOp(d.stencilStoreOp);
			depth.initialLayout = ToVkLayout(d.initialState);
			depth.finalLayout = ToVkLayout(d.finalState);

			depthRef.attachment = (uint32_t)attachments.size();
			depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			attachments.push_back(depth);
		}

		// --- Subpass ---
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = (uint32_t)colorRefs.size();
		subpass.pColorAttachments = colorRefs.data();
		if (desc.hasDepthStencil)
			subpass.pDepthStencilAttachment = &depthRef;

		// --- RenderPass ---
		VkRenderPassCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		ci.attachmentCount = (uint32_t)attachments.size();
		ci.pAttachments = attachments.data();
		ci.subpassCount = 1;
		ci.pSubpasses = &subpass;

		VkRenderPass rp;
		VkResult result = vkCreateRenderPass(ctx.device, &ci, nullptr, &rp);
		if (!CheckVk(result, "VKCreateRenderPass: Failed to create render pass")) {
			return RenderPassHandle{};
		}
		RENDERX_ASSERT_MSG(rp != VK_NULL_HANDLE, "VKCreateRenderPass: created render pass is null");

		// Use ResourcePool for RenderPass management
		RenderPassHandle handle = g_RenderPassPool.allocate(rp);

		return handle;
	}

	void VKDestroyRenderPass(RenderPassHandle& handle) {
		auto& ctx = GetVulkanContext();
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "VKDestroyRenderPass: device is VK_NULL_HANDLE");

		if (!handle.IsValid()) {
			RENDERX_WARN("VKDestroyRenderPass: invalid render pass handle");
			return;
		}

		// Try ResourcePool first
		auto* rp = g_RenderPassPool.get(handle);
		if (rp != nullptr && *rp != VK_NULL_HANDLE) {
			vkDestroyRenderPass(ctx.device, *rp, nullptr);
			g_RenderPassPool.free(handle);
		}
		else {
			RENDERX_WARN("VKDestroyRenderPass: render pass already null or invalid");
		}
	}

	void VKCmdBeginRenderPass(
		CommandList& cmdList,
		RenderPassHandle pass,
		const ClearValue* clears,
		uint32_t clearCount) {
		PROFILE_FUNCTION();

		RENDERX_ASSERT_MSG(cmdList.IsValid(), "VKCmdBeginRenderPass: Invalid CommandList");
		auto& frame = GetFrameContex(g_CurrentFrame);
		auto& ctx = GetVulkanContext();
		auto* vkCmdList = g_CommandListPool.get(cmdList);
		RENDERX_ASSERT_MSG(vkCmdList != nullptr, "VKCmdBeginRenderPass: retrieved null command list");
		RENDERX_ASSERT_MSG(vkCmdList->isOpen, "VKCmdBeginRenderPass: CommandList must be open");
		RENDERX_ASSERT_MSG(vkCmdList->cmdBuffer != VK_NULL_HANDLE, "VKCmdBeginRenderPass: command buffer is null");
		RENDERX_ASSERT_MSG(pass.IsValid(), "VKCmdBeginRenderPass: Invalid render pass handle");
		
		if (clears && clearCount > 0) {
			RENDERX_ASSERT_MSG(clears != nullptr, "VKCmdBeginRenderPass: clears pointer is null but clearCount > 0");
		}
		
		// Try ResourcePool first
		auto* rp = g_RenderPassPool.get(pass);
		if (rp == nullptr || *rp == VK_NULL_HANDLE) {
			RENDERX_ERROR("VKCmdBeginRenderPass: Invalid render pass handle: {}", pass.id);
			return;
		}

		RENDERX_ASSERT_MSG(frame.swapchainImageIndex < ctx.swapchainFramebuffers.size(),
			"VKCmdBeginRenderPass: swapchain image index out of bounds");
		
		VkFramebuffer framebuffer = ctx.swapchainFramebuffers[frame.swapchainImageIndex];
		RENDERX_ASSERT_MSG(framebuffer != VK_NULL_HANDLE, "VKCmdBeginRenderPass: framebuffer is null");

		std::vector<VkClearValue> vkClears;
		for (uint32_t i = 0; i < clearCount; ++i) {
			VkClearValue v{};
			auto c = clears[i];
			v.color = { c.color.color.r, c.color.color.g,
				c.color.color.b, c.color.color.a };
			vkClears.push_back(v);
		}

		VkRenderPassBeginInfo bi{};
		bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		bi.renderPass = *rp;
		bi.framebuffer = framebuffer;
		bi.renderArea.extent = ctx.swapchainExtent;
		bi.clearValueCount = (uint32_t)vkClears.size();
		bi.pClearValues = vkClears.data();

		vkCmdBeginRenderPass(
			vkCmdList->cmdBuffer,
			&bi,
			VK_SUBPASS_CONTENTS_INLINE);
	}

	void VKCmdEndRenderPass(CommandList& cmdList) {
		PROFILE_FUNCTION();

		RENDERX_ASSERT_MSG(cmdList.IsValid(), "VKCmdEndRenderPass: Invalid CommandList");
		auto* vkCmdList = g_CommandListPool.get(cmdList);
		RENDERX_ASSERT_MSG(vkCmdList != nullptr, "VKCmdEndRenderPass: retrieved null command list");
		RENDERX_ASSERT_MSG(vkCmdList->isOpen, "VKCmdEndRenderPass: CommandList must be open");
		RENDERX_ASSERT_MSG(vkCmdList->cmdBuffer != VK_NULL_HANDLE, "VKCmdEndRenderPass: command buffer is null");

		auto& frame = GetFrameContex(g_CurrentFrame);
		vkCmdEndRenderPass(vkCmdList->cmdBuffer);
	}

	RenderPassHandle VKGetDefaultRenderPass() {
		return GetVulkanContext().swapchainRenderPassHandle;
	}

} // namespace  RxVK


} // namespace Rx
