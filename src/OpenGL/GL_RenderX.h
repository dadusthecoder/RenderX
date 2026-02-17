#pragma once
#include "RenderX/Common.h"

namespace Rx {

namespace RxGL {

// Declare backend functions matching RenderXAPI.def
#undef RENDERER_FUNC
#define RX_FUNC_BACKEND_GL_DECAL(_ret, _name, _parms, _args) _ret GL##_name _parms;
RENDERX_FUNC(RX_FUNC_BACKEND_GL_DECAL)
#undef X

void                GLBindPipeline(const PipelineHandle pipeline);
const PipelineDesc* GLGetPipelineDesc(const PipelineHandle pipeline);
void                GLClearPipelineCache();
void                GLClearVAOCache();

} // namespace RxGL

} // namespace Rx
