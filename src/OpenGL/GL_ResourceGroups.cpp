#include "GL_Common.h"

namespace Rx {
namespace RxGL {

	// Simple in-memory store for pipeline layouts used by the OpenGL backend.
	static std::unordered_map<uint64_t, ResourceGroupLayout> s_PipelineLayouts;
	static uint32_t s_NextLayoutIndex = 1;

	void GLCmdSetResourceGroup(const CommandList& list, const ResourceGroupHandle& handel) {
		return;
	}

	PipelineLayoutHandle GLCreatePipelineLayout(const ResourceGroupLayout* pLayouts, uint32_t layoutCount) {
		PROFILE_FUNCTION();
		PipelineLayoutHandle handle;
		if (!pLayouts || layoutCount == 0) {
			// Return an empty handle when nothing provided
			return handle;
		}

		uint64_t id = (static_cast<uint64_t>(1) << 32) | static_cast<uint64_t>(s_NextLayoutIndex++);
		// Store the first layout provided (most simple use-cases use a single layout)
		s_PipelineLayouts[id] = pLayouts[0];
		handle.id = id;
		RENDERX_INFO("GL: Created PipelineLayout (id={})", handle.id);
		return handle;
	}

	// Buffer view destroy is implemented in BufferGL.cpp - no duplicate here.

} // namespace  RxGL

} // namespace  Rx
