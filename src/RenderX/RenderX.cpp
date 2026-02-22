#include "RenderX/RenderX.h"
#include "RenderX/RX_Core.h"

#include <cassert>
#include <cstring>

#ifdef RX_ENABLE_OPENGL
#include "OpenGL/GL_RenderX.h"
#endif

#ifdef RX_ENABLE_VULKAN
#include "Vulkan/VK_RenderX.h"
#endif

namespace Rx {

RenderDispatchTable g_DispatchTable = {};
GraphicsAPI         API             = GraphicsAPI::NONE;

namespace {

void InitializeLoggingSystem() {
    LOG_INIT();
}

void ClearDispatchTable() {
    std::memset(&g_DispatchTable, 0, sizeof(g_DispatchTable));
}

void ShutdownActiveBackend() {
    if (API != GraphicsAPI::NONE && g_DispatchTable.BackendShutdown) {
        RENDERX_INFO("Shutting down active backend before reinitializing");
        g_DispatchTable.BackendShutdown();
        ClearDispatchTable();
        API = GraphicsAPI::NONE;
    }
}

bool InitializeOpenGLBackend(const InitDesc& window) {
#ifdef RX_ENABLE_OPENGL
    RENDERX_INFO("Initializing OpenGL backend...");

#define RX_BIND_FUNC(_ret, _name, _parms, _args) g_DispatchTable._name = RxGL::GL##_name;
    RENDERX_FUNC(RX_BIND_FUNC)
#undef RX_BIND_FUNC

    if (!g_DispatchTable.BackendInit) {
        RENDERX_ERROR("OpenGL BackendInit function pointer is null");
        ClearDispatchTable();
        return false;
    }

    g_DispatchTable.BackendInit(window);

    API = GraphicsAPI::OPENGL;
    RENDERX_INFO("OpenGL backend loaded successfully");
    return true;
#else
    RENDERX_ERROR("OpenGL support not compiled (RX_ENABLE_OPENGL not defined)");
    return false;
#endif
}

bool InitializeVulkanBackend(const InitDesc& window) {
#ifdef RX_ENABLE_VULKAN
    RENDERX_INFO("Initializing Vulkan backend...");

#define RX_BIND_FUNC(_ret, _name, _parms, _args) g_DispatchTable._name = RxVK::VK##_name;
    RENDERX_FUNC(RX_BIND_FUNC)
#undef RX_BIND_FUNC

    // Validate and call backend initialization
    if (!g_DispatchTable.BackendInit) {
        RENDERX_ERROR("Vulkan BackendInit function pointer is null");
        ClearDispatchTable();
        return false;
    }

    g_DispatchTable.BackendInit(window);
    API = GraphicsAPI::VULKAN;
    RENDERX_INFO("Vulkan backend loaded successfully");
    return true;
#else
    RENDERX_ERROR("Vulkan support not compiled (RX_ENABLE_VULKAN not defined)");
    return false;
#endif
}

} // anonymous namespace

void Init(const InitDesc& window) {
    InitializeLoggingSystem();
    ShutdownActiveBackend();
    switch (window.api) {
    case GraphicsAPI::OPENGL:
        InitializeOpenGLBackend(window);
        break;
    case GraphicsAPI::VULKAN:
        InitializeVulkanBackend(window);
        break;
    case GraphicsAPI::NONE:
        RENDERX_WARN("GraphicsAPI::NONE selected - no rendering backend loaded");
        break;
    default:
        RENDERX_ERROR("Unknown GraphicsAPI requested: {}", static_cast<int>(window.api));
        break;
    }
}

void Shutdown() {
    // Call the active backend's shutdown if available
    if (g_DispatchTable.BackendShutdown) {
        RENDERX_INFO("Shutting down RenderX backend");
        g_DispatchTable.BackendShutdown();
    }
    ClearDispatchTable();
    API = GraphicsAPI::NONE;
    LOG_SHUTDOWN();
}

#define RX_FORWARD_FUNC(_ret, _name, _parms, _args)                                                                              \
    _ret _name _parms {                                                                                                          \
        RENDERX_ASSERT_MSG(g_DispatchTable._name != nullptr, "Function not initialized");                                        \
        return g_DispatchTable._name _args;                                                                                      \
    }
RENDERX_FUNC(RX_FORWARD_FUNC)
#undef RX_FORWARD_FUNC

} // namespace Rx