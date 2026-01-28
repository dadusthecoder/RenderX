#include "RenderX/Log.h"
#include "RenderXVK.h"
#include "CommonVK.h"


namespace Rx {
namespace RxVK {

	static uint32_t s_NextRenderPassId = 1;

	RenderPassHandle VKCreateRenderPass(const RenderPassDesc& desc) {
		auto& ctx = GetVulkanContext();

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
		VK_CHECK(vkCreateRenderPass(ctx.device, &ci, nullptr, &rp));

		// Use ResourcePool for RenderPass management
		RenderPassHandle handle = g_RenderPassPool.allocate(rp);

		return handle;
	}

	void VKDestroyRenderPass(RenderPassHandle& handle) {
		auto& ctx = GetVulkanContext();

		// Try ResourcePool first
		auto& rp = g_RenderPassPool.get(handle);
		if (rp != VK_NULL_HANDLE) {
			vkDestroyRenderPass(ctx.device, rp, nullptr);
			g_RenderPassPool.free(handle);
		}
	}

	void VKCmdBeginRenderPass(
		CommandList& cmd,
		RenderPassHandle pass,
		const ClearValue* clears,
		uint32_t clearCount) {
		PROFILE_FUNCTION();

		RENDERX_ASSERT_MSG(cmd.IsValid(), "Invalid CommandList");
		RENDERX_ASSERT_MSG(cmd.isOpen, "VKCmdBeginRenderPass : CommandList must be open");
		auto& frame = GetCurrentFrameContex(g_CurrentFrame);
		auto& ctx = GetVulkanContext();

		// Try ResourcePool first
		auto& rp = g_RenderPassPool.get(pass);
		if (rp == VK_NULL_HANDLE) {
			RENDERX_ERROR("Invalid render pass handle: {}", pass.id);
			return;
		}

		VkFramebuffer framebuffer = ctx.swapchainFramebuffers[frame.swapchainImageIndex];

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
		bi.renderPass = rp;
		bi.framebuffer = framebuffer;
		bi.renderArea.extent = ctx.swapchainExtent;
		bi.clearValueCount = (uint32_t)vkClears.size();
		bi.pClearValues = vkClears.data();

		vkCmdBeginRenderPass(
			frame.commandBuffers[cmd.id],
			&bi,
			VK_SUBPASS_CONTENTS_INLINE);
	}

	void VKCmdEndRenderPass(CommandList& cmdList) {
		PROFILE_FUNCTION();

		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid CommandList");
		RENDERX_ASSERT_MSG(cmdList.isOpen, "VKCmdEndRenderPass : CommandList must be open");

		auto& frame = GetCurrentFrameContex(g_CurrentFrame);
		vkCmdEndRenderPass(frame.commandBuffers[cmdList.id]);
	}

	RenderPassHandle VKGetDefaultRenderPass() {
		return GetVulkanContext().swapchainRenderPassHandle;
	}

} // namespace  RxVK


} // namespace Rx
