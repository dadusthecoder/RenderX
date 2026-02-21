#include "VK_Common.h"
#include "VK_RenderX.h"

namespace Rx {

namespace RxVK {

std::unordered_map<Hash64, BufferViewHandle>     g_BufferViewCache;
ResourcePool<VulkanBufferView, BufferViewHandle> g_BufferViewPool;

// Helper functions - VMA handles memory type selection now

inline Hash64 ComputeBufferViewHash(const BufferViewDesc& desc) {
    Hash64 hash = 0xcbf29ce484222325;

    hash ^= desc.buffer.id;
    hash *= 0x100000001b3;

    hash ^= desc.offset;
    hash *= 0x100000001b3;

    hash ^= desc.range;
    hash *= 0x100000001b3;

    return hash;
}

BufferHandle VKCreateBuffer(const BufferDesc& desc) {
    PROFILE_FUNCTION();

    // Safety validation
    if (desc.size == 0) {
        RENDERX_ERROR("VKCreateBuffer: buffer size is zero");
        return BufferHandle{};
    }

    VulkanContext& ctx = GetVulkanContext();
    if (ctx.allocator->handle() == VK_NULL_HANDLE) {
        RENDERX_ERROR("VKCreateBuffer: VMA allocator is VK_NULL_HANDLE");
        return BufferHandle{};
    }

    RENDERX_ASSERT_MSG(IsValidBufferFlags(desc.usage), "The Buffer Flag Combination is Invalid")
    // Create buffer
    VulkanBuffer vulkanBuffer{};
    vulkanBuffer.size  = desc.size;
    vulkanBuffer.flags = desc.usage;
    auto memFlags      = ToVmaAllocationCreateInfo(desc.memoryType, desc.usage);

    if (!ctx.allocator->createBuffer(desc.size,
                                     ToVulkanBufferUsage(desc.usage),
                                     memFlags.usage,
                                     memFlags.flags,
                                     vulkanBuffer.buffer,
                                     vulkanBuffer.allocation,
                                     &vulkanBuffer.allocInfo))
        return BufferHandle{};

    // Upload initial data if provided
    if (desc.initialData) {
        if (desc.memoryType == MemoryType::GPU_ONLY) {
            // TODO
            // TEMP
            // to future me : Try to use the deffered uploader after you figure out the how to sync that with the users
            // frame loop may i should keep the beginframe and endframe api for rendering usage
            ctx.loadTimeStagingUploader->uploadBuffer(vulkanBuffer.buffer, desc.initialData, (uint32_t)desc.size, 0);
        } else {
            // Direct upload for dynamic and stream buffers
            void* ptr = nullptr;
            if (vulkanBuffer.allocInfo.pMappedData) {
                // Already persistently mapped
                ptr = vulkanBuffer.allocInfo.pMappedData;
            } else {
                ptr = ctx.allocator->map(vulkanBuffer.allocation);
            }
            if (!ptr) {
                RENDERX_ERROR("Buffer mapping resulted in null pointer");
                if (!vulkanBuffer.allocInfo.pMappedData) {
                    ctx.allocator->unmap(vulkanBuffer.allocation);
                }
                ctx.allocator->destroyBuffer(vulkanBuffer.buffer, vulkanBuffer.allocation);
                return BufferHandle{};
            }
            memcpy(ptr, desc.initialData, desc.size);
            if (!vulkanBuffer.allocInfo.pMappedData) {
                ctx.allocator->unmap(vulkanBuffer.allocation);
            }
        }
    }

    vulkanBuffer.bindingCount = desc.bindingCount;
    BufferHandle handle       = g_BufferPool.allocate(vulkanBuffer);
    RENDERX_INFO("Vulkan: Created Buffer | ID: {} | Size: {} bytes", handle.id, desc.size);
    return handle;
}

void VKDestroyBuffer(BufferHandle& handle) {
    PROFILE_FUNCTION();

    if (!handle.isValid()) {
        RENDERX_WARN("VKDestroyBuffer: invalid buffer handle");
        return;
    }

    if (!g_BufferPool.IsAlive(handle)) {
        RENDERX_WARN("VKDestroyBuffer: buffer handle is stale");
        return;
    }

    RxVK::VulkanContext& ctx    = RxVK::GetVulkanContext();
    auto*                buffer = g_BufferPool.get(handle);
    if (!buffer) {
        RENDERX_ERROR("VKDestroyBuffer: failed to retrieve buffer from pool");
        return;
    }

    if (buffer->buffer != VK_NULL_HANDLE) {
        ctx.allocator->destroyBuffer(buffer->buffer, buffer->allocation);
        g_BufferPool.free(handle);
    } else {
        RENDERX_WARN("VKDestroyBuffer: buffer handle already null");
    }
}

BufferViewHandle VKCreateBufferView(const BufferViewDesc& desc) {
    if (!desc.buffer.isValid()) {
        RENDERX_ERROR("VKCreateBufferView: invalid buffer handle");
        return BufferViewHandle{};
    }

    if (!g_BufferPool.IsAlive(desc.buffer)) {
        RENDERX_ERROR("VKCreateBufferView: buffer handle is stale");
        return BufferViewHandle{};
    }

    VulkanBuffer* buffer = g_BufferPool.get(desc.buffer);
    if (!buffer || buffer->buffer == VK_NULL_HANDLE) {
        RENDERX_ERROR("VKCreateBufferView: buffer is VK_NULL_HANDLE");
        return BufferViewHandle{};
    }

    if (desc.offset + desc.range > buffer->size) {
        RENDERX_ERROR("VKCreateBufferView: view range exceeds buffer size (offset: {}, range: {}, buffer size: {})",
                      desc.offset,
                      desc.range,
                      buffer->size);
        return BufferViewHandle{};
    }

    Hash64 hash = ComputeBufferViewHash(desc);

    auto it = g_BufferViewCache.find(hash);
    if (it != g_BufferViewCache.end())
        return it->second;

    VulkanBufferView bufferview{};
    bufferview.buffer  = desc.buffer;
    bufferview.range   = desc.range;
    bufferview.offset  = desc.offset;
    bufferview.hash    = hash;
    bufferview.isValid = true;

    auto handle             = g_BufferViewPool.allocate(bufferview);
    g_BufferViewCache[hash] = handle;
    return handle;
}

void VKDestroyBufferView(BufferViewHandle& handle) {
    g_BufferViewPool.free(handle);
}

void* VKMapBuffer(BufferHandle handle) {
    if (!handle.isValid()) {
        RENDERX_WARN("VKMapBuffer: invalid buffer handle");
        return nullptr;
    }

    if (!g_BufferPool.IsAlive(handle)) {
        RENDERX_WARN("VKMapBuffer: buffer handle is stale");
        return nullptr;
    }

    auto* buffer = g_BufferPool.get(handle);
    if (!buffer) {
        RENDERX_WARN("VKMapBuffer: failed to retrieve buffer from pool");
        return nullptr;
    }

    if (buffer->allocation == VK_NULL_HANDLE) {
        RENDERX_WARN("VKMapBuffer: buffer has no allocation");
        return nullptr;
    }

    if (buffer->allocInfo.pMappedData) {
        return buffer->allocInfo.pMappedData;
    }

    // Consider: attempt ctx.allocator->map() here if mapping is needed
    RENDERX_WARN("Failed to map buffer: id: {}", handle.id);
    return nullptr;
}
} // namespace RxVK

} // namespace Rx