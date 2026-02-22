#include "GL_Common.h"

#include <algorithm>
#include <cstring>

namespace Rx::RxGL {

namespace {

bool TryGetBuffer(BufferHandle h, GLBufferResource*& out) {
    auto it = g_Buffers.find(h.id);
    if (it == g_Buffers.end()) {
        out = nullptr;
        return false;
    }
    out = &it->second;
    return true;
}

bool IsFloatFormat(Format fmt) {
    switch (fmt) {
    case Format::R32_SFLOAT:
    case Format::RG32_SFLOAT:
    case Format::RGB32_SFLOAT:
    case Format::RGBA32_SFLOAT:
    case Format::R16_SFLOAT:
    case Format::RG16_SFLOAT:
    case Format::RGBA16_SFLOAT:
        return true;
    default:
        return false;
    }
}

GLint FormatComponents(Format fmt) {
    switch (fmt) {
    case Format::R32_SFLOAT:
    case Format::R16_SFLOAT:
        return 1;
    case Format::RG32_SFLOAT:
    case Format::RG16_SFLOAT:
        return 2;
    case Format::RGB32_SFLOAT:
        return 3;
    case Format::RGBA32_SFLOAT:
    case Format::RGBA16_SFLOAT:
    case Format::RGBA8_UNORM:
    case Format::RGBA8_SRGB:
    case Format::BGRA8_UNORM:
    case Format::BGRA8_SRGB:
        return 4;
    default:
        return 4;
    }
}

GLenum FormatType(Format fmt) {
    switch (fmt) {
    case Format::R16_SFLOAT:
    case Format::RG16_SFLOAT:
    case Format::RGBA16_SFLOAT:
        return GL_FLOAT;
    case Format::R32_SFLOAT:
    case Format::RG32_SFLOAT:
    case Format::RGB32_SFLOAT:
    case Format::RGBA32_SFLOAT:
        return GL_FLOAT;
    case Format::RGBA8_UNORM:
    case Format::RGBA8_SRGB:
    case Format::BGRA8_UNORM:
    case Format::BGRA8_SRGB:
    default:
        return GL_UNSIGNED_BYTE;
    }
}

void ApplyVertexState(const GLCommandState& st, const PipelineDesc* pipelineDesc) {
    if (!pipelineDesc || !st.vertexBuffer.isValid()) {
        return;
    }

    auto vbIt = g_Buffers.find(st.vertexBuffer.id);
    if (vbIt == g_Buffers.end() || vbIt->second.bytes.empty()) {
        return;
    }

    const uint8_t* vbBase = vbIt->second.bytes.data() + st.vertexOffset;

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    for (const auto& attr : pipelineDesc->vertexInputState.attributes) {
        uint32_t stride = 0;
        for (const auto& bind : pipelineDesc->vertexInputState.vertexBindings) {
            if (bind.binding == attr.binding) {
                stride = bind.stride;
                break;
            }
        }

        const void* ptr = vbBase + attr.offset;
        const GLint comps = FormatComponents(attr.format);
        const GLenum type = FormatType(attr.format);

        if (attr.location == 0) {
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(comps, type, static_cast<GLsizei>(stride), ptr);
        } else if (attr.location == 1) {
            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer(comps, type, static_cast<GLsizei>(stride), ptr);
        } else if (attr.location == 2) {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(comps, type, static_cast<GLsizei>(stride), ptr);
        } else if (attr.location == 3 && IsFloatFormat(attr.format) && comps >= 3) {
            glEnableClientState(GL_NORMAL_ARRAY);
            glNormalPointer(type, static_cast<GLsizei>(stride), ptr);
        }
    }
}

void SwapBackbuffer() {
   
}

} // namespace

void GLExecuteCommandList(GLCommandList& cmdList) {
    auto& st = cmdList.state;

    if (st.hasViewport) {
        glViewport(st.viewport.x, st.viewport.y, st.viewport.width, st.viewport.height);
    } else if (g_WindowWidth > 0 && g_WindowHeight > 0) {
        glViewport(0, 0, g_WindowWidth, g_WindowHeight);
    }

    if (st.hasScissor) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(st.scissor.x, st.scissor.y, st.scissor.width, st.scissor.height);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    GLBindPipeline(st.pipeline);

    const PipelineDesc* pd = GLGetPipelineDesc(st.pipeline);
    ApplyVertexState(st, pd);

    GLenum mode = GL_TRIANGLES;
    if (pd) {
        switch (pd->primitiveType) {
        case Topology::POINTS:
            mode = GL_POINTS;
            break;
        case Topology::LINES:
            mode = GL_LINES;
            break;
        case Topology::LINE_STRIP:
            mode = GL_LINE_STRIP;
            break;
        case Topology::TRIANGLE_STRIP:
            mode = GL_TRIANGLE_STRIP;
            break;
        case Topology::TRIANGLE_FAN:
            mode = GL_TRIANGLE_FAN;
            break;
        case Topology::TRIANGLES:
        default:
            mode = GL_TRIANGLES;
            break;
        }
    }

    if (st.indexCount > 0 && st.indexBuffer.isValid()) {
        auto ibIt = g_Buffers.find(st.indexBuffer.id);
        if (ibIt != g_Buffers.end()) {
            const uint8_t* ibData = ibIt->second.bytes.data() + st.indexOffset;
            GLenum idxType = st.indexType == Format::UINT16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
            const uint32_t indexSize = st.indexType == Format::UINT16 ? 2u : 4u;
            const void* indices = ibData + st.firstIndex * indexSize;
            glDrawElements(mode, static_cast<GLsizei>(st.indexCount), idxType, indices);
        }
    } else if (st.vertexCount > 0) {
        glDrawArrays(mode, static_cast<GLint>(st.firstVertex), static_cast<GLsizei>(st.vertexCount));
    }
}

void GLCommandList::open() {
    state = {};
    state.instanceCount = 1;
}

void GLCommandList::close() {}

void GLCommandList::setPipeline(const PipelineHandle& pipeline) {
    state.pipeline = pipeline;
}

void GLCommandList::setVertexBuffer(const BufferHandle& buffer, uint64_t offset) {
    state.vertexBuffer = buffer;
    state.vertexOffset = offset;
}

void GLCommandList::setIndexBuffer(const BufferHandle& buffer, uint64_t offset, Format indextype) {
    state.indexBuffer = buffer;
    state.indexOffset = offset;
    state.indexType   = indextype;
}

void GLCommandList::setFramebuffer(FramebufferHandle) {}

void GLCommandList::setViewport(const Viewport& viewport) {
    state.viewport    = viewport;
    state.hasViewport = true;
}

void GLCommandList::setScissor(const Scissor& scissor) {
    state.scissor    = scissor;
    state.hasScissor = true;
}

void GLCommandList::beginRenderPass(RenderPassHandle, const void* clearValues, uint32_t clearCount) {
    state.inRenderPass = true;
    if (clearValues && clearCount > 0) {
        const auto* cv = static_cast<const ClearValue*>(clearValues);
        state.clearColor = cv[0].color;
    }
    glClearColor(state.clearColor.color.r, state.clearColor.color.g, state.clearColor.color.b, state.clearColor.color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GLCommandList::endRenderPass() {
    state.inRenderPass = false;
}

void GLCommandList::beginRendering(const RenderingDesc& desc) {
    state.inRendering = true;
    state.clearColor  = desc.clearColor;
    glClearColor(state.clearColor.color.r, state.clearColor.color.g, state.clearColor.color.b, state.clearColor.color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GLCommandList::endRendering() {
    state.inRendering = false;
}

void GLCommandList::writeBuffer(BufferHandle handle, const void* data, uint32_t offset, uint32_t size) {
    if (!data || size == 0) {
        return;
    }
    auto it = g_Buffers.find(handle.id);
    if (it == g_Buffers.end()) {
        return;
    }
    if (offset >= it->second.bytes.size()) {
        return;
    }
    const uint32_t copySize = static_cast<uint32_t>(std::min<size_t>(size, it->second.bytes.size() - offset));
    std::memcpy(it->second.bytes.data() + offset, data, copySize);
}

void GLCommandList::copyBuffer(BufferHandle src, BufferHandle dst, const BufferCopy& region) {
    auto srcIt = g_Buffers.find(src.id);
    auto dstIt = g_Buffers.find(dst.id);
    if (srcIt == g_Buffers.end() || dstIt == g_Buffers.end()) {
        return;
    }
    if (region.srcOffset >= srcIt->second.bytes.size() || region.dstOffset >= dstIt->second.bytes.size()) {
        return;
    }
    size_t n = static_cast<size_t>(region.size);
    n = std::min(n, srcIt->second.bytes.size() - static_cast<size_t>(region.srcOffset));
    n = std::min(n, dstIt->second.bytes.size() - static_cast<size_t>(region.dstOffset));
    std::memcpy(dstIt->second.bytes.data() + region.dstOffset, srcIt->second.bytes.data() + region.srcOffset, n);
}

void GLCommandList::copyTexture(TextureHandle srcTexture, TextureHandle dstTexture, const TextureCopy&) {
    auto srcIt = g_Textures.find(srcTexture.id);
    auto dstIt = g_Textures.find(dstTexture.id);
    if (srcIt == g_Textures.end() || dstIt == g_Textures.end()) {
        return;
    }
    const size_t n = std::min(srcIt->second.bytes.size(), dstIt->second.bytes.size());
    std::memcpy(dstIt->second.bytes.data(), srcIt->second.bytes.data(), n);
}

void GLCommandList::copyBufferToTexture(BufferHandle srcBuffer, TextureHandle dstTexture, const TextureCopy&) {
    auto srcIt = g_Buffers.find(srcBuffer.id);
    auto dstIt = g_Textures.find(dstTexture.id);
    if (srcIt == g_Buffers.end() || dstIt == g_Textures.end()) {
        return;
    }
    const size_t n = std::min(srcIt->second.bytes.size(), dstIt->second.bytes.size());
    std::memcpy(dstIt->second.bytes.data(), srcIt->second.bytes.data(), n);
}

void GLCommandList::copyTextureToBuffer(TextureHandle srcTexture, BufferHandle dstBuffer, const TextureCopy&) {
    auto srcIt = g_Textures.find(srcTexture.id);
    auto dstIt = g_Buffers.find(dstBuffer.id);
    if (srcIt == g_Textures.end() || dstIt == g_Buffers.end()) {
        return;
    }
    const size_t n = std::min(srcIt->second.bytes.size(), dstIt->second.bytes.size());
    std::memcpy(dstIt->second.bytes.data(), srcIt->second.bytes.data(), n);
}

void GLCommandList::Barrier(const Memory_Barrier*, uint32_t, const BufferBarrier*, uint32_t, const TextureBarrier*, uint32_t) {}

void GLCommandList::drawIndexed(uint32_t indexCount,
                                int32_t  vertexOffset,
                                uint32_t instanceCount,
                                uint32_t firstIndex,
                                uint32_t firstInstance) {
    state.indexCount      = indexCount;
    state.vertexOffsetIdx = vertexOffset;
    state.instanceCount   = instanceCount;
    state.firstIndex      = firstIndex;
    state.firstInstance   = firstInstance;
    state.vertexCount     = 0;
}

void GLCommandList::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    state.vertexCount   = vertexCount;
    state.instanceCount = instanceCount;
    state.firstVertex   = firstVertex;
    state.firstInstance = firstInstance;
    state.indexCount    = 0;
}

void GLCommandList::setDescriptorSet(uint32_t, SetHandle) {}
void GLCommandList::setDescriptorSets(uint32_t, const SetHandle*, uint32_t) {}
void GLCommandList::setBindlessTable(BindlessTableHandle) {}
void GLCommandList::pushConstants(uint32_t, const void*, uint32_t, uint32_t) {}
void GLCommandList::setDescriptorHeaps(DescriptorHeapHandle*, uint32_t) {}
void GLCommandList::setInlineCBV(uint32_t, BufferHandle, uint64_t) {}
void GLCommandList::setInlineSRV(uint32_t, BufferHandle, uint64_t) {}
void GLCommandList::setInlineUAV(uint32_t, BufferHandle, uint64_t) {}
void GLCommandList::setDescriptorBufferOffset(uint32_t, uint32_t, uint64_t) {}
void GLCommandList::setDynamicOffset(uint32_t, uint32_t) {}
void GLCommandList::pushDescriptor(uint32_t, const DescriptorWrite*, uint32_t) {}

CommandList* GLCommandAllocator::Allocate() {
    return new GLCommandList();
}

void GLCommandAllocator::Reset(CommandList* list) {
    if (list) {
        list->open();
    }
}

void GLCommandAllocator::Free(CommandList* list) {
    delete list;
}

void GLCommandAllocator::Reset() {}

GLCommandQueue::GLCommandQueue(QueueType t)
    : m_Type(t) {}

CommandAllocator* GLCommandQueue::CreateCommandAllocator(const char*) {
    return new GLCommandAllocator();
}

void GLCommandQueue::DestroyCommandAllocator(CommandAllocator* allocator) {
    delete allocator;
}

Timeline GLCommandQueue::Submit(CommandList* commandList) {
    SubmitInfo info{};
    info.commandList = commandList;
    return Submit(info);
}

Timeline GLCommandQueue::Submit(const SubmitInfo& submitInfo) {
    if (submitInfo.commandList) {
        if (auto* glList = dynamic_cast<GLCommandList*>(submitInfo.commandList)) {
            GLExecuteCommandList(*glList);
        }
    }

    const uint64_t signaled = ++m_Submitted;
    m_Completed = m_Submitted;

    if (submitInfo.writesToSwapchain) {
        SwapBackbuffer();
    }

    return Timeline(signaled);
}

bool GLCommandQueue::Wait(Timeline value, uint64_t) {
    return value.value <= m_Completed;
}

void GLCommandQueue::WaitIdle() {
    m_Completed = m_Submitted;
}

bool GLCommandQueue::Poll(Timeline value) {
    return value.value <= m_Completed;
}

Timeline GLCommandQueue::Completed() {
    return Timeline(m_Completed);
}

Timeline GLCommandQueue::Submitted() const {
    return Timeline(m_Submitted);
}

float GLCommandQueue::TimestampFrequency() const {
    return 1.0f;
}

GLSwapchain::GLSwapchain(const SwapchainDesc& desc)
    : m_Width(desc.width),
      m_Height(desc.height),
      m_Count(desc.preferredImageCount == 0 ? 1u : desc.preferredImageCount),
      m_Format(desc.preferredFromat),
      m_Index(0) {
    rebuildImages();
}

void GLSwapchain::rebuildImages() {
    m_Images.clear();
    m_Depths.clear();
    m_ImageViews.clear();
    m_DepthViews.clear();

    for (uint32_t i = 0; i < m_Count; ++i) {
        auto color = GLCreateTexture(TextureDesc::RenderTarget(m_Width, m_Height, m_Format));
        auto depth = GLCreateTexture(TextureDesc::DepthStencil(m_Width, m_Height));
        m_Images.push_back(color);
        m_Depths.push_back(depth);
        m_ImageViews.push_back(GLCreateTextureView(TextureViewDesc::Default(color, m_Format)));
        m_DepthViews.push_back(GLCreateTextureView(TextureViewDesc::Default(depth, Format::D24_UNORM_S8_UINT)));
    }
}

uint32_t GLSwapchain::AcquireNextImage() {
    if (m_Count == 0) {
        return 0;
    }
    m_Index = (m_Index + 1) % m_Count;
    return m_Index;
}

void GLSwapchain::Present(uint32_t) {
    SwapBackbuffer();
}

void GLSwapchain::Resize(uint32_t width, uint32_t height) {
    m_Width = width;
    m_Height = height;
    rebuildImages();
}

Format GLSwapchain::GetFormat() const {
    return m_Format;
}

uint32_t GLSwapchain::GetWidth() const {
    return m_Width;
}

uint32_t GLSwapchain::GetHeight() const {
    return m_Height;
}

uint32_t GLSwapchain::GetImageCount() const {
    return m_Count;
}

TextureHandle GLSwapchain::GetImage(uint32_t imageIndex) const {
    return imageIndex < m_Images.size() ? m_Images[imageIndex] : TextureHandle{};
}

TextureHandle GLSwapchain::GetDepth(uint32_t imageIndex) const {
    return imageIndex < m_Depths.size() ? m_Depths[imageIndex] : TextureHandle{};
}

TextureViewHandle GLSwapchain::GetImageView(uint32_t imageIndex) const {
    return imageIndex < m_ImageViews.size() ? m_ImageViews[imageIndex] : TextureViewHandle{};
}

TextureViewHandle GLSwapchain::GetDepthView(uint32_t imageIndex) const {
    return imageIndex < m_DepthViews.size() ? m_DepthViews[imageIndex] : TextureViewHandle{};
}

CommandQueue* GLGetGpuQueue(QueueType type) {
    switch (type) {
    case QueueType::GRAPHICS:
        return g_GraphicsQueue;
    case QueueType::COMPUTE:
        return g_ComputeQueue;
    case QueueType::TRANSFER:
        return g_TransferQueue;
    default:
        return nullptr;
    }
}

Swapchain* GLCreateSwapchain(const SwapchainDesc& desc) {
    PROFILE_FUNCTION();
    return new GLSwapchain(desc);
}

void GLDestroySwapchain(Swapchain* swapchain) {
    PROFILE_FUNCTION();
    delete swapchain;
}

} // namespace Rx::RxGL
