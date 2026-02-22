#include "VK_Common.h"
#include "VK_RenderX.h"

namespace Rx {
namespace RxVK {

ResourcePool<VulkanSetLayout, SetLayoutHandle>           g_SetLayoutPool;
ResourcePool<VulkanDescriptorPool, DescriptorPoolHandle> g_DescriptorPoolPool;
ResourcePool<VulkanSet, SetHandle>                       g_SetPool;
ResourcePool<VulkanDescriptorHeap, DescriptorHeapHandle> g_DescriptorHeapPool;

SetLayoutHandle VKCreateSetLayout(const SetLayoutDesc& desc) {
    auto& ctx = GetVulkanContext();

    VulkanSetLayout internal;
    internal.debugName    = desc.debugName;
    internal.bindingCount = desc.count;

    // Build VkDescriptorSetLayoutBinding array
    VkDescriptorSetLayoutBinding vkBindings[SetLayoutDesc::MAX_BINDINGS];
    // Flags per binding — needed for UPDATE_AFTER_BIND and PARTIALLY_BOUND
    VkDescriptorBindingFlags bindingFlags[SetLayoutDesc::MAX_BINDINGS];
    bool                     needsBindingFlags = false;

    for (uint32_t i = 0; i < desc.count; i++) {
        const Binding& b = desc.bindings[i];

        vkBindings[i].binding            = b.slot;
        vkBindings[i].descriptorType     = ToVulkanDescriptorType(b.type);
        vkBindings[i].descriptorCount    = (b.count == UINT32_MAX) ? 1024 * 1024 : b.count;
        vkBindings[i].stageFlags         = MapShaderStageFlags(b.stages);
        vkBindings[i].pImmutableSamplers = nullptr;

        bindingFlags[i] = 0;
        if (b.updateAfterBind) {
            bindingFlags[i]             |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
            needsBindingFlags            = true;
            internal.hasUpdateAfterBind  = true;
        }
        if (b.partiallyBound) {
            bindingFlags[i]   |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
            needsBindingFlags  = true;
        }
        if (b.nonUniformIndex) {
            // nonuniform indexing is declared in the shader, no layout flag needed
            // but we do need VARIABLE_DESCRIPTOR_COUNT for runtime-sized arrays
            if (b.count == UINT32_MAX) {
                bindingFlags[i]   |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
                needsBindingFlags  = true;
            }
        }

        // Cache internally
        internal.bindings[i].slot            = b.slot;
        internal.bindings[i].vkType          = vkBindings[i].descriptorType;
        internal.bindings[i].count           = vkBindings[i].descriptorCount;
        internal.bindings[i].stages          = vkBindings[i].stageFlags;
        internal.bindings[i].updateAfterBind = b.updateAfterBind;
        internal.bindings[i].partiallyBound  = b.partiallyBound;

        internal.totalDescriptorCount += vkBindings[i].descriptorCount;
    }

    // Build the create info
    VkDescriptorSetLayoutCreateInfo layoutCI = {};
    layoutCI.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCI.bindingCount                    = desc.count;
    layoutCI.pBindings                       = vkBindings;
    layoutCI.flags                           = 0;

    // If any binding has UPDATE_AFTER_BIND, the layout itself needs the flag
    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCI = {};
    if (needsBindingFlags) {
        flagsCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flagsCI.bindingCount  = desc.count;
        flagsCI.pBindingFlags = bindingFlags;
        layoutCI.pNext        = &flagsCI;

        if (internal.hasUpdateAfterBind) {
            layoutCI.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        }
    }

    VK_CHECK(vkCreateDescriptorSetLayout(ctx.device->logical(), &layoutCI, nullptr, &internal.vkLayout));

    if (desc.debugName) {
        // TODO---------------------------------------------------------------------
        //  set debug name via vkSetDebugUtilsObjectNameEXT if available
        //-------------------------------------------------------------------------
    }

    return g_SetLayoutPool.allocate(std::move(internal));
}

void VKDestroySetLayout(SetLayoutHandle& handle) {
    auto& ctx    = GetVulkanContext();
    auto* layout = g_SetLayoutPool.get(handle);
    if (!layout)
        return;

    vkDestroyDescriptorSetLayout(ctx.device->logical(), layout->vkLayout, nullptr);
    g_SetLayoutPool.free(handle);
}

DescriptorPoolHandle VKCreateDescriptorPool(const DescriptorPoolDesc& desc) {
    auto& ctx = GetVulkanContext();

    VulkanDescriptorPool internal;
    internal.flags           = desc.flags;
    internal.layout          = desc.layout;
    internal.capacity        = desc.capacity;
    internal.updateAfterBind = desc.updateAfterBind;
    internal.heapHandle      = desc.heap;
    internal.heapBaseOffset  = desc.heapOffset;
    internal.debugName       = desc.debugName;

    bool isDescriptorSets   = Has(desc.flags, DescriptorPoolFlags::DESCRIPTOR_SETS);
    bool isDescriptorBuffer = Has(desc.flags, DescriptorPoolFlags::DESCRIPTOR_BUFFER);
    bool isLinear           = Has(desc.flags, DescriptorPoolFlags::LINEAR);
    bool isPool             = Has(desc.flags, DescriptorPoolFlags::POOL);

    RENDERX_ASSERT_MSG(isDescriptorSets != isDescriptorBuffer,
                       "DescriptorPool: must specify exactly one of DESCRIPTOR_SETS or DESCRIPTOR_BUFFER");
    RENDERX_ASSERT_MSG(!(isLinear && isPool), "DescriptorPool: LINEAR and POOL are mutually exclusive");

    if (isDescriptorSets) {
        // ── Classic Vulkan pool path ─────────────────────────────────────
        // Get the layout to compute pool sizes
        // auto* layout = g_SetLayoutPool.get(desc.layout);
        // RENDERX_ASSERT_MSG(layout, "DescriptorPool: invalid SetLayoutHandle");

        // VkDescriptorPoolSize poolSizes[7]

        // VkDescriptorPoolCreateInfo poolCI = {};
        // poolCI.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        // poolCI.maxSets                    = desc.capacity;
        // poolCI.poolSizeCount              = 16;
        // poolCI.pPoolSizes                 = poolSizes;
        // poolCI.flags                      = 0;

        // // POOL policy: individual sets can be freed
        // if (isPool) {
        //     poolCI.flags |= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        // }
        // // UPDATE_AFTER_BIND: GPU can read while CPU writes
        // if (desc.updateAfterBind) {
        //     poolCI.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        // }

        // VK_CHECK(vkCreateDescriptorPool(ctx.device->logical(), &poolCI, nullptr, &internal.vkPool));

        internal.vkPool = CreateDescriptorPool(DescriptorPoolSizes(desc.capacity), isPool, desc.updateAfterBind);

    } else {
        // ---- Descriptor buffer path ----------------------------------------------
        // No VkDescriptorPool — we suballocate from the user's heap.
        // Compute the stride per set from the layout.
        auto* layout = g_SetLayoutPool.get(desc.layout);
        if (layout) {
            // For the descriptor buffer path, stride comes from
            // vkGetDescriptorSetLayoutSizeEXT if the extension is available.
            // For now we use a conservative calculation:
            // each binding needs descriptorCount * descriptorSize bytes
            // We'll compute proper alignment in GetLayoutMemoryInfo.

            // Basic stride: sum of all binding sizes, aligned to
            // VkPhysicalDeviceDescriptorBufferPropertiesEXT::descriptorBufferOffsetAlignment
            // Since we may not have the extension, default to 256 byte alignment
            uint64_t stride = 0;
            for (uint32_t i = 0; i < layout->bindingCount; i++) {
                // Each descriptor is 64 bytes (conservative upper bound)
                // Real size from VkPhysicalDeviceDescriptorBufferPropertiesEXT
                stride += layout->bindings[i].count * 64;
            }
            // Align to 256 bytes
            stride                = (stride + 255) & ~255ull;
            internal.stridePerSet = stride;
        }

        // POOL policy: pre-populate freelist
        if (isPool) {
            internal.freeSlots.reserve(desc.capacity);
            for (uint32_t i = 0; i < desc.capacity; i++) {
                internal.freeSlots.push_back(i);
            }
        }
    }

    return g_DescriptorPoolPool.allocate(std::move(internal));
}

void VKDestroyDescriptorPool(DescriptorPoolHandle& handle) {
    auto& ctx  = GetVulkanContext();
    auto* pool = g_DescriptorPoolPool.get(handle);
    if (!pool)
        return;

    if (pool->vkPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(ctx.device->logical(), pool->vkPool, nullptr);
    }

    g_DescriptorPoolPool.free(handle);
}

void VKResetDescriptorPool(DescriptorPoolHandle handle) {
    auto& ctx  = GetVulkanContext();
    auto* pool = g_DescriptorPoolPool.get(handle);
    if (!pool)
        return;

    bool isLinear = Has(pool->flags, DescriptorPoolFlags::LINEAR);
    RENDERX_ASSERT_MSG(isLinear, "ResetDescriptorPool: only LINEAR pools can be reset");

    if (Has(pool->flags, DescriptorPoolFlags::DESCRIPTOR_SETS)) {
        // VK:    vkResetDescriptorPool — invalidates all sets from this pool
        // User must ensure GPU is done before calling this
        VK_CHECK(vkResetDescriptorPool(ctx.device->logical(), pool->vkPool, 0));

    } else {
        // DESCRIPTOR_BUFFER path: just move the write pointer back
        pool->writePtr = pool->heapBaseOffset;
    }
}

SetHandle VKAllocateSet(DescriptorPoolHandle poolHandle, SetLayoutHandle layoutHandle) {
    auto* pool   = g_DescriptorPoolPool.get(poolHandle);
    auto* layout = g_SetLayoutPool.get(layoutHandle);
    RENDERX_ASSERT_MSG(pool, "AllocateSet: invalid pool handle");
    RENDERX_ASSERT_MSG(layout, "AllocateSet: invalid layout handle");

    VulkanSet internal;
    internal.poolFlags    = pool->flags;
    internal.poolHandle   = poolHandle;
    internal.layoutHandle = layoutHandle;

    if (Has(pool->flags, DescriptorPoolFlags::DESCRIPTOR_SETS)) {
        // ── Classic VK path ──────────────────────────────────────────────
        auto& ctx = GetVulkanContext();

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool              = pool->vkPool;
        allocInfo.descriptorSetCount          = 1;
        allocInfo.pSetLayouts                 = &layout->vkLayout;

        VkResult result = vkAllocateDescriptorSets(ctx.device->logical(), &allocInfo, &internal.vkSet);

        RENDERX_ASSERT_MSG(result == VK_SUCCESS, "AllocateSet: vkAllocateDescriptorSets failed: {}", VkResultToString(result));

    } else {
        // ── Descriptor buffer path ───────────────────────────────────────
        // Find a byte offset in the heap for this set
        bool isLinear = Has(pool->flags, DescriptorPoolFlags::LINEAR);
        bool isPool   = Has(pool->flags, DescriptorPoolFlags::POOL);
        bool isManual = Has(pool->flags, DescriptorPoolFlags::MANUAL);

        if (isLinear) {
            // Bump allocate
            uint64_t offset = pool->heapBaseOffset + pool->writePtr;
            // Align to 256 bytes
            offset = (offset + 255) & ~255ull;

            RENDERX_ASSERT_MSG((offset - pool->heapBaseOffset + pool->stridePerSet) <= (pool->capacity * pool->stridePerSet),
                               "AllocateSet: descriptor buffer pool is full");

            internal.heapHandle = pool->heapHandle;
            internal.byteOffset = offset;

            pool->writePtr = (offset - pool->heapBaseOffset) + pool->stridePerSet;

        } else if (isPool) {
            RENDERX_ASSERT_MSG(!pool->freeSlots.empty(), "AllocateSet: POOL is full");

            uint32_t slot = pool->freeSlots.back();
            pool->freeSlots.pop_back();

            internal.heapHandle = pool->heapHandle;
            internal.byteOffset = pool->heapBaseOffset + slot * pool->stridePerSet;

        } else if (isManual) {
            // MANUAL: byteOffset is not set here — user calls AllocateSetAt
            internal.heapHandle = pool->heapHandle;
            internal.byteOffset = 0; // user must use AllocateSetAt
        }
    }

    return g_SetPool.allocate(std::move(internal));
}

// Batch allocation — one vkAllocateDescriptorSets call for DESCRIPTOR_SETS path
void VKAllocateSets(DescriptorPoolHandle poolHandle, SetLayoutHandle layoutHandle, SetHandle* outSets, uint32_t count) {
    auto* pool   = g_DescriptorPoolPool.get(poolHandle);
    auto* layout = g_SetLayoutPool.get(layoutHandle);
    RENDERX_ASSERT_MSG(pool, "AllocateSets: invalid pool handle");
    RENDERX_ASSERT_MSG(layout, "AllocateSets: invalid layout handle");
    RENDERX_ASSERT_MSG(outSets, "AllocateSets: null output array");

    if (Has(pool->flags, DescriptorPoolFlags::DESCRIPTOR_SETS)) {
        // ── Classic VK path — one API call for all sets ───────────────────
        auto& ctx = GetVulkanContext();

        // Fill layout array — all sets use the same layout
        std::vector<VkDescriptorSetLayout> layouts(count, layout->vkLayout);
        std::vector<VkDescriptorSet>       vkSets(count, VK_NULL_HANDLE);

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool              = pool->vkPool;
        allocInfo.descriptorSetCount          = count;
        allocInfo.pSetLayouts                 = layouts.data();

        VkResult result = vkAllocateDescriptorSets(ctx.device->logical(), &allocInfo, vkSets.data());

        RENDERX_ASSERT_MSG(result == VK_SUCCESS, "AllocateSets: vkAllocateDescriptorSets failed: {}", VkResultToString(result));

        // Wrap each VkDescriptorSet into a SetHandle
        for (uint32_t i = 0; i < count; i++) {
            VulkanSet internal;
            internal.poolFlags    = pool->flags;
            internal.vkSet        = vkSets[i];
            internal.poolHandle   = poolHandle;
            internal.layoutHandle = layoutHandle;
            outSets[i]            = g_SetPool.allocate(std::move(internal));
        }

    } else {
        // ── Descriptor buffer path — allocate N consecutive slots ─────────
        for (uint32_t i = 0; i < count; i++) {
            outSets[i] = VKAllocateSet(poolHandle, layoutHandle);
        }
    }
}

void VKFreeSet(DescriptorPoolHandle poolHandle, SetHandle& setHandle) {
    auto* pool = g_DescriptorPoolPool.get(poolHandle);
    auto* set  = g_SetPool.get(setHandle);
    if (!pool || !set)
        return;

    bool isPool = Has(pool->flags, DescriptorPoolFlags::POOL);
    RENDERX_ASSERT_MSG(isPool,
                       "FreeSet: can only free individual sets from a POOL policy pool. "
                       "For LINEAR pools, use ResetDescriptorPool.");

    if (Has(pool->flags, DescriptorPoolFlags::DESCRIPTOR_SETS)) {
        auto& ctx = GetVulkanContext();
        // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT was set at pool creation
        vkFreeDescriptorSets(ctx.device->logical(), pool->vkPool, 1, &set->vkSet);

    } else {
        // Descriptor buffer path: return the slot to the pool's freelist
        // Compute slot index from byte offset
        uint32_t slot = static_cast<uint32_t>((set->byteOffset - pool->heapBaseOffset) / pool->stridePerSet);
        pool->freeSlots.push_back(slot);
    }

    g_SetPool.free(setHandle);
}

// Write a single resource into a VkDescriptorSet
static void WriteVkDescriptorSingle(
    VkDevice device, VkDescriptorSet dstSet, uint32_t dstBinding, ResourceType type, uint64_t resourceHandle) {
    VkWriteDescriptorSet write = {};
    write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet               = dstSet;
    write.dstBinding           = dstBinding;
    write.dstArrayElement      = 0;
    write.descriptorCount      = 1;
    write.descriptorType       = ToVulkanDescriptorType(type);

    // We need the actual Vulkan object from the handle.
    // The handle encodes either a BufferViewHandle.id or TextureViewHandle.id.
    // Look them up from the global pools.

    VkDescriptorBufferInfo bufInfo = {};
    VkDescriptorImageInfo  imgInfo = {};

    switch (type) {
    case ResourceType::CONSTANT_BUFFER:
    case ResourceType::STORAGE_BUFFER:
    case ResourceType::RW_STORAGE_BUFFER: {
        // resourceHandle is a BufferViewHandle id
        BufferViewHandle bvHandle;
        bvHandle.id   = resourceHandle;
        auto* bufView = g_BufferViewPool.get(bvHandle);
        RENDERX_ASSERT_MSG(bufView, "WriteSet: invalid BufferViewHandle");

        auto* buf = g_BufferPool.get(bufView->buffer);
        RENDERX_ASSERT_MSG(buf, "WriteSet: invalid BufferHandle in view");

        bufInfo.buffer = buf->buffer;
        bufInfo.offset = bufView->offset;
        bufInfo.range  = (bufView->range == 0) ? VK_WHOLE_SIZE : bufView->range;

        write.pBufferInfo = &bufInfo;
        break;
    }

    case ResourceType::TEXTURE_SRV: {
        TextureViewHandle tvHandle;
        tvHandle.id   = resourceHandle;
        auto* texView = g_TextureViewPool.get(tvHandle);
        RENDERX_ASSERT_MSG(texView, "WriteSet: invalid TextureViewHandle for SRV");

        imgInfo.imageView   = texView->view;
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgInfo.sampler     = VK_NULL_HANDLE;

        write.pImageInfo = &imgInfo;
        break;
    }

    case ResourceType::TEXTURE_UAV: {
        TextureViewHandle tvHandle;
        tvHandle.id   = resourceHandle;
        auto* texView = g_TextureViewPool.get(tvHandle);
        RENDERX_ASSERT_MSG(texView, "WriteSet: invalid TextureViewHandle for UAV");

        imgInfo.imageView   = texView->view;
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imgInfo.sampler     = VK_NULL_HANDLE;

        write.pImageInfo = &imgInfo;
        break;
    }

    case ResourceType::SAMPLER: {
        SamplerHandle spHandle;
        spHandle.id   = resourceHandle;
        auto* smapler = g_SamplerPool.get(spHandle);
        RENDERX_ASSERT_MSG(smapler, "invalid sampler handle at slot");
        imgInfo.sampler  = smapler->vkSampler;
        write.pImageInfo = &imgInfo;
        break;
    }

    case ResourceType::COMBINED_TEXTURE_SAMPLER: {
        // resourceHandle is TextureViewHandle.id
        // Sampler handle was packed separately — see DescriptorWrite::CombinedTextureSampler
        TextureViewHandle tvHandle;
        tvHandle.id   = resourceHandle;
        auto* texView = g_TextureViewPool.get(tvHandle);
        RENDERX_ASSERT_MSG(texView, "WriteSet: invalid TextureViewHandle for combined");

        imgInfo.imageView   = texView->view;
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // sampler needs separate handle — extend DescriptorWrite to carry it
        imgInfo.sampler = VK_NULL_HANDLE;

        write.pImageInfo = &imgInfo;
        break;
    }

    default:
        RENDERX_ASSERT_MSG(false, "WriteSet: unsupported ResourceType");
        return;
    }

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void VKWriteSet(SetHandle setHandle, const DescriptorWrite* writes, uint32_t writeCount) {
    auto* set = g_SetPool.get(setHandle);
    RENDERX_ASSERT_MSG(set, "WriteSet: invalid SetHandle");

    auto& ctx = GetVulkanContext();

    if (Has(set->poolFlags, DescriptorPoolFlags::DESCRIPTOR_SETS)) {

        // 32 writes max / for the sake os stack allocations
        // may be i will cange it to the vector in future
        VkDescriptorBufferInfo bufInfos[32];
        VkDescriptorImageInfo  imgInfos[32];
        VkWriteDescriptorSet   vkWrites[32];
        uint32_t               bufIdx   = 0;
        uint32_t               imgIdx   = 0;
        uint32_t               writeIdx = 0;

        for (uint32_t i = 0; i < writeCount && i < 32; i++) {
            const DescriptorWrite& w = writes[i];

            VkWriteDescriptorSet& vkw = vkWrites[writeIdx];
            vkw                       = {};
            vkw.sType                 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            vkw.dstSet                = set->vkSet;
            vkw.dstBinding            = w.slot;
            vkw.dstArrayElement       = 0;
            vkw.descriptorCount       = 1;
            vkw.descriptorType        = ToVulkanDescriptorType(w.type);

            switch (w.type) {
            case ResourceType::CONSTANT_BUFFER:
            case ResourceType::STORAGE_BUFFER:
            case ResourceType::RW_STORAGE_BUFFER: {
                BufferViewHandle bvHandle;
                bvHandle.id   = w.handle;
                auto* bufView = g_BufferViewPool.get(bvHandle);
                auto* buf     = bufView ? g_BufferPool.get(bufView->buffer) : nullptr;
                RENDERX_ASSERT_MSG(buf, "WriteSet: invalid buffer handle at slot {}", w.slot);

                bufInfos[bufIdx].buffer = buf->buffer;
                bufInfos[bufIdx].offset = bufView->offset;
                bufInfos[bufIdx].range  = (bufView->range == 0) ? VK_WHOLE_SIZE : bufView->range;
                vkw.pBufferInfo         = &bufInfos[bufIdx++];
                break;
            }
            case ResourceType::TEXTURE_SRV: {
                TextureViewHandle tvHandle;
                tvHandle.id   = w.handle;
                auto* texView = g_TextureViewPool.get(tvHandle);
                RENDERX_ASSERT_MSG(texView, "WriteSet: invalid texture view handle at slot {}", w.slot);

                imgInfos[imgIdx].imageView   = texView->view;
                imgInfos[imgIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imgInfos[imgIdx].sampler     = VK_NULL_HANDLE;
                vkw.pImageInfo               = &imgInfos[imgIdx++];
                break;
            }
            case ResourceType::TEXTURE_UAV: {
                TextureViewHandle tvHandle;
                tvHandle.id   = w.handle;
                auto* texView = g_TextureViewPool.get(tvHandle);
                RENDERX_ASSERT_MSG(texView, "invalid texture view handle at slot {}", w.slot);
                imgInfos[imgIdx].imageView   = texView->view;
                imgInfos[imgIdx].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                imgInfos[imgIdx].sampler     = VK_NULL_HANDLE;
                vkw.pImageInfo               = &imgInfos[imgIdx++];
                break;
            }
            case ResourceType::SAMPLER: {
                SamplerHandle spHandle;
                spHandle.id   = w.handle;
                auto* smapler = g_SamplerPool.get(spHandle);
                RENDERX_ASSERT_MSG(smapler, "invalid sampler handle at slot {}", w.slot);
                imgInfos[imgIdx].sampler = smapler->vkSampler;
                vkw.pImageInfo           = &imgInfos[imgIdx++];
                break;
            }

            default: {
                // TODO-------------------------------
                //  Skip unsupported types for now
                RENDERX_WARN("unsupported ResourceType {}", (uint32_t)w.type);
                continue;
                //-----------------------------------
            }
            }

            writeIdx++;
        }

        // One call — the key performance win for DESCRIPTOR_SETS path
        if (writeIdx > 0) {
            vkUpdateDescriptorSets(ctx.device->logical(), writeIdx, vkWrites, 0, nullptr);
        }

    } else {
        //--- Descriptor buffer path --------------------------------------------------
        auto* heap = g_DescriptorHeapPool.get(set->heapHandle);
        RENDERX_ASSERT_MSG(heap, "WriteSet: invalid heap handle in set");
        RENDERX_ASSERT_MSG(heap->mappedPtr, "WriteSet: heap is not CPU-writable");

        uint8_t* setBase = heap->mappedPtr + set->byteOffset;

        // Get layout to find per-binding offsets
        auto* setLayout = g_SetLayoutPool.get(set->layoutHandle);
        RENDERX_ASSERT_MSG(setLayout, "WriteSet: invalid layout in set");

        for (uint32_t i = 0; i < writeCount; i++) {
            const DescriptorWrite& w = writes[i];

            // Find which binding this slot maps to
            uint64_t bindingByteOffset = 0;
            for (uint32_t b = 0; b < setLayout->bindingCount; b++) {
                if (setLayout->bindings[b].slot == w.slot) {
                    break;
                }
                // Advance by this binding's size
                bindingByteOffset += setLayout->bindings[b].count * heap->descriptorSize;
            }

            uint8_t* dest = setBase + bindingByteOffset;

            // For descriptor buffer path:
            // vkGetDescriptorEXT writes the raw descriptor bytes.
            // We call it if VK_EXT_descriptor_buffer is available.
            // For now, write a placeholder — wire to vkGetDescriptorEXT when available.
            (void)dest; // will be used once extension is confirmed present

            // TODO: call vkGetDescriptorEXT(device, &info, size, dest)
            // This requires VK_EXT_descriptor_buffer function pointer loaded at init.
        }
    }
}

// Batch write — all sets in one vkUpdateDescriptorSets call
void VKWriteSets(SetHandle** sets, const DescriptorWrite** writes, uint32_t setCount, const uint32_t* writeCounts) {
    auto& ctx = GetVulkanContext();

    // Count total writes across all sets
    uint32_t totalWrites = 0;
    for (uint32_t i = 0; i < setCount; i++)
        totalWrites += writeCounts[i];

    if (totalWrites == 0)
        return;

    // Allocate flat arrays for the batch
    // Use vectors here since count is runtime — stack allocation for small counts
    std::vector<VkWriteDescriptorSet>   vkWrites(totalWrites);
    std::vector<VkDescriptorBufferInfo> bufInfos(totalWrites);
    std::vector<VkDescriptorImageInfo>  imgInfos(totalWrites);
    uint32_t                            writeIdx = 0;
    uint32_t                            bufIdx   = 0;
    uint32_t                            imgIdx   = 0;

    for (uint32_t s = 0; s < setCount; s++) {
        auto* set = g_SetPool.get(*sets[s]);
        if (!set || !Has(set->poolFlags, DescriptorPoolFlags::DESCRIPTOR_SETS))
            continue;

        for (uint32_t w = 0; w < writeCounts[s]; w++) {
            const DescriptorWrite& dw = writes[s][w];

            VkWriteDescriptorSet& vkw = vkWrites[writeIdx];
            vkw                       = {};
            vkw.sType                 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            vkw.dstSet                = set->vkSet;
            vkw.dstBinding            = dw.slot;
            vkw.dstArrayElement       = 0;
            vkw.descriptorCount       = 1;
            vkw.descriptorType        = ToVulkanDescriptorType(dw.type);

            switch (dw.type) {
            case ResourceType::CONSTANT_BUFFER:
            case ResourceType::STORAGE_BUFFER:
            case ResourceType::RW_STORAGE_BUFFER: {
                BufferViewHandle bvh;
                bvh.id    = dw.handle;
                auto* bv  = g_BufferViewPool.get(bvh);
                auto* buf = bv ? g_BufferPool.get(bv->buffer) : nullptr;
                if (!buf)
                    continue;
                bufInfos[bufIdx] = {buf->buffer, bv->offset, bv->range ? (VkDeviceSize)bv->range : VK_WHOLE_SIZE};
                vkw.pBufferInfo  = &bufInfos[bufIdx++];
                break;
            }
            case ResourceType::TEXTURE_SRV: {
                TextureViewHandle tvh;
                tvh.id   = dw.handle;
                auto* tv = g_TextureViewPool.get(tvh);
                if (!tv)
                    continue;
                imgInfos[imgIdx] = {VK_NULL_HANDLE, tv->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                vkw.pImageInfo   = &imgInfos[imgIdx++];
                break;
            }
            case ResourceType::TEXTURE_UAV: {
                TextureViewHandle tvh;
                tvh.id   = dw.handle;
                auto* tv = g_TextureViewPool.get(tvh);
                if (!tv)
                    continue;
                imgInfos[imgIdx] = {VK_NULL_HANDLE, tv->view, VK_IMAGE_LAYOUT_GENERAL};
                vkw.pImageInfo   = &imgInfos[imgIdx++];
                break;
            }
            default:
                continue;
            }

            writeIdx++;
        }
    }

    if (writeIdx > 0) {
        // this is The whole point of WriteSets  one Vulkan call for all sets
        // but i think this can aslo cause a lil bit of cpu overhead
        vkUpdateDescriptorSets(ctx.device->logical(), writeIdx, vkWrites.data(), 0, nullptr);
    }
}

DescriptorHeapHandle VKCreateDescriptorHeap(const DescriptorHeapDesc& desc) {
    auto& ctx = GetVulkanContext();

    VulkanDescriptorHeap internal;
    internal.type          = desc.type;
    internal.shaderVisible = desc.shaderVisible;
    internal.debugName     = desc.debugName;

    // Descriptor size depends on heap type and is hardware-specific.
    // With VK_EXT_descriptor_buffer: query from
    // VkPhysicalDeviceDescriptorBufferPropertiesEXT.
    // Without the extension (or as a fallback): use conservative sizes.
    uint32_t descriptorSize = 0;
    switch (desc.type) {
    case DescriptorHeapType::RESOURCES:
        // Combined CBV/SRV/UAV — conservative 64 bytes covers all vendors
        descriptorSize = 64;
        break;
    case DescriptorHeapType::SAMPLERS:
        descriptorSize = 32;
        break;
    default:
        descriptorSize = 64;
        break;
    }

    internal.descriptorSize = descriptorSize;
    internal.capacity       = desc.capacity;

    // Total size in bytes
    VkDeviceSize bufferSize = (VkDeviceSize)desc.capacity * descriptorSize;

    // Build usage flags
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (desc.type == DescriptorHeapType::SAMPLERS) {
        usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    // Memory flags based on MemoryType
    VmaAllocationCreateInfo allocCI = {};
    switch (desc.memoryType) {
    case MemoryType::CPU_TO_GPU:
        allocCI.usage = VMA_MEMORY_USAGE_AUTO;
        allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        break;
    case MemoryType::GPU_ONLY:
        allocCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocCI.flags = 0;
        break;
    default:
        allocCI.usage = VMA_MEMORY_USAGE_AUTO;
        allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        break;
    }

    VmaAllocationInfo allocInfo = {};
    ctx.allocator->createBuffer(
        bufferSize, usage, allocCI.usage, allocCI.flags, internal.buffer, internal.allocation, &allocInfo);

    // Get persistent mapped pointer if host-visible
    if (allocCI.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) {
        internal.mappedPtr = static_cast<uint8_t*>(allocInfo.pMappedData);
    }

    // Get GPU device address
    VkBufferDeviceAddressInfo addrInfo = {};
    addrInfo.sType                     = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addrInfo.buffer                    = internal.buffer;
    internal.gpuAddress                = vkGetBufferDeviceAddress(ctx.device->logical(), &addrInfo);

    return g_DescriptorHeapPool.allocate(std::move(internal));
}

void VKDestroyDescriptorHeap(DescriptorHeapHandle& handle) {
    auto& ctx  = GetVulkanContext();
    auto* heap = g_DescriptorHeapPool.get(handle);
    if (!heap)
        return;

    ctx.allocator->destroyBuffer(heap->buffer, heap->allocation);
    g_DescriptorHeapPool.free(handle);
}

DescriptorPointer VKGetDescriptorHeapPtr(DescriptorHeapHandle heapHandle, uint32_t index) {
    auto* heap = g_DescriptorHeapPool.get(heapHandle);
    RENDERX_ASSERT_MSG(heap, "GetDescriptorHeapPtr: invalid heap handle");

    RENDERX_ASSERT_MSG(
        index < heap->capacity, "GetDescriptorHeapPtr: index {} out of bounds (capacity {})", index, heap->capacity);

    uint64_t byteOffset = (uint64_t)index * heap->descriptorSize;

    DescriptorPointer ptr;
    ptr.cpuPtr  = heap->mappedPtr ? heap->mappedPtr + byteOffset : nullptr;
    ptr.gpuAddr = heap->gpuAddress + byteOffset;
    ptr.size    = heap->descriptorSize;
    return ptr;
}

void VulkanCommandList::setDescriptorSet(uint32_t slot, SetHandle setHandle) {
    auto* set = g_SetPool.get(setHandle);
    RENDERX_ASSERT_MSG(set, "setDescriptorSet: invalid SetHandle");

    // We need the pipeline layout to bind against
    auto* pipelineLayout = g_PipelineLayoutPool.get(m_CurrentPipelineLayoutHandle);
    RENDERX_ASSERT_MSG(pipelineLayout, "setDescriptorSet: no pipeline bound — call setPipeline first");

    if (Has(set->poolFlags, DescriptorPoolFlags::DESCRIPTOR_SETS)) {
        // ── Classic VK path ──────────────────────────────────────────────
        vkCmdBindDescriptorSets(m_CommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout->vkLayout,
                                slot, // firstSet
                                1,    // descriptorSetCount
                                &set->vkSet,
                                0, // dynamicOffsetCount
                                nullptr);

    } else {
        // ── Descriptor buffer path ───────────────────────────────────────
        // The buffer must already be bound via setDescriptorHeaps.
        // Here we just set the offset into it.
        // vkCmdSetDescriptorBufferOffsetsEXT requires:
        //   - the buffer index (which buffer in the bound array)
        //   - the byte offset
        // For now we assume buffer index 0 (resources heap).
        // Full multi-heap support requires tracking which heap this set belongs to.

        auto* heap = g_DescriptorHeapPool.get(set->heapHandle);
        RENDERX_ASSERT_MSG(heap, "setDescriptorSet: invalid heap in set");

        // vkCmdSetDescriptorBufferOffsetsEXT(
        //     m_CommandBuffer,
        //     VK_PIPELINE_BIND_POINT_GRAPHICS,
        //     pipelineLayout->layout,
        //     slot,           // firstSet
        //     1,              // setCount
        //     &bufferIndex,   // pBufferIndices — index into bound buffer array
        //     &set->byteOffset);

        // NOTE: vkCmdSetDescriptorBufferOffsetsEXT is a function pointer loaded
        // from VK_EXT_descriptor_buffer at device init time.
        // Wire it up when the extension is confirmed present in VulkanDevice.
        (void)heap;
    }
}

void VulkanCommandList::setDescriptorSets(uint32_t firstSlot, const SetHandle* sets, uint32_t count) {
    if (count == 0)
        return;

    // Check that all sets use the same path
    auto* firstSet = g_SetPool.get(sets[0]);
    RENDERX_ASSERT_MSG(firstSet, "setDescriptorSets: invalid first SetHandle");

    auto* pipelineLayout = g_PipelineLayoutPool.get(m_CurrentPipelineLayoutHandle);
    RENDERX_ASSERT_MSG(pipelineLayout, "setDescriptorSets: no pipeline bound");

    if (Has(firstSet->poolFlags, DescriptorPoolFlags::DESCRIPTOR_SETS)) {
        // Collect all VkDescriptorSets — stack array
        VkDescriptorSet vkSets[32];
        RENDERX_ASSERT_MSG(count <= 32, "setDescriptorSets: max 32 sets per call");

        for (uint32_t i = 0; i < count; i++) {
            auto* s = g_SetPool.get(sets[i]);
            RENDERX_ASSERT_MSG(s, "setDescriptorSets: invalid SetHandle at index {}", i);
            vkSets[i] = s->vkSet;
        }

        // One vkCmdBindDescriptorSets for all sets
        vkCmdBindDescriptorSets(
            m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout->vkLayout, firstSlot, count, vkSets, 0, nullptr);

    } else {
        // Descriptor buffer path — N offset calls
        for (uint32_t i = 0; i < count; i++) {
            setDescriptorSet(firstSlot + i, sets[i]);
        }
    }
}

void VulkanCommandList::setDescriptorHeaps(DescriptorHeapHandle* heaps, uint32_t count) {
    RENDERX_ASSERT_MSG(count <= 2, "setDescriptorHeaps: max 2 heaps (one RESOURCES, one SAMPLERS)");

    // Collect VkBuffer handles and their device addresses
    // vkCmdBindDescriptorBuffersEXT takes VkDescriptorBufferBindingInfoEXT array

    VkDescriptorBufferBindingInfoEXT bindingInfos[2] = {};
    for (uint32_t i = 0; i < count; i++) {
        auto* heap = g_DescriptorHeapPool.get(heaps[i]);
        RENDERX_ASSERT_MSG(heap, "setDescriptorHeaps: invalid heap handle at index {}", i);

        bindingInfos[i].sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
        bindingInfos[i].address = heap->gpuAddress;
        bindingInfos[i].usage   = (heap->type == DescriptorHeapType::SAMPLERS) ? VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT
                                                                               : VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
    }

    // vkCmdBindDescriptorBuffersEXT(m_CommandBuffer, count, bindingInfos);
    // NOTE: function pointer from VK_EXT_descriptor_buffer — wire at device init
    (void)bindingInfos;
}

void VulkanCommandList::pushConstants(uint32_t slot, const void* data, uint32_t sizeIn32BitWords, uint32_t offsetIn32BitWords) {
    RENDERX_ASSERT_MSG(m_BoundPipelineLayout != VK_NULL_HANDLE,
                       "pushConstants: no pipeline bound — call setPipeline before pushConstants");
    RENDERX_ASSERT_MSG(data != nullptr, "pushConstants: data pointer is null");
    RENDERX_ASSERT_MSG(sizeIn32BitWords > 0 && (sizeIn32BitWords % 4) == 0,
                       "pushConstants: size must be > 0 and a multiple of 4 (got {})",
                       sizeIn32BitWords);
    RENDERX_ASSERT_MSG(
        (offsetIn32BitWords % 4) == 0, "pushConstants: offset must be a multiple of 4 (got {})", offsetIn32BitWords);

    // We cache the stage flags that cover this offset+size range from
    // the pipeline layout so we don't pass ALL_STAGES every time.
    // For the common case (one push constant range covering all stages)
    // this is just a single cached value.
    VkShaderStageFlags stages = m_PushConstantStages;

    // If the layout has multiple ranges, find the stages that cover [offset, offset+size)
    if (m_HasMultiplePushRanges) {
        stages = 0;
        for (uint32_t i = 0; i < m_PushRangeCount; i++) {
            const auto& r = m_PushRanges[i];
            // Include this range if it overlaps [offset, offset+size)
            if (r.offset < offsetIn32BitWords + sizeIn32BitWords && r.offset + r.size > offsetIn32BitWords)
                stages |= r.stageFlags;
        }
        RENDERX_ASSERT_MSG(
            stages != 0, "pushConstants: no push constant range covers offset={} size={}", offsetIn32BitWords, sizeIn32BitWords);
    }

    vkCmdPushConstants(m_CommandBuffer, m_BoundPipelineLayout, stages, offsetIn32BitWords, sizeIn32BitWords, data);
}

void VulkanCommandList::setDescriptorBufferOffset(uint32_t slot, uint32_t bufferIndex, uint64_t byteOffset) {
    auto* pipelineLayout = g_PipelineLayoutPool.get(m_CurrentPipelineLayoutHandle);
    RENDERX_ASSERT_MSG(pipelineLayout, "setDescriptorBufferOffset: no pipeline bound");

    // vkCmdSetDescriptorBufferOffsetsEXT(
    //     m_CommandBuffer,
    //     VK_PIPELINE_BIND_POINT_GRAPHICS,
    //     pipelineLayout->layout,
    //     slot,
    //     1,
    //     &bufferIndex,
    //     &byteOffset);

    // NOTE: wire to function pointer once VK_EXT_descriptor_buffer is loaded
    (void)bufferIndex;
    (void)byteOffset;
}

void VulkanCommandList::setDynamicOffset(uint32_t slot, uint32_t byteOffset) {
    auto* pipelineLayout = g_PipelineLayoutPool.get(m_CurrentPipelineLayoutHandle);
    RENDERX_ASSERT_MSG(pipelineLayout, "setDynamicOffset: no pipeline bound");

    // Re-bind the set that is currently bound at this slot with the new dynamic offset.
    // This requires tracking which set is currently bound per slot.
    // Add: SetHandle m_BoundSets[MAX_BOUND_SETS] to VulkanCommandList state.
    // For now: placeholder until bound set tracking is added.

    // vkCmdBindDescriptorSets(
    //     m_CommandBuffer,
    //     VK_PIPELINE_BIND_POINT_GRAPHICS,
    //     pipelineLayout->layout,
    //     slot, 1,
    //     &m_BoundSets[slot].vkSet,
    //     1, &byteOffset);

    (void)slot;
    (void)byteOffset;
}

void VulkanCommandList::pushDescriptor(uint32_t slot, const DescriptorWrite* writes, uint32_t count) {
    // Requires VK_KHR_push_descriptor
    // vkCmdPushDescriptorSetKHR — function pointer loaded at device init

    auto* pipelineLayout = g_PipelineLayoutPool.get(m_CurrentPipelineLayoutHandle);
    RENDERX_ASSERT_MSG(pipelineLayout, "pushDescriptor: no pipeline bound");

    // Build VkWriteDescriptorSet array then call vkCmdPushDescriptorSetKHR
    // Same translation logic as VK_WriteSet — reuse that building code.
    // Placeholder until function pointer is wired:

    (void)slot;
    (void)writes;
    (void)count;
}

void VulkanCommandList::setBindlessTable(BindlessTableHandle table) {
    // Bindless: bind the global descriptor set at set 0.
    // The BindlessTable handle maps to a VkDescriptorSet internally.
    // Wire this once the bindless table backend is implemented.
    (void)table;
}

void VulkanCommandList::setInlineCBV(uint32_t slot, BufferHandle bufHandle, uint64_t offset) {
    auto* buf = g_BufferPool.get(bufHandle);
    RENDERX_ASSERT_MSG(buf, "setInlineCBV: invalid BufferHandle");

    auto* pipelineLayout = g_PipelineLayoutPool.get(m_CurrentPipelineLayoutHandle);
    RENDERX_ASSERT_MSG(pipelineLayout, "setInlineCBV: no pipeline bound");

    VkBufferDeviceAddressInfo addrInfo = {};
    addrInfo.sType                     = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addrInfo.buffer                    = buf->buffer;

    auto&           ctx     = GetVulkanContext();
    VkDeviceAddress gpuAddr = vkGetBufferDeviceAddress(ctx.device->logical(), &addrInfo);

    // vkCmdPushDescriptorSetWithTemplateKHR or vkCmdPushDescriptorSetKHR
    // For inline CBV the cleanest mapping is a push descriptor with
    // a VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC write.
    // Placeholder until push descriptor extension is wired:

    (void)slot;
    (void)gpuAddr;
    (void)offset;
}

void VulkanCommandList::setInlineSRV(uint32_t slot, BufferHandle buf, uint64_t offset) {
    // Same pattern as setInlineCBV but with STORAGE_BUFFER type
    (void)slot;
    (void)buf;
    (void)offset;
}

void VulkanCommandList::setInlineUAV(uint32_t slot, BufferHandle buf, uint64_t offset) {
    // Same pattern as setInlineCBV but with STORAGE_BUFFER RW type
    (void)slot;
    (void)buf;
    (void)offset;
}

// working:
//     VKCreateSetLayout         : vkCreateDescriptorSetLayout, all flags
//     VKDestroySetLayout        : vkDestroyDescriptorSetLayout
//     VKCreateDescriptorPool    : vkCreateDescriptorPool with correct pool sizes
//     VKDestroyDescriptorPool   : vkDestroyDescriptorPool
//     VKResetDescriptorPool     : vkResetDescriptorPool (LINEAR) / byte ptr reset
//     VKAllocateSet             : vkAllocateDescriptorSets (DESCRIPTOR_SETS path)
//     VKAllocateSets            : one vkAllocateDescriptorSets for N sets
//     VKFreeSet                 : vkFreeDescriptorSets (POOL path)
//     VKWriteSet                : batched vkUpdateDescriptorSets (CBV/SRV/UAV)
//     VKWriteSets               : one vkUpdateDescriptorSets for N sets
//     VKCreateDescriptorHeap    : VkBuffer + VMA + device address
//     VKDestroyDescriptorHeap   : destroys VkBuffer
//     VKGetDescriptorHeapPtr    : CPU/GPU pointer into heap
//     setDescriptorSets         :   one vkCmdBindDescriptorSets for N sets
//     pushConstants             :   vkCmdPushConstants

// TODO-------------------------------------------------------------------------------
//  palceholdes needs extension function pointers wired at device init:
//      DESCRIPTOR_BUFFER write    : needs vkGetDescriptorEXT
//      setDescriptorHeaps         : needs vkCmdBindDescriptorBuffersEXT
//      setDescriptorBufferOffset  : needs vkCmdSetDescriptorBufferOffsetsEXT
//      pushDescriptor             : needs vkCmdPushDescriptorSetKHR
//      setInlineCBV/SRV/UAV       : needs push descriptor or device address path
//      setDynamicOffset           : needs bound set tracking per slot
//      setBindlessTable           : needs bindless backend
//-----------------------------------------------------------------------------------

} // namespace RxVK
} // namespace Rx