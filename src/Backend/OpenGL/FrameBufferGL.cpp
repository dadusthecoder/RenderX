#include "CommonGL.h"

namespace RenderX {
namespace RenderXGL {

	FramebufferHandle GLCreateFramebuffer(const FramebufferDesc&) {
		RENDERX_ASSERT(false && "OpenGL Framebuffer not implemented yet");
		return {};
	}

	void GLDestroyFramebuffer(FramebufferHandle&) {}
} // namespace RenderXGL

} // namespace RenderX
