#include "GL_Common.h"

#include <algorithm>
#include <cstring>

namespace Rx::RxGL {

BufferHandle GLCreateBuffer(const BufferDesc& desc) {
    PROFILE_FUNCTION();

    if (desc.size == 0) {
        RENDERX_WARN("GLCreateBuffer: zero-size buffer");
        return BufferHandle{};
    }

    GLBufferResource res{};
    res.desc = desc;
    res.bytes.resize(desc.size);

    if (desc.initialData) {
        std::memcpy(res.bytes.data(), desc.initialData, desc.size);
    }

    BufferHandle handle{GLNextHandle()};
    g_Buffers.emplace(handle.id, std::move(res));
    return handle;
}

BufferViewHandle GLCreateBufferView(const BufferViewDesc& desc) {
    PROFILE_FUNCTION();

    if (!desc.buffer.isValid()) {
        return BufferViewHandle{};
    }

    BufferViewHandle handle{GLNextHandle()};
    g_BufferViews.emplace(handle.id, desc);
    return handle;
}

void GLDestroyBufferView(BufferViewHandle& handle) {
    PROFILE_FUNCTION();
    g_BufferViews.erase(handle.id);
    handle.id = 0;
}

void* GLMapBuffer(BufferHandle handle) {
    PROFILE_FUNCTION();
    auto it = g_Buffers.find(handle.id);
    if (it == g_Buffers.end() || it->second.bytes.empty()) {
        return nullptr;
    }
    return it->second.bytes.data();
}

void GLDestroyBuffer(BufferHandle& handle) {
    PROFILE_FUNCTION();
    g_Buffers.erase(handle.id);
    handle.id = 0;
}

} // namespace Rx::RxGL
