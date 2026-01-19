#pragma once
#include "RenderX/RenderXCommon.h"

namespace RenderX {

namespace RenderXGL {

// Declare backend functions matching RenderXAPI.def
#undef RENDERER_FUNC
#define RENDERER_FUNC(_ret, _name, ...) _ret GL##_name(__VA_ARGS__);
#include "RenderX/RenderXAPI.def"
#undef RENDERER_FUNC

void GLBindPipeline(const PipelineHandle pipeline);
const PipelineDesc* GLGetPipelineDesc(const PipelineHandle pipeline);

} // namespace RenderXGL

} // namespace RenderX
