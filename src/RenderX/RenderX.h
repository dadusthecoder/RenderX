#pragma once
#include "Common.h"


//------------------------------------------------------------------------------
// RENDERING HARDWARE INTERFACE (RHI)
//------------------------------------------------------------------------------
// Defines the runtime-dispatchable abstraction for graphics APIs.
// Backend modules (OpenGL(Not Functional - WIP), Vulkan(Unstable), DirectX(Planed), etc.) provide actual
// implementations routed through the dispatch table.
//------------------------------------------------------------------------------

namespace Rx {
#define RX_PUBLIC_FUNC_DECAL(_ret, _name, _parms, _args) RENDERX_EXPORT _ret _name _parms;
	RENDERX_FUNC(RX_PUBLIC_FUNC_DECAL)
#undef RX_PUBLIC_FUNC_DECAL
	RENDERX_EXPORT void Init(const Window& window);
	RENDERX_EXPORT void Shutdown();
} // namespace Rx
