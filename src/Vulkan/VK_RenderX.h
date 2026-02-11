#pragma once
#include "RenderX/Common.h"

namespace Rx {

namespace RxVK {

#undef RENDERER_FUNC
#define RX_FUNC_BACKEND_VK_DECAL(_ret, _name, _parms, _args) _ret VK##_name _parms;
	RENDERX_FUNC(RX_FUNC_BACKEND_VK_DECAL)
#undef X

} // namespace RxVK

} // namespace Rx
