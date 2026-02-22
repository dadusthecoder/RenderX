#include "GL_Common.h"

namespace Rx::RxGL {

FramebufferHandle GLCreateFramebuffer(const FramebufferDesc& desc) {
    PROFILE_FUNCTION();
    FramebufferHandle handle{GLNextHandle()};
    g_Framebuffers.emplace(handle.id, GLFramebufferResource{desc});
    return handle;
}

void GLDestroyFramebuffer(FramebufferHandle& framebuffer) {
    PROFILE_FUNCTION();
    g_Framebuffers.erase(framebuffer.id);
    framebuffer.id = 0;
}

} // namespace Rx::RxGL
