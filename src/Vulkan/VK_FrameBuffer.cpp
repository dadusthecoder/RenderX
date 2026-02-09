#include "RenderX/Log.h"
#include "VK_RenderX.h"
#include "VK_Common.h"

namespace Rx::RxVK {

	static std::unordered_map<uint32_t, VkFramebuffer> s_Framebuffers;
	static uint32_t s_NextFramebufferId = 1;

	FramebufferHandle VKCreateFramebuffer(const FramebufferDesc& desc) {
		auto& ctx = GetVulkanContext();
		(void)ctx; // Context will be used when implementing this function

		// TODO: implement VKCreateFramebuffer - creating VkFramebuffer and
		// registering it in s_Framebuffers. Currently this is intentionally
		// unimplemented and returns an invalid handle.
		// RENDERX_ASSERT(desc.renderPass.IsValid());
		// RENDERX_ASSERT(desc.width > 0 && desc.height > 0);
		//
		// std::vector<VkImageView> attachments;
		//
		// // --- Color attachments ---
		// for (auto& tex : desc.colorAttachments) {
		// 	RENDERX_ASSERT(tex.IsValid());
		// 	attachments.push_back(GetVkImageView(tex));
		// }
		//
		// // --- Depth attachment ---
		// if (desc.depthStencilAttachment.IsValid()) {
		// 	attachments.push_back(GetVkImageView(desc.depthStencilAttachment));
		// }
		//
		// VkFramebufferCreateInfo ci{};
		// ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		// ci.renderPass = s_RenderPasses[desc.renderPass.id];
		// ci.attachmentCount = (uint32_t)attachments.size();
		// ci.pAttachments = attachments.data();
		// ci.width = desc.width;
		// ci.height = desc.height;
		// ci.layers = desc.layers;
		//
		// VkFramebuffer framebuffer;
		// VK_CHECK(vkCreateFramebuffer(ctx.device, &ci, nullptr, &framebuffer));
		//
		// FramebufferHandle handle{ s_NextFramebufferId++ };
		// s_Framebuffers[handle.id] = framebuffer;

		return FramebufferHandle{};
	}

	void VKDestroyFramebuffer(FramebufferHandle& handle) {
		// auto& ctx = GetVulkanContext();
		// auto it = s_Framebuffers.find(handle.id);
		// if (it == s_Framebuffers.end())
		// 	return;

		/*auto& ctx = GetVulkanContext();
		auto it = s_Framebuffers.find(handle.id);
		if (it == s_Framebuffers.end())
			return;


		if (it->second != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(ctx.device->logical(), it->second, nullptr);
		}
		s_Framebuffers.erase(it);
		handle.id = 0;*/
	}
}
