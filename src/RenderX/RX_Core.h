#pragma once
#include "RenderX.h"

namespace Rx {
// RENDER DISPATCH TABLE
/**
 * @struct RenderDispatchTable
 * @brief Function table holding the active backends GPU entry points.
 *
 * When a graphics API is loaded, its function pointers populate this table,
 * allowing the engine to remain API-agnostic at runtime.
 */

struct RenderDispatchTable {
#define RX_DESPATCH_TABLE_FUNC(_ret, _name, _parms, _args)                                                             \
    using _name##_func_t = _ret(*) _parms;                                                                             \
    _name##_func_t _name;
    RENDERX_FUNC(RX_DESPATCH_TABLE_FUNC)
#undef RX_DESPATCH_TABLE_FUNC
};

// GLOBAL STATE
/// @brief Global dispatch table instance bound to the active backend.
extern RENDERX_EXPORT RenderDispatchTable g_DispatchTable;

/// @brief Enum representing the currently active rendering backend.
extern RENDERX_EXPORT GraphicsAPI API;
} // namespace Rx