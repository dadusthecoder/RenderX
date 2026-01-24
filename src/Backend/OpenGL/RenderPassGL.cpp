#include "CommonGL.h"
namespace RenderX {
namespace RenderXGL {
	// temp
	RenderPassHandle GLCreateRenderPass(const RenderPassDesc&) {
		RENDERX_ASSERT(false && "OpenGL RenderPass not implemented yet");
		return {};
	}

	void GLDestroyRenderPass(RenderPassHandle&) {}

	void GLCmdBeginRenderPass(
		CommandList&,
		RenderPassHandle,
		const ClearValue* clears , uint32_t clearCount) {
		// no-op for now
	}

	void GLCmdEndRenderPass(CommandList&) {}
} // namespace RenderXGl


} // namespace  RenderX
