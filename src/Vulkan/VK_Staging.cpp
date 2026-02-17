#include "VK_Common.h"
#include "VK_RenderX.h"

namespace Rx {
namespace RxVK {

VulkanStagingAllocator::VulkanStagingAllocator(VulkanContext& ctx, uint32_t chunkSize)
    : m_Ctx(ctx),
      m_ChunkSize(chunkSize) {}

VulkanStagingAllocator::~VulkanStagingAllocator() {
    cleanup();
}

StagingAllocation VulkanStagingAllocator::allocate(uint32_t size, uint32_t alignment) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    // Check if current chunk has space
    if (m_CurrentChunk && m_CurrentChunk->canAllocate(size, alignment)) {
        return m_CurrentChunk->allocate(size, alignment);
    }

    // Need a new chunk
    StagingChunk* chunk = getOrCreateChunk(size);
    if (!chunk) {
        RENDERX_ERROR("Failed to get staging chunk");
        return StagingAllocation{};
    }

    m_CurrentChunk = chunk;
    return m_CurrentChunk->allocate(size, alignment);
}

void VulkanStagingAllocator::submit(uint64_t submissionID) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (!m_CurrentChunk) {
        return;
    }

    // Mark the current chunk as in-flight
    m_CurrentChunk->lastSubmissionID = submissionID;
    m_InFlightChunks.push_back(*m_CurrentChunk);

    // Clear current chunk (will get a new one on next allocate)
    m_CurrentChunk = nullptr;
}

void VulkanStagingAllocator::retire(uint64_t completedSubmissionID) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    // Retire all chunks whose submissions have completed
    while (!m_InFlightChunks.empty()) {
        auto& chunk = m_InFlightChunks.front();

        // If this chunk's submission hasn't completed yet, stop
        if (chunk.lastSubmissionID > completedSubmissionID) {
            break;
        }

        // GPU is done with this chunk - recycle it
        chunk.reset();
        m_FreeChunks.push_back(chunk);
        m_InFlightChunks.pop_front();
    }
}

StagingChunk* VulkanStagingAllocator::getOrCreateChunk(uint32_t requiredSize) {
    // First, try to get a free chunk
    if (!m_FreeChunks.empty()) {
        StagingChunk chunk = m_FreeChunks.back();
        m_FreeChunks.pop_back();

        // If free chunk is big enough, use it
        if (chunk.size >= requiredSize) {
            // Find a slot in m_AllChunks to store it
            for (auto& c : m_AllChunks) {
                if (c.buffer == chunk.buffer) {
                    return &c;
                }
            }
        } else {
            // Put it back, it's too small
            m_FreeChunks.push_back(chunk);
        }
    }

    // Create a new chunk
    uint32_t     chunkSize = std::max(m_ChunkSize, requiredSize);
    StagingChunk newChunk  = createChunk(chunkSize);

    if (newChunk.buffer == VK_NULL_HANDLE) {
        return nullptr;
    }

    m_AllChunks.push_back(newChunk);
    return &m_AllChunks.back();
}

StagingChunk VulkanStagingAllocator::createChunk(uint32_t size) {
    StagingChunk chunk{};
    chunk.size = size;

    // Create staging buffer
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocResult{};

    bool success = m_Ctx.allocator->createBuffer(size,
                                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                 VMA_MEMORY_USAGE_AUTO,
                                                 allocInfo.flags,
                                                 chunk.buffer,
                                                 chunk.allocation,
                                                 &allocResult);

    if (!success) {
        RENDERX_ERROR("[VulkanStagingAllocator] Failed to create staging buffer");
        return chunk;
    }

    // Get mapped pointer
    chunk.mappedPtr = static_cast<uint8_t*>(allocResult.pMappedData);

    if (!chunk.mappedPtr) {
        RENDERX_ERROR("[VulkanStagingAllocator] Failed to map staging buffer");
        destroyChunk(chunk);
        chunk = StagingChunk{};
        return chunk;
    }

    RENDERX_INFO("Created staging chunk: {} MB", size / (1024.0f * 1024.0f));

    return chunk;
}

void VulkanStagingAllocator::destroyChunk(StagingChunk& chunk) {
    if (chunk.buffer != VK_NULL_HANDLE) {
        m_Ctx.allocator->destroyBuffer(chunk.buffer, chunk.allocation);
        chunk.buffer     = VK_NULL_HANDLE;
        chunk.allocation = VK_NULL_HANDLE;
        chunk.mappedPtr  = nullptr;
    }
}

void VulkanStagingAllocator::cleanup() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    // Destroy all chunks
    for (auto& chunk : m_AllChunks) {
        destroyChunk(chunk);
    }

    m_AllChunks.clear();
    m_InFlightChunks.clear();
    m_FreeChunks.clear();
    m_CurrentChunk = nullptr;
}

// VulkanImmediateUploader Implementation

VulkanImmediateUploader::VulkanImmediateUploader(VulkanContext& ctx)
    : m_Ctx(ctx) {
    ensureContext();
}

VulkanImmediateUploader::~VulkanImmediateUploader() {
    auto device = m_Ctx.device->logical();

    if (m_ImmediateCtx.fence != VK_NULL_HANDLE) {
        vkDestroyFence(device, m_ImmediateCtx.fence, nullptr);
    }
    if (m_ImmediateCtx.commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, m_ImmediateCtx.commandPool, nullptr);
    }
}

void VulkanImmediateUploader::ensureContext() {
    if (m_ImmediateCtx.commandPool != VK_NULL_HANDLE) {
        return; // Already created
    }

    auto     device         = m_Ctx.device->logical();
    uint32_t transferFamily = m_Ctx.device->transferFamily();

    // Create command pool for transfer queue
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = transferFamily;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VK_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &m_ImmediateCtx.commandPool));

    // Allocate command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = m_ImmediateCtx.commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &m_ImmediateCtx.commandBuffer));

    // Create fence
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &m_ImmediateCtx.fence));
}

void VulkanImmediateUploader::resetContext() {
    auto device = m_Ctx.device->logical();

    vkWaitForFences(device, 1, &m_ImmediateCtx.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &m_ImmediateCtx.fence);
    vkResetCommandBuffer(m_ImmediateCtx.commandBuffer, 0);
}

VkCommandBuffer VulkanImmediateUploader::beginSingleTimeCommands() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    resetContext();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(m_ImmediateCtx.commandBuffer, &beginInfo));
    m_ImmediateCtx.isRecording = true;

    return m_ImmediateCtx.commandBuffer;
}

void VulkanImmediateUploader::endSingleTimeCommands(VkCommandBuffer cmd) {
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd;

    auto transferQueue = m_Ctx.device->transferQueue();
    VK_CHECK(vkQueueSubmit(transferQueue, 1, &submitInfo, m_ImmediateCtx.fence));

    auto device = m_Ctx.device->logical();
    vkWaitForFences(device, 1, &m_ImmediateCtx.fence, VK_TRUE, UINT64_MAX);

    m_ImmediateCtx.isRecording = false;
}

bool VulkanImmediateUploader::uploadBuffer(
    VkBuffer dstBuffer, const void* data, uint32_t size, uint32_t dstOffset, uint32_t alignment) {
    // Allocate staging memory
    StagingAllocation staging = m_Ctx.stagingAllocator->allocate(size, alignment);
    if (!staging.mappedPtr) {
        RENDERX_ERROR("Failed to allocate staging memory");
        return false;
    }

    // Copy data to staging
    std::memcpy(staging.mappedPtr, data, size);

    // Begin command recording
    VkCommandBuffer cmd = beginSingleTimeCommands();

    // Record copy command
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = staging.offset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size      = size;

    vkCmdCopyBuffer(cmd, staging.buffer, dstBuffer, 1, &copyRegion);

    // Submit and wait
    endSingleTimeCommands(cmd);

    // Mark staging as submitted with a fake high submission ID
    // It's already complete, so it will be recycled immediately
    m_Ctx.stagingAllocator->submit(UINT64_MAX);
    m_Ctx.stagingAllocator->retire(UINT64_MAX);
    return true;
}

bool VulkanImmediateUploader::uploadTexture(VkImage                  dstTexture,
                                            const void*              data,
                                            uint32_t                 size,
                                            const TextureCopyRegion& region) {
    // Allocate staging memory
    StagingAllocation staging = m_Ctx.stagingAllocator->allocate(size);
    if (!staging.mappedPtr) {
        RENDERX_ERROR("[ImmediateUploader] Failed to allocate staging memory");
        return false;
    }

    // Copy data to staging
    std::memcpy(staging.mappedPtr, data, size);

    // Begin command recording
    VkCommandBuffer cmd = beginSingleTimeCommands();

    // Transition image to TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = dstTexture;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = region.dstMipLevel;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = region.dstArrayLayer;
    barrier.subresourceRange.layerCount     = 1;
    barrier.srcAccessMask                   = 0;
    barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy buffer to image
    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset                    = staging.offset;
    copyRegion.bufferRowLength                 = 0; // Tightly packed
    copyRegion.bufferImageHeight               = 0;
    copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel       = region.dstMipLevel;
    copyRegion.imageSubresource.baseArrayLayer = region.dstArrayLayer;
    copyRegion.imageSubresource.layerCount     = 1;
    // copyRegion.imageOffset = { (int32_t)region.dstOffset.x, (int32_t)region.dstOffset.y, (int32_t)region.dstOffset.z
    // }; copyRegion.imageExtent = { region.dstOffset.x, region.dstOffset.y, region.z };

    vkCmdCopyBufferToImage(cmd, staging.buffer, dstTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    // Transition to SHADER_READ_ONLY_OPTIMAL
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Submit and wait
    endSingleTimeCommands(cmd);

    // Recycle staging
    m_Ctx.stagingAllocator->submit(UINT64_MAX);
    m_Ctx.stagingAllocator->retire(UINT64_MAX);
    return true;
}

void VulkanImmediateUploader::beginBatch() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    resetContext();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(m_ImmediateCtx.commandBuffer, &beginInfo));
    m_ImmediateCtx.isRecording = true;
}

void VulkanImmediateUploader::uploadBufferBatched(VkBuffer dstBuffer, const void* data, uint32_t size, uint32_t dstOffset) {
    if (!m_ImmediateCtx.isRecording) {
        RENDERX_ERROR("[ImmediateUploader] Must call beginBatch() first");
        return;
    }
    RENDERX_ASSERT_MSG(dstBuffer != VK_NULL_HANDLE, "dstBuffer is null handle");

    // Allocate staging
    StagingAllocation staging = m_Ctx.stagingAllocator->allocate(size);
    if (!staging.mappedPtr) {
        return;
    }

    std::memcpy(staging.mappedPtr, data, size);

    // Record copy
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = staging.offset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size      = size;

    vkCmdCopyBuffer(m_ImmediateCtx.commandBuffer, staging.buffer, dstBuffer, 1, &copyRegion);
}

void VulkanImmediateUploader::uploadTextureBatched(VkImage                  dstTexture,
                                                   const void*              data,
                                                   uint32_t                 size,
                                                   const TextureCopyRegion& region) {
    if (!m_ImmediateCtx.isRecording) {
        RENDERX_ERROR("[ImmediateUploader] Must call beginBatch() first");
        return;
    }
    RENDERX_ASSERT_MSG(dstTexture != VK_NULL_HANDLE,
                       "VulkanImmediateUploader::uploadTextureBatched: dstTexture is VK_NULL_HANDLE");
    StagingAllocation staging = m_Ctx.stagingAllocator->allocate(size);
    if (!staging.mappedPtr) {
        return;
    }
    std::memcpy(staging.mappedPtr, data, size);
    // TODO
}

bool VulkanImmediateUploader::endBatch() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (!m_ImmediateCtx.isRecording) {
        return false;
    }
    VK_CHECK(vkEndCommandBuffer(m_ImmediateCtx.commandBuffer));
    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &m_ImmediateCtx.commandBuffer;

    auto transferQueue = m_Ctx.device->transferQueue();
    VK_CHECK(vkQueueSubmit(transferQueue, 1, &submitInfo, m_ImmediateCtx.fence));

    auto device = m_Ctx.device->logical();
    vkWaitForFences(device, 1, &m_ImmediateCtx.fence, VK_TRUE, UINT64_MAX);

    m_ImmediateCtx.isRecording = false;

    // Recycle staging
    m_Ctx.stagingAllocator->submit(UINT64_MAX);
    m_Ctx.stagingAllocator->retire(UINT64_MAX);

    return true;
}

// VulkanDeferredUploader Implementation
VulkanDeferredUploader::VulkanDeferredUploader(VulkanContext& ctx)
    : m_Ctx(ctx) {}

VulkanDeferredUploader::~VulkanDeferredUploader() {
    // Wait for any pending uploads
    if (m_Ctx.transferQueue) {
        m_Ctx.transferQueue->WaitIdle();
    }
}

void VulkanDeferredUploader::uploadBuffer(BufferHandle dstBuffer, const void* data, uint32_t size, uint32_t dstOffset) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    VulkanBuffer* buffer = g_BufferPool.get(dstBuffer);
    if (!buffer || !data || size == 0) {
        return;
    }

    // Allocate staging
    StagingAllocation staging = m_Ctx.stagingAllocator->allocate(size);
    if (!staging.mappedPtr) {
        RENDERX_ERROR("[DeferredUploader] Failed to allocate staging");
        return;
    }

    // Copy to staging
    std::memcpy(staging.mappedPtr, data, size);

    // Queue for later flush
    DeferredUpload upload{};
    upload.staging      = staging;
    upload.dstBuffer    = dstBuffer;
    upload.dstOffset    = dstOffset;
    upload.size         = size;
    upload.submissionID = 0; // Will be set on flush

    m_PendingBufferUploads.push_back(upload);
}

void VulkanDeferredUploader::uploadTexture(TextureHandle            dstTexture,
                                           const void*              data,
                                           uint32_t                 size,
                                           const TextureCopyRegion& region) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    VulkanTexture* texture = g_TexturePool.get(dstTexture);
    if (!texture || !data || size == 0) {
        return;
    }

    StagingAllocation staging = m_Ctx.stagingAllocator->allocate(size);
    if (!staging.mappedPtr) {
        return;
    }

    std::memcpy(staging.mappedPtr, data, size);

    DeferredTextureUpload upload{};
    upload.staging      = staging;
    upload.dstTexture   = dstTexture;
    upload.region       = region;
    upload.submissionID = 0;

    m_PendingTextureUploads.push_back(upload);
}

Timeline VulkanDeferredUploader::flush() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_PendingBufferUploads.empty() && m_PendingTextureUploads.empty()) {
        return Timeline{0};
    }

    // Create a one-time command buffer
    auto  cmdAllocator = m_Ctx.transferQueue->CreateCommandAllocator("DeferredUploadAllocator");
    auto  cmdList      = cmdAllocator->Allocate();
    auto* vkCmd        = static_cast<VulkanCommandList*>(cmdList);

    vkCmd->open();

    // Record all buffer copies
    for (auto& upload : m_PendingBufferUploads) {
        VulkanBuffer* buffer = g_BufferPool.get(upload.dstBuffer);
        if (!buffer)
            continue;

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = upload.staging.offset;
        copyRegion.dstOffset = upload.dstOffset;
        copyRegion.size      = upload.size;

        // TODO
        //  vkCmdCopyBuffer(
        //  	vkCmd->m_CommandBuffer,
        //  	upload.staging.buffer,
        //  	buffer->buffer,
        //  	1,
        //  	&copyRegion);
    }

    // Record all texture copies
    for (auto& upload : m_PendingTextureUploads) {
        VulkanTexture* texture = g_TexturePool.get(upload.dstTexture);
        if (!texture)
            continue;

        // Barrier + copy logic here (similar to immediate uploader)
        // ...
    }

    vkCmd->close();

    // Submit to transfer queue
    Timeline timeline = m_Ctx.transferQueue->Submit(cmdList);

    // Mark staging as submitted
    m_Ctx.stagingAllocator->submit(timeline.value);

    // Clear pending uploads
    m_PendingBufferUploads.clear();
    m_PendingTextureUploads.clear();

    // Cleanup command allocator after submit
    m_Ctx.transferQueue->DestroyCommandAllocator(cmdAllocator);

    return timeline;
}

void VulkanDeferredUploader::retire(uint64_t completedSubmission) {
    // Staging allocator handles this automatically
    m_Ctx.stagingAllocator->retire(completedSubmission);
}

} // namespace RxVK
} // namespace Rx
