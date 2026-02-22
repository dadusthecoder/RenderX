#include "GL_Common.h"

#include <algorithm>
#include <cstring>

namespace Rx::RxGL {

namespace {
uint32_t DescriptorStrideBytes() {
    return 32;
}
} // namespace

TextureHandle GLCreateTexture(const TextureDesc& desc) {
    PROFILE_FUNCTION();
    TextureHandle handle{GLNextHandle()};

    GLTextureResource res{};
    res.desc = desc;

    const uint32_t w = desc.width == 0 ? 1 : desc.width;
    const uint32_t h = desc.height == 0 ? 1 : desc.height;
    const uint32_t d = desc.depth == 0 ? 1 : desc.depth;
    const uint32_t bytes = w * h * d * 4;
    res.bytes.resize(bytes);

    if (desc.initialData && desc.size > 0) {
        const uint32_t copySize = desc.size < bytes ? desc.size : bytes;
        std::memcpy(res.bytes.data(), desc.initialData, copySize);
    }

    g_Textures.emplace(handle.id, std::move(res));
    return handle;
}

void GLDestroyTexture(TextureHandle& handle) {
    PROFILE_FUNCTION();
    g_Textures.erase(handle.id);
    handle.id = 0;
}

TextureViewHandle GLCreateTextureView(const TextureViewDesc& desc) {
    PROFILE_FUNCTION();
    if (!desc.texture.isValid()) {
        return TextureViewHandle{};
    }
    TextureViewHandle handle{GLNextHandle()};
    g_TextureViews.emplace(handle.id, GLTextureViewResource{desc});
    return handle;
}

void GLDestroyTextureView(TextureViewHandle& handle) {
    PROFILE_FUNCTION();
    g_TextureViews.erase(handle.id);
    handle.id = 0;
}

SetLayoutHandle GLCreateSetLayout(const SetLayoutDesc& desc) {
    PROFILE_FUNCTION();
    SetLayoutHandle handle{GLNextHandle()};
    g_SetLayouts.emplace(handle.id, GLSetLayoutResource{desc});
    return handle;
}

void GLDestroySetLayout(SetLayoutHandle& handle) {
    PROFILE_FUNCTION();
    g_SetLayouts.erase(handle.id);
    handle.id = 0;
}

DescriptorPoolHandle GLCreateDescriptorPool(const DescriptorPoolDesc& desc) {
    PROFILE_FUNCTION();
    DescriptorPoolHandle handle{GLNextHandle()};
    g_DescriptorPools.emplace(handle.id, GLDescriptorPoolResource{desc, {}});
    return handle;
}

void GLDestroyDescriptorPool(DescriptorPoolHandle& handle) {
    PROFILE_FUNCTION();
    g_DescriptorPools.erase(handle.id);
    handle.id = 0;
}

void GLResetDescriptorPool(DescriptorPoolHandle handle) {
    PROFILE_FUNCTION();
    auto it = g_DescriptorPools.find(handle.id);
    if (it == g_DescriptorPools.end()) {
        return;
    }

    for (SetHandle s : it->second.sets) {
        g_Sets.erase(s.id);
    }
    it->second.sets.clear();
}

SetHandle GLAllocateSet(DescriptorPoolHandle pool, SetLayoutHandle layout) {
    PROFILE_FUNCTION();

    auto poolIt = g_DescriptorPools.find(pool.id);
    if (poolIt == g_DescriptorPools.end()) {
        return SetHandle{};
    }

    SetHandle handle{GLNextHandle()};
    g_Sets.emplace(handle.id, GLSetResource{layout, {}});
    poolIt->second.sets.push_back(handle);
    return handle;
}

void GLAllocateSets(DescriptorPoolHandle pool, SetLayoutHandle layout, SetHandle* pSets, uint32_t count) {
    PROFILE_FUNCTION();
    if (!pSets) {
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        pSets[i] = GLAllocateSet(pool, layout);
    }
}

void GLFreeSet(DescriptorPoolHandle pool, SetHandle& set) {
    PROFILE_FUNCTION();

    auto poolIt = g_DescriptorPools.find(pool.id);
    if (poolIt != g_DescriptorPools.end()) {
        auto& v = poolIt->second.sets;
        v.erase(std::remove_if(v.begin(), v.end(), [&](SetHandle s) { return s.id == set.id; }), v.end());
    }

    g_Sets.erase(set.id);
    set.id = 0;
}

void GLWriteSet(SetHandle set, const DescriptorWrite* writes, uint32_t writeCount) {
    PROFILE_FUNCTION();

    auto it = g_Sets.find(set.id);
    if (it == g_Sets.end() || !writes) {
        return;
    }

    for (uint32_t i = 0; i < writeCount; ++i) {
        it->second.writes[writes[i].slot] = writes[i];
    }
}

void GLWriteSets(SetHandle** sets, const DescriptorWrite** writes, uint32_t setCount, const uint32_t* writeCounts) {
    PROFILE_FUNCTION();

    if (!sets || !writes || !writeCounts) {
        return;
    }

    for (uint32_t i = 0; i < setCount; ++i) {
        if (!sets[i] || !writes[i]) {
            continue;
        }
        GLWriteSet(*sets[i], writes[i], writeCounts[i]);
    }
}

DescriptorHeapHandle GLCreateDescriptorHeap(const DescriptorHeapDesc& desc) {
    PROFILE_FUNCTION();

    DescriptorHeapHandle handle{GLNextHandle()};
    GLDescriptorHeapResource heap{};
    heap.desc = desc;
    heap.bytes.resize(static_cast<size_t>(desc.capacity) * DescriptorStrideBytes());
    g_DescriptorHeaps.emplace(handle.id, std::move(heap));
    return handle;
}

void GLDestroyDescriptorHeap(DescriptorHeapHandle& handle) {
    PROFILE_FUNCTION();
    g_DescriptorHeaps.erase(handle.id);
    handle.id = 0;
}

DescriptorPointer GLGetDescriptorHeapPtr(DescriptorHeapHandle heap, uint32_t index) {
    PROFILE_FUNCTION();

    auto it = g_DescriptorHeaps.find(heap.id);
    if (it == g_DescriptorHeaps.end()) {
        return {};
    }

    const uint32_t stride = DescriptorStrideBytes();
    const uint64_t off = static_cast<uint64_t>(index) * stride;
    if (off + stride > it->second.bytes.size()) {
        return {};
    }

    auto* base = it->second.bytes.data() + off;
    return {base, reinterpret_cast<uint64_t>(base), static_cast<uint32_t>(it->second.bytes.size() - off)};
}

SamplerHandle GLCreateSampler(const SamplerDesc& desc) {
    PROFILE_FUNCTION();
    SamplerHandle handle{GLNextHandle()};
    g_Samplers.emplace(handle.id, GLSamplerResource{desc});
    return handle;
}

void GLDestroySampler(SamplerHandle& handle) {
    PROFILE_FUNCTION();
    g_Samplers.erase(handle.id);
    handle.id = 0;
}

void GLFlushUploads() {
    PROFILE_FUNCTION();
}

void GLPrintHandles() {
    PROFILE_FUNCTION();
    RENDERX_INFO("GL Handles | Buffers={} BufferViews={} Textures={} TextureViews={} Shaders={} Pipelines={} Layouts={} Sets={} Pools={} Heaps={} Samplers={}",
                 g_Buffers.size(),
                 g_BufferViews.size(),
                 g_Textures.size(),
                 g_TextureViews.size(),
                 g_Shaders.size(),
                 g_Pipelines.size(),
                 g_PipelineLayouts.size(),
                 g_Sets.size(),
                 g_DescriptorPools.size(),
                 g_DescriptorHeaps.size(),
                 g_Samplers.size());
}

} // namespace Rx::RxGL
