#include "VK_Common.h"
#include "VK_RenderX.h"

namespace Rx::RxVK {

FramebufferHandle VKCreateFramebuffer(const FramebufferDesc& desc) {
    auto& ctx = GetVulkanContext();
    RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "VKCreateFramebuffer: device is VK_NULL_HANDLE");
    RENDERX_ASSERT(desc.width > 0 && desc.height > 0);

    const bool               hasDepth = desc.depthStencilAttachment.isValid();
    std::vector<VkImageView> attachments;
    attachments.reserve(desc.colorAttachments.size() + (hasDepth ? 1u : 0u));

    for (const auto& tex : desc.colorAttachments) {
        RENDERX_ASSERT(tex.isValid());
        auto* vkTexView = g_TextureViewPool.get(tex);
        RENDERX_ASSERT(vkTexView != nullptr && vkTexView->view != VK_NULL_HANDLE);
        attachments.push_back(vkTexView->view);
    }

    if (hasDepth) {
        auto* vkDepthView = g_TextureViewPool.get(desc.depthStencilAttachment);
        RENDERX_ASSERT(vkDepthView != nullptr && vkDepthView->view != VK_NULL_HANDLE);
        attachments.push_back(vkDepthView->view);
    }

    VkFramebufferCreateInfo ci{};
    ci.sType                  = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.attachmentCount        = static_cast<uint32_t>(attachments.size());
    ci.pAttachments           = attachments.data();
    ci.width                  = desc.width;
    ci.height                 = desc.height;
    ci.layers                 = desc.layers;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkResult      result      = vkCreateFramebuffer(ctx.device->logical(), &ci, nullptr, &framebuffer);
    if (!CheckVk(result, "VKCreateFramebuffer: Failed to create framebuffer")) {
        return FramebufferHandle{};
    }
    RENDERX_ASSERT(framebuffer != VK_NULL_HANDLE);
    VulkanFramebuffer temp{framebuffer};
    auto              handle = g_FramebufferPool.allocate(temp);
    return handle;
}

void VKDestroyFramebuffer(FramebufferHandle& handle) {
    auto* it = g_FramebufferPool.get(handle);
    RENDERX_ASSERT_MSG(it->framebuffer != VK_NULL_HANDLE, "Framebuffer is VK_NULL_HANDLE");

    auto& ctx = GetVulkanContext();
    vkDestroyFramebuffer(ctx.device->logical(), it->framebuffer, nullptr);
    g_FramebufferPool.free(handle);
}
} // namespace Rx::RxVK
