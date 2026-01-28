#include "CommonGL.h"

namespace Rx {
namespace RxGL {

	FramebufferHandle GLCreateFramebuffer(const FramebufferDesc&) {
		RENDERX_ASSERT(false && "OpenGL Framebuffer not implemented yet");
		return {};
	}

	void GLDestroyFramebuffer(FramebufferHandle&) {}
} // namespace RxGL

} // namespace Rx
