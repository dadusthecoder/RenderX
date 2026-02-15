#include "GL_Common.h"

namespace Rx {
namespace RxGL {

	FramebufferHandle GLCreateFramebuffer(const FramebufferDesc&) {
		PROFILE_FUNCTION();
		// Minimal implementation: Framebuffers are not required for basic on-screen rendering
		// Return an empty handle to indicate default framebuffer should be used.
		FramebufferHandle fb{};
		RENDERX_INFO("GL: CreateFramebuffer - returning default (no-op)");
		return fb;
	}

	void GLDestroyFramebuffer(FramebufferHandle& fb) {
		PROFILE_FUNCTION();
		fb.id = 0;
	}
} // namespace RxGL

} // namespace Rx
