#include "RenderX/Log.h"
#include "RenderXVK.h"
#include "CommonVK.h"


namespace RenderX {
namespace RenderXVK {

	std::unordered_map<uint32_t, VkRenderPass> g_RenderPasses;
	static uint32_t s_NextRenderPassId = 1;

	VkAttachmentLoadOp ToVkAttachmentLoadOp(LoadOp op) {
		switch (op) {
		case LoadOp::Load:
			return VK_ATTACHMENT_LOAD_OP_LOAD;

		case LoadOp::Clear:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;

		case LoadOp::DontCare:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;

		default:
			RENDERX_ERROR("Unknown LoadOp");
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		}
	}

	VkAttachmentStoreOp ToVkAttachmentStoreOp(StoreOp op) {
		switch (op) {
		case StoreOp::Store:
			return VK_ATTACHMENT_STORE_OP_STORE;

		case StoreOp::DontCare:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;

		default:
			RENDERX_ERROR("Unknown LoadOp");
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
	}

	VkImageLayout ToVkLayout(ResourceState state) {
		switch (state) {
		case ResourceState::Undefined:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case ResourceState::RenderTarget:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ResourceState::DepthWrite:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ResourceState::ShaderRead:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ResourceState::Present:
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		default:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}

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

		RenderPassHandle handle = s_NextRenderPassId++;
		g_RenderPasses[handle.id] = rp;

		// temp
		CreateSwapchainFramebuffers(handle);

		return handle;
	}

	void VKDestroyRenderPass(RenderPassHandle& handle) {
		auto& ctx = GetVulkanContext();
		auto it = g_RenderPasses.find(handle.id);
		if (it == g_RenderPasses.end())
			return;

		vkDestroyRenderPass(ctx.device, it->second, nullptr);
		g_RenderPasses.erase(it);
	}

	void VKCmdBeginRenderPass(
		CommandList& cmd,
		RenderPassHandle pass,
		const ClearValue* clears,
		uint32_t clearCount) {
		PROFILE_FUNCTION();

		RENDERX_ASSERT_MSG(cmd.IsValid(), "Invalid CommandList");
		RENDERX_ASSERT_MSG(cmd.isOpen, "CommandList must be open");
		auto& frame = GetCurrentFrameContex();
		auto& ctx = GetVulkanContext();

		VkRenderPass rp = g_RenderPasses[pass.id];
		VkFramebuffer framebuffer = ctx.swapchainFramebuffers[frame.swapchainImageIndex];

		std::vector<VkClearValue> vkClears;
		for(uint32_t  i = 0 ; i < clearCount  ; ++i) {
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
		RENDERX_ASSERT_MSG(cmdList.isOpen, "CommandList must be open");

		auto& frame = GetCurrentFrameContex();
		vkCmdEndRenderPass(frame.commandBuffers[cmdList.id]);
	}

} // namespace  RenderXVK


} // namespace RenderX
