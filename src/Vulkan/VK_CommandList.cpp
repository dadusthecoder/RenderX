#include "RenderX/DebugProfiler.h"
#include "VK_Common.h"
#include "VK_RenderX.h"

namespace Rx {

namespace RxVK {

// command allocator
CommandList* VulkanCommandAllocator::Allocate() {
    VkCommandBufferAllocateInfo allocInfo{};
    VkCommandBuffer             buffer;
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool        = m_Pool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
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
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
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
    m_CurrentPipeline       = pipeline;
    m_CurrentPipelineLayout = p->layout;
}

void VulkanCommandList::setVertexBuffer(const BufferHandle& buffer, uint64_t offset) {
    PROFILE_FUNCTION();
    m_VertexBuffer       = buffer;
    m_VertexBufferOffset = offset;

    auto* vb = g_BufferPool.get(buffer);
    if (vb == nullptr || vb->buffer == VK_NULL_HANDLE) {
        RENDERX_WARN("invalid vertex buffer handle {}", buffer.id);
        return;
    }

    VkDeviceSize offs = static_cast<VkDeviceSize>(offset);
    VkBuffer     buf  = vb->buffer;
    vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &buf, &offs);
}

void VulkanCommandList::setIndexBuffer(const BufferHandle& buffer, uint64_t offset) {
    PROFILE_FUNCTION();
    m_IndexBuffer       = buffer;
    m_IndexBufferOffset = offset;

    auto* ib = g_BufferPool.get(buffer);
    if (ib == nullptr || ib->buffer == VK_NULL_HANDLE) {
        RENDERX_WARN("invalid index buffer handle");
        return;
    }

    // Assume 32-bit indices for now
    vkCmdBindIndexBuffer(m_CommandBuffer, ib->buffer, static_cast<VkDeviceSize>(offset), VK_INDEX_TYPE_UINT32);
}

void VulkanCommandList::draw(uint32_t vertexCount,
                             uint32_t instanceCount,
                             uint32_t firstVertex,
                             uint32_t firstInstance) {
    PROFILE_FUNCTION();
    vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandList::drawIndexed(
    uint32_t indexCount, int32_t vertexOffset, uint32_t instanceCount, uint32_t firstIndex, uint32_t firstInstance) {
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
    RENDERX_ASSERT(desc.width > 0 && desc.height > 0);

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;

    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = {static_cast<uint32_t>(desc.width), static_cast<uint32_t>(desc.height)};

    renderingInfo.layerCount = 1;
    renderingInfo.viewMask   = 0;

    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    colorAttachments.reserve(desc.colorAttachments.size());

    for (const auto& rxAtt : desc.colorAttachments) {
        VkRenderingAttachmentInfo att{};
        att.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

        att.loadOp  = ToVulkanLoadOp(rxAtt.loadOp);
        att.storeOp = ToVulkanStoreOp(rxAtt.storeOp);

        auto* view = g_TextureViewPool.get(rxAtt.handle);
        RENDERX_ASSERT_MSG(view, "Color attachment texture view is null");

        att.imageView   = view->view;
        att.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        att.clearValue                  = {};
        att.clearValue.color.float32[0] = 1.0f;
        att.clearValue.color.float32[1] = 1.0f;
        att.clearValue.color.float32[2] = 1.0f;
        att.clearValue.color.float32[3] = 1.0f;

        colorAttachments.push_back(att);
    }

    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderingInfo.pColorAttachments    = colorAttachments.empty() ? nullptr : colorAttachments.data();

    VkRenderingAttachmentInfo depthAttachment{};
    VkRenderingAttachmentInfo stencilAttachment{};

    if (desc.hasDepthStencil) {
        auto* depthView = g_TextureViewPool.get(desc.depthStencilAttachment.handle);
        RENDERX_ASSERT_MSG(depthView, "Depth attachment texture view is null");

        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

        depthAttachment.loadOp  = ToVulkanLoadOp(desc.depthStencilAttachment.depthLoadOp);
        depthAttachment.storeOp = ToVulkanStoreOp(desc.depthStencilAttachment.depthStoreOp);

        depthAttachment.imageView   = depthView->view;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

        depthAttachment.clearValue                      = {};
        depthAttachment.clearValue.depthStencil.depth   = 1.0f;
        depthAttachment.clearValue.depthStencil.stencil = 0;

        renderingInfo.pDepthAttachment = &depthAttachment;

        // if later support separate stencil ops:
        // stencilAttachment = depthAttachment;
        // renderingInfo.pStencilAttachment = &stencilAttachment;
    }

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
    if (!handle.IsValid())
        return;
    auto* group = g_ResourceGroupPool.get(handle);
    if (!group)
        return;
    auto* layout = g_PipelineLayoutPool.get(m_CurrentPipelineLayout);
    if (!layout)
        return;
    auto& ctx = GetVulkanContext();
    ctx.descriptorSetManager->bind(m_CommandBuffer, layout->layout, 0, *group);
}

void VulkanCommandList::setFramebuffer(FramebufferHandle handle) {
    VulkanFramebuffer* framebuffer = g_FramebufferPool.get(handle);
    RENDERX_ASSERT_MSG(framebuffer, "framebuffer handle {} is invalid or was not created", handle.id);
    RENDERX_ERROR("Function not implemented for the vulkan backend yet");
}

void VulkanCommandList::copyBufferToTexture(BufferHandle             srcBuffer,
                                            TextureHandle            dstTexture,
                                            const TextureCopyRegion& region) {
    RENDERX_ERROR("is not implemented for the Vulkan backend yet");
}

void VulkanCommandList::copyTexture(TextureHandle            srcTexture,
                                    TextureHandle            dstTexture,
                                    const TextureCopyRegion& region) {
    RENDERX_ERROR("is not implemented for the Vulkan backend yet");
}

void VulkanCommandList::copyBuffer(BufferHandle src, BufferHandle dst, const BufferCopyRegion& region) {
    RENDERX_ERROR("is not implemented for the Vulkan backend yet");
}

void VulkanCommandList::copyTextureToBuffer(TextureHandle            srcTexture,
                                            BufferHandle             dstBuffer,
                                            const TextureCopyRegion& region) {
    RENDERX_ERROR("is not implemented for the Vulkan backend yet");
}

void VulkanCommandList::Barrier(const Memory_Barrier* memoryBarriers,
                                uint32_t              memoryCount,
                                const BufferBarrier*  bufferBarriers,
                                uint32_t              bufferCount,
                                const TextureBarrier* imageBarriers,
                                uint32_t              imageCount) {
    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;

    // --- MEMORY BARRIERS ---
    VkMemoryBarrier2 vkMemory[16];
    for (uint32_t i = 0; i < memoryCount; ++i) {
        vkMemory[i] = {.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                       .srcStageMask  = MapPipelineStage(memoryBarriers[i].srcStage),
                       .srcAccessMask = MapAccess(memoryBarriers[i].srcAccess),
                       .dstStageMask  = MapPipelineStage(memoryBarriers[i].dstStage),
                       .dstAccessMask = MapAccess(memoryBarriers[i].dstAccess)};
    }

    // --- BUFFER BARRIERS ---
    VkBufferMemoryBarrier2 vkBuffer[32];
    for (uint32_t i = 0; i < bufferCount; ++i) {
        auto& b = bufferBarriers[i];

        vkBuffer[i] = {.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                       .srcStageMask        = MapPipelineStage(b.srcStage),
                       .srcAccessMask       = MapAccess(b.srcAccess),
                       .dstStageMask        = MapPipelineStage(b.dstStage),
                       .dstAccessMask       = MapAccess(b.dstAccess),
                       .srcQueueFamilyIndex = b.srcQueue,
                       .dstQueueFamilyIndex = b.dstQueue,
                       .buffer              = g_BufferPool.get(b.buffer)->buffer,
                       .offset              = b.offset,
                       .size                = b.size};
    }

    // --- IMAGE BARRIERS ---
    VkImageMemoryBarrier2 vkImage[32];
    for (uint32_t i = 0; i < imageCount; ++i) {
        auto& b = imageBarriers[i];

        vkImage[i] = {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask        = MapPipelineStage(b.srcStage),
            .srcAccessMask       = MapAccess(b.srcAccess),
            .dstStageMask        = MapPipelineStage(b.dstStage),
            .dstAccessMask       = MapAccess(b.dstAccess),
            .oldLayout           = MapLayout(b.oldLayout),
            .newLayout           = MapLayout(b.newLayout),
            .srcQueueFamilyIndex = b.srcQueue,
            .dstQueueFamilyIndex = b.dstQueue,
            .image               = g_TexturePool.get(b.texture)->image,
            .subresourceRange    = {
                MapAspect(b.range.aspect), b.range.baseMip, b.range.mipCount, b.range.baseLayer, b.range.layerCount}};
    }

    dependencyInfo.memoryBarrierCount = memoryCount;
    dependencyInfo.pMemoryBarriers    = vkMemory;

    dependencyInfo.bufferMemoryBarrierCount = bufferCount;
    dependencyInfo.pBufferMemoryBarriers    = vkBuffer;

    dependencyInfo.imageMemoryBarrierCount = imageCount;
    dependencyInfo.pImageMemoryBarriers    = vkImage;

    vkCmdPipelineBarrier2(m_CommandBuffer, &dependencyInfo);
}

// void VulkanCommandList::flushBarriers() {

// }

} // namespace RxVK

} // namespace Rx
