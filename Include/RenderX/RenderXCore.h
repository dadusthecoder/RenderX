#pragma once
#include "RenderX.h"

namespace Rx {
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
#define RENDERER_FUNC(_ret, _name, ...)           \
	using _name##_func_t = _ret (*)(__VA_ARGS__); \
	_name##_func_t _name;
#include "RenderXAPI.def"
#undef RENDERER_FUNC
	};

	//------------------------------------------------------------------------------
	// GLOBAL STATE
	//------------------------------------------------------------------------------

	/// @brief Global dispatch table instance bound to the active backend.
	extern RenderDispatchTable RENDERX_API g_DispatchTable;

	/// @brief Enum representing the currently active rendering backend.
	extern GraphicsAPI RENDERX_API API;
}