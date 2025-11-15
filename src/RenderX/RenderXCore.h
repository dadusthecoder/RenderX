#pragma once
#include "RenderX.h"

namespace Lgt {

#define RENDERER_FUNC(_ret, _name, ...)inline _ret _name(__VA_ARGS__);
RENDERER_FUNC(void,BindPipeline, const PipelineHandle handle)
RENDERER_FUNC(void,Draw,uint32_t vertexCount)
#include "RenderXAPI.def"
#undef RENDERER_FUNC

	//------------------------------------------------------------------------------
	// RENDER DISPATCH TABLE
	//------------------------------------------------------------------------------

	/**
	 * @struct RenderDispatchTable
	 * @brief Function table holding the active backend�s GPU entry points.
	 *
	 * When a graphics API is loaded, its function pointers populate this table,
	 * allowing the engine to remain API-agnostic at runtime.
	 */

	struct RenderDispatchTable {
#define RENDERER_FUNC(_ret, _name, ...) decltype(&_name) _name;
#include "RenderXAPI.def"
#undef RENDERER_FUNC
	};

	//------------------------------------------------------------------------------
	// GLOBAL STATE
	//------------------------------------------------------------------------------

	/// @brief Global dispatch table instance bound to the active backend.
	extern RenderDispatchTable RENDERX_API g_DispatchTable;

	/// @brief Enum representing the currently active rendering backend.
	extern RenderXAPI RENDERX_API API;
}