#pragma once

#include "RenderX/RX_Common.h"

namespace Rx::RxGL {

#define RX_FUNC_BACKEND_GL_DECAL(_ret, _name, _parms, _args) _ret GL##_name _parms;
RENDERX_FUNC(RX_FUNC_BACKEND_GL_DECAL)
#undef RX_FUNC_BACKEND_GL_DECAL

void                GLBindPipeline(const PipelineHandle pipeline);
const PipelineDesc* GLGetPipelineDesc(const PipelineHandle pipeline);
void                GLClearPipelineCache();

} // namespace Rx::RxGL
