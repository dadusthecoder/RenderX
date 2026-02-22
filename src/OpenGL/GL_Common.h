#pragma once

#include "RenderX/RX_Common.h"
#include "GL_RenderX.h"
#include "ProLog/ProLog.h"

// GL_Common.h
#pragma once

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

#include <GL/gl.h>

#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Rx::RxGL {

struct GLBufferResource {
    BufferDesc           desc{};
    std::vector<uint8_t> bytes{};
};

struct GLTextureResource {
    TextureDesc          desc{};
    std::vector<uint8_t> bytes{};
};

struct GLTextureViewResource {
    TextureViewDesc desc{};
};

struct GLShaderResource {
    ShaderDesc desc{};
};

struct GLPipelineResource {
    PipelineDesc desc{};
};

struct GLPipelineLayoutResource {
    std::vector<SetLayoutHandle>   layouts{};
    std::vector<PushConstantRange> pushRanges{};
};

struct GLRenderPassResource {
    RenderPassDesc desc{};
};

struct GLFramebufferResource {
    FramebufferDesc desc{};
};

struct GLSetLayoutResource {
    SetLayoutDesc desc{};
};

struct GLDescriptorPoolResource {
    DescriptorPoolDesc     desc{};
    std::vector<SetHandle> sets{};
};

struct GLSetResource {
    SetLayoutHandle                               layout{};
    std::unordered_map<uint32_t, DescriptorWrite> writes{};
};

struct GLDescriptorHeapResource {
    DescriptorHeapDesc   desc{};
    std::vector<uint8_t> bytes{};
};

struct GLSamplerResource {
    SamplerDesc desc{};
};

struct GLCommandState {
    PipelineHandle pipeline{};
    BufferHandle   vertexBuffer{};
    BufferHandle   indexBuffer{};
    uint64_t       vertexOffset = 0;
    uint64_t       indexOffset  = 0;
    Format         indexType    = Format::UINT32;

    Viewport viewport{};
    bool     hasViewport = false;
    Scissor  scissor{};
    bool     hasScissor = false;

    bool       inRenderPass = false;
    bool       inRendering  = false;
    ClearColor clearColor   = ClearColor::Black();

    uint32_t vertexCount   = 0;
    uint32_t instanceCount = 1;
    uint32_t firstVertex   = 0;
    uint32_t firstInstance = 0;

    uint32_t indexCount      = 0;
    int32_t  vertexOffsetIdx = 0;
    uint32_t firstIndex      = 0;
};

class GLCommandList final : public CommandList {
public:
    GLCommandState state{};

    void open() override;
    void close() override;
    void setPipeline(const PipelineHandle& pipeline) override;
    void setVertexBuffer(const BufferHandle& buffer, uint64_t offset) override;
    void setIndexBuffer(const BufferHandle& buffer, uint64_t offset, Format indextype) override;
    void setFramebuffer(FramebufferHandle handle) override;
    void setViewport(const Viewport& viewport) override;
    void setScissor(const Scissor& scissor) override;
    void beginRenderPass(RenderPassHandle pass, const void* clearValues, uint32_t clearCount) override;
    void endRenderPass() override;
    void beginRendering(const RenderingDesc& desc) override;
    void endRendering() override;
    void writeBuffer(BufferHandle handle, const void* data, uint32_t offset, uint32_t size) override;
    void copyBuffer(BufferHandle src, BufferHandle dst, const BufferCopy& region) override;
    void copyTexture(TextureHandle srcTexture, TextureHandle dstTexture, const TextureCopy& region) override;
    void copyBufferToTexture(BufferHandle srcBuffer, TextureHandle dstTexture, const TextureCopy& region) override;
    void copyTextureToBuffer(TextureHandle srcTexture, BufferHandle dstBuffer, const TextureCopy& region) override;
    void Barrier(const Memory_Barrier* memoryBarriers,
                 uint32_t              memoryCount,
                 const BufferBarrier*  bufferBarriers,
                 uint32_t              bufferCount,
                 const TextureBarrier* imageBarriers,
                 uint32_t              imageCount) override;

    void drawIndexed(
        uint32_t indexCount, int32_t vertexOffset, uint32_t instanceCount, uint32_t firstIndex, uint32_t firstInstance) override;
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;

    void setDescriptorSet(uint32_t slot, SetHandle set) override;
    void setDescriptorSets(uint32_t firstSlot, const SetHandle* sets, uint32_t count) override;
    void setBindlessTable(BindlessTableHandle table) override;
    void pushConstants(uint32_t slot, const void* data, uint32_t sizeIn32BitWords, uint32_t offsetIn32BitWords) override;
    void setDescriptorHeaps(DescriptorHeapHandle* heaps, uint32_t count) override;
    void setInlineCBV(uint32_t slot, BufferHandle buf, uint64_t offset) override;
    void setInlineSRV(uint32_t slot, BufferHandle buf, uint64_t offset) override;
    void setInlineUAV(uint32_t slot, BufferHandle buf, uint64_t offset) override;
    void setDescriptorBufferOffset(uint32_t slot, uint32_t bufferIndex, uint64_t byteOffset) override;
    void setDynamicOffset(uint32_t slot, uint32_t byteOffset) override;
    void pushDescriptor(uint32_t slot, const DescriptorWrite* writes, uint32_t count) override;
};

class GLCommandAllocator final : public CommandAllocator {
public:
    CommandList* Allocate() override;
    void         Reset(CommandList* list) override;
    void         Free(CommandList* list) override;
    void         Reset() override;
};

class GLCommandQueue final : public CommandQueue {
public:
    explicit GLCommandQueue(QueueType t);

    CommandAllocator* CreateCommandAllocator(const char* debugName) override;
    void              DestroyCommandAllocator(CommandAllocator* allocator) override;

    Timeline Submit(CommandList* commandList) override;
    Timeline Submit(const SubmitInfo& submitInfo) override;

    bool     Wait(Timeline value, uint64_t timeout) override;
    void     WaitIdle() override;
    bool     Poll(Timeline value) override;
    Timeline Completed() override;
    Timeline Submitted() const override;

    float TimestampFrequency() const override;

private:
    QueueType m_Type;
    uint64_t  m_Submitted = 0;
    uint64_t  m_Completed = 0;
};

class GLSwapchain final : public Swapchain {
public:
    explicit GLSwapchain(const SwapchainDesc& desc);

    uint32_t          AcquireNextImage() override;
    void              Present(uint32_t imageIndex) override;
    void              Resize(uint32_t width, uint32_t height) override;
    Format            GetFormat() const override;
    uint32_t          GetWidth() const override;
    uint32_t          GetHeight() const override;
    uint32_t          GetImageCount() const override;
    TextureHandle     GetImage(uint32_t imageIndex) const override;
    TextureHandle     GetDepth(uint32_t imageIndex) const override;
    TextureViewHandle GetImageView(uint32_t imageIndex) const override;
    TextureViewHandle GetDepthView(uint32_t imageIndex) const override;

private:
    void rebuildImages();

    uint32_t                       m_Width  = 0;
    uint32_t                       m_Height = 0;
    uint32_t                       m_Count  = 0;
    Format                         m_Format = Format::BGRA8_SRGB;
    uint32_t                       m_Index  = 0;
    std::vector<TextureHandle>     m_Images{};
    std::vector<TextureHandle>     m_Depths{};
    std::vector<TextureViewHandle> m_ImageViews{};
    std::vector<TextureViewHandle> m_DepthViews{};
};

#if defined(_WIN32)
extern int   g_WindowWidth;
extern int   g_WindowHeight;
#else
extern int   g_WindowWidth;
extern int   g_WindowHeight;
#endif

    extern std::atomic<uint64_t>
        g_NextHandleId;

extern std::unordered_map<uint64_t, GLBufferResource>         g_Buffers;
extern std::unordered_map<uint64_t, BufferViewDesc>           g_BufferViews;
extern std::unordered_map<uint64_t, GLTextureResource>        g_Textures;
extern std::unordered_map<uint64_t, GLTextureViewResource>    g_TextureViews;
extern std::unordered_map<uint64_t, GLShaderResource>         g_Shaders;
extern std::unordered_map<uint64_t, GLPipelineResource>       g_Pipelines;
extern std::unordered_map<uint64_t, GLPipelineLayoutResource> g_PipelineLayouts;
extern std::unordered_map<uint64_t, GLRenderPassResource>     g_RenderPasses;
extern std::unordered_map<uint64_t, GLFramebufferResource>    g_Framebuffers;
extern std::unordered_map<uint64_t, GLSetLayoutResource>      g_SetLayouts;
extern std::unordered_map<uint64_t, GLDescriptorPoolResource> g_DescriptorPools;
extern std::unordered_map<uint64_t, GLSetResource>            g_Sets;
extern std::unordered_map<uint64_t, GLDescriptorHeapResource> g_DescriptorHeaps;
extern std::unordered_map<uint64_t, GLSamplerResource>        g_Samplers;

extern GLCommandQueue* g_GraphicsQueue;
extern GLCommandQueue* g_ComputeQueue;
extern GLCommandQueue* g_TransferQueue;

uint64_t GLNextHandle();
void     GLExecuteCommandList(GLCommandList& cmdList);

void                GLBindPipeline(const PipelineHandle pipeline);
const PipelineDesc* GLGetPipelineDesc(const PipelineHandle pipeline);
void                GLClearPipelineCache();

} // namespace Rx::RxGL
