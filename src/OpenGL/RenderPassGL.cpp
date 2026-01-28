#include "CommonGL.h"
namespace Rx {
namespace RxGL {
	// temp
	RenderPassHandle GLCreateRenderPass(const RenderPassDesc&) {
		RENDERX_ASSERT(false && "OpenGL RenderPass not implemented yet");
		return {};
	}


	void GLDestroyRenderPass(RenderPassHandle&) {}

	RenderPassHandle GLGetDefaultRenderPass() {
		RENDERX_WARN("GLGetDefaultRenderPass Not Implemented yet");
		return RenderPassHandle{};
	}

	void GLCmdBeginRenderPass(
		CommandList&,
		RenderPassHandle,
		const ClearValue* clears, uint32_t clearCount) {
		// no-op for now
	}

	void GLCmdEndRenderPass(CommandList&) {}
} // namespace RxGl


} // namespace  Rx
