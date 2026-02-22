#include "GL_Common.h"

namespace Rx::RxGL {

int   g_WindowWidth   = 0;
int   g_WindowHeight  = 0;

std::atomic<uint64_t> g_NextHandleId{1};

std::unordered_map<uint64_t, GLBufferResource>         g_Buffers;
std::unordered_map<uint64_t, BufferViewDesc>           g_BufferViews;
std::unordered_map<uint64_t, GLTextureResource>        g_Textures;
std::unordered_map<uint64_t, GLTextureViewResource>    g_TextureViews;
std::unordered_map<uint64_t, GLShaderResource>         g_Shaders;
std::unordered_map<uint64_t, GLPipelineResource>       g_Pipelines;
std::unordered_map<uint64_t, GLPipelineLayoutResource> g_PipelineLayouts;
std::unordered_map<uint64_t, GLRenderPassResource>     g_RenderPasses;
std::unordered_map<uint64_t, GLFramebufferResource>    g_Framebuffers;
std::unordered_map<uint64_t, GLSetLayoutResource>      g_SetLayouts;
std::unordered_map<uint64_t, GLDescriptorPoolResource> g_DescriptorPools;
std::unordered_map<uint64_t, GLSetResource>            g_Sets;
std::unordered_map<uint64_t, GLDescriptorHeapResource> g_DescriptorHeaps;
std::unordered_map<uint64_t, GLSamplerResource>        g_Samplers;

GLCommandQueue* g_GraphicsQueue = nullptr;
GLCommandQueue* g_ComputeQueue  = nullptr;
GLCommandQueue* g_TransferQueue = nullptr;

uint64_t GLNextHandle() {
    return g_NextHandleId.fetch_add(1, std::memory_order_relaxed);
}

} // namespace Rx::RxGL
