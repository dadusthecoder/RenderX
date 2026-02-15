#include "VK_RenderX.h"
#include "VK_Common.h"


namespace Rx {
namespace RxVK {

	RenderPassHandle VKCreateRenderPass(const RenderPassDesc& desc) {
		auto& ctx = GetVulkanContext();
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "VKCreateRenderPass: device is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(desc.colorAttachments.size() > 0 || desc.hasDepthStencil,
			"VKCreateRenderPass: at least one attachment is required");

		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> colorRefs;

		//  Color attachments
		for (uint32_t i = 0; i < desc.colorAttachments.size(); i++) {
			const auto& a = desc.colorAttachments[i];

			VkAttachmentDescription att{};
			att.format = ToVulkanFormat(a.format);
			att.samples = VK_SAMPLE_COUNT_1_BIT;
			att.loadOp = ToVulkanLoadOp(a.loadOp);
			att.storeOp = ToVulkanStoreOp(a.storeOp);
			att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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
			depth.format = ToVulkanFormat(d.format);
			depth.samples = VK_SAMPLE_COUNT_1_BIT;
			depth.loadOp = ToVulkanLoadOp(d.depthLoadOp);
			depth.storeOp = ToVulkanStoreOp(d.depthStoreOp);
			depth.stencilLoadOp = ToVulkanLoadOp(d.stencilLoadOp);
			depth.stencilStoreOp = ToVulkanStoreOp(d.stencilStoreOp);

			depthRef.attachment = (uint32_t)attachments.size();
			depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			attachments.push_back(depth);
		}

		//  Subpass
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = (uint32_t)colorRefs.size();
		subpass.pColorAttachments = colorRefs.data();
		if (desc.hasDepthStencil)
			subpass.pDepthStencilAttachment = &depthRef;

		//  RenderPass
		VkRenderPassCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		ci.attachmentCount = (uint32_t)attachments.size();
		ci.pAttachments = attachments.data();
		ci.subpassCount = 1;
		ci.pSubpasses = &subpass;

		VulkanRenderPass rp;
		VkResult result = vkCreateRenderPass(ctx.device->logical(), &ci, nullptr, &rp.renderPass);
		if (!CheckVk(result, "VKCreateRenderPass: Failed to create render pass")) {
			return RenderPassHandle{};
		}
		RENDERX_ASSERT_MSG(rp.renderPass != VK_NULL_HANDLE, "VKCreateRenderPass: created render pass is null");

		// Use ResourcePool for RenderPass management
		RenderPassHandle handle = g_RenderPassPool.allocate(rp);
		RENDERX_INFO("Created Frame Buffer ID : {}", handle.id);
		return handle;
	}

	void VKDestroyRenderPass(RenderPassHandle& handle) {
		auto& ctx = GetVulkanContext();
		auto* rp = g_RenderPassPool.get(handle);
		if (rp) {
			vkDestroyRenderPass(ctx.device->logical(), rp->renderPass, nullptr);
			g_RenderPassPool.free(handle);
			return;
		}
		RENDERX_ASSERT_MSG(false, "VKDestroyRenderPass: invalid RenderPassHandle");
	} // namespace  RxVK


}
} // namespace Rx
