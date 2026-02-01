
#include "CommonGL.h"
#include <atomic>

namespace Rx {
namespace RxGL {

	static std::atomic<uint32_t> s_NextRenderPassId{ 1 };
	static std::vector<ClearValue> s_PendingClearValues;

	RenderPassHandle GLCreateRenderPass(const RenderPassDesc&) {
		PROFILE_FUNCTION();
		// For the simple OpenGL backend we don't need explicit renderpasses.
		// Return an empty/default handle to indicate default framebuffer usage.
		RenderPassHandle h{};
		RENDERX_INFO("GL: CreateRenderPass - returning default (no-op)");
		return h;
	}

	void GLDestroyRenderPass(RenderPassHandle& handle) {
		PROFILE_FUNCTION();
		handle.id = 0;
	}

	RenderPassHandle GLGetDefaultRenderPass() {
		// Default render pass is represented by an empty handle here
		return RenderPassHandle{};
	}

	void GLCmdBeginRenderPass(
		CommandList&,
		RenderPassHandle,
		const ClearValue* clears, uint32_t clearCount) {
		PROFILE_FUNCTION();
		// Store clear values to be applied during render pass
		s_PendingClearValues.clear();
		if (clears && clearCount > 0) {
			s_PendingClearValues.insert(s_PendingClearValues.end(), clears, clears + clearCount);
		}

		// Apply the clear immediately
		if (!s_PendingClearValues.empty()) {
			const auto& clearColor = s_PendingClearValues[0].color.color;
				clearColor.x, clearColor.y, clearColor.z, clearColor.w;
				glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
		}
		else {
			// Default clear color if no clear value provided
			glClearColor(0.1f, 0.01f, 0.01f, 1.0f);
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void GLCmdEndRenderPass(CommandList&) {}

	// Helper to get pending clear values
	const std::vector<ClearValue>& GLGetPendingClearValues() {
		return s_PendingClearValues;
	}
} // namespace RxGl


} // namespace  Rx
