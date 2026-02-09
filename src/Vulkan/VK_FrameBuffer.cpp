#include "VK_RenderX.h"
#include "VK_Common.h"

namespace Rx::RxVK {

	ResourcePool<VulkanFramebuffer, FramebufferHandle> g_FramebufferPool;

	FramebufferHandle VKCreateFramebuffer(const FramebufferDesc& desc) {
		auto& ctx = GetVulkanContext();
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "VKCreateFramebuffer: device is VK_NULL_HANDLE");
		RENDERX_ASSERT(desc.renderPass.IsValid());
		RENDERX_ASSERT(desc.width > 0 && desc.height > 0);

		auto* rp = g_RenderPassPool.get(desc.renderPass);
		if (rp == nullptr || rp->renderPass == VK_NULL_HANDLE) {
			RENDERX_WARN("VKCreateFramebuffer: invalid render pass handle");
			return FramebufferHandle{};
		}

		const bool hasDepth = desc.depthStencilAttachment.IsValid();
		std::vector<VkImageView> attachments;
		attachments.reserve(desc.colorAttachments.size() + (hasDepth ? 1u : 0u));

		for (const auto& tex : desc.colorAttachments) {
			RENDERX_ASSERT(tex.IsValid());
			auto* vkTex = g_TexturePool.get(tex);
			RENDERX_ASSERT(vkTex != nullptr && vkTex->view != VK_NULL_HANDLE);
			attachments.push_back(vkTex->view);
		}

		if (hasDepth) {
			auto* vkDepth = g_TexturePool.get(desc.depthStencilAttachment);
			RENDERX_ASSERT(vkDepth != nullptr && vkDepth->view != VK_NULL_HANDLE);
			attachments.push_back(vkDepth->view);
		}

		VkFramebufferCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		ci.renderPass = rp->renderPass;
		ci.attachmentCount = static_cast<uint32_t>(attachments.size());
		ci.pAttachments = attachments.data();
		ci.width = desc.width;
		ci.height = desc.height;
		ci.layers = desc.layers;
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
		VkResult result = vkCreateFramebuffer(ctx.device->logical(), &ci, nullptr, &framebuffer);
		if (!CheckVk(result, "VKCreateFramebuffer: Failed to create framebuffer")) {
			return FramebufferHandle{};
		}
		RENDERX_ASSERT(framebuffer != VK_NULL_HANDLE);
		VulkanFramebuffer temp{ framebuffer };
		auto handle = g_FramebufferPool.allocate(temp);
		return handle;
	}

	void VKDestroyFramebuffer(FramebufferHandle& handle) {
		auto* it = g_FramebufferPool.get(handle);
		RENDERX_ASSERT_MSG(it->framebuffer != VK_NULL_HANDLE, "Framebuffer is VK_NULL_HANDLE");

		auto& ctx = GetVulkanContext();
		vkDestroyFramebuffer(ctx.device->logical(), it->framebuffer, nullptr);
		g_FramebufferPool.free(handle);
	}
}
