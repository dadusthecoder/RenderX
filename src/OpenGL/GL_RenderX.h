#pragma once
#include "RenderX/RenderXCommon.h"

namespace Rx {

namespace RxGL {

// Declare backend functions matching RenderXAPI.def
#undef X
#define X(_ret, _name, ...) _ret GL##_name(__VA_ARGS__);
	RENDERX_API(X)
#undef X

	void GLBindPipeline(const PipelineHandle pipeline);
	const PipelineDesc* GLGetPipelineDesc(const PipelineHandle pipeline);
	void GLClearPipelineCache();
	void GLClearVAOCache();

} // namespace RxGL

} // namespace Rx
