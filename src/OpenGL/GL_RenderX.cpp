#include "GL_Common.h"

namespace Rx::RxGL {
namespace {

bool SetupPixelFormat(HDC dc) {
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cColorBits   = 32;
    pfd.cDepthBits   = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType   = PFD_MAIN_PLANE;

    const int existing = GetPixelFormat(dc);
    if (existing != 0) {
        return true;
    }

    const int fmt = ChoosePixelFormat(dc, &pfd);
    if (fmt == 0) {
        return false;
    }

    return SetPixelFormat(dc, fmt, &pfd) == TRUE;
}

void ClearAllResources() {
    g_Buffers.clear();
    g_BufferViews.clear();
    g_Textures.clear();
    g_TextureViews.clear();
    g_Shaders.clear();
    g_Pipelines.clear();
    g_PipelineLayouts.clear();
    g_RenderPasses.clear();
    g_Framebuffers.clear();
    g_SetLayouts.clear();
    g_DescriptorPools.clear();
    g_Sets.clear();
    g_DescriptorHeaps.clear();
    g_Samplers.clear();
}

} // namespace

void GLBackendInit(const InitDesc& window) {
    PROFILE_FUNCTION();

    if (!window.nativeWindowHandle) {
        RENDERX_ERROR("OpenGL BackendInit: InitDesc::nativeWindowHandle is null");
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    if (!g_GraphicsQueue) {
        g_GraphicsQueue = new GLCommandQueue(QueueType::GRAPHICS);
        g_ComputeQueue  = new GLCommandQueue(QueueType::COMPUTE);
        g_TransferQueue = new GLCommandQueue(QueueType::TRANSFER);
    }

    RENDERX_INFO("OpenGL backend initialized");
}

void GLBackendShutdown() {
    PROFILE_FUNCTION();

    delete g_GraphicsQueue;
    g_GraphicsQueue = nullptr;

    delete g_ComputeQueue;
    g_ComputeQueue = nullptr;

    delete g_TransferQueue;
    g_TransferQueue = nullptr;

    ClearAllResources();

    RENDERX_INFO("OpenGL backend shutdown complete");
}

} // namespace Rx::RxGL
