#pragma once
#include "RenderX/RenderXCommon.h"

namespace Rx {

namespace RxVK {

#undef RENDERER_FUNC
#define X(_ret, _name, ...) _ret VK##_name(__VA_ARGS__);
	RENDERX_API(X)
#undef X

} // namespace RxVK

} // namespace Rx
