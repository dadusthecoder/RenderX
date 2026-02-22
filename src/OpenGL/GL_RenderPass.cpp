#include "GL_Common.h"

namespace Rx::RxGL {

RenderPassHandle GLCreateRenderPass(const RenderPassDesc& desc) {
    PROFILE_FUNCTION();
    RenderPassHandle handle{GLNextHandle()};
    g_RenderPasses.emplace(handle.id, GLRenderPassResource{desc});
    return handle;
}

void GLDestroyRenderPass(RenderPassHandle& pass) {
    PROFILE_FUNCTION();
    g_RenderPasses.erase(pass.id);
    pass.id = 0;
}

} // namespace Rx::RxGL
