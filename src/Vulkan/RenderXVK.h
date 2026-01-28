#pragma once
#include "RenderX/RenderXCommon.h"

namespace Rx {

namespace RxVK {

#undef RENDERER_FUNC
#define RENDERER_FUNC(_ret, _name, ...) _ret VK##_name(__VA_ARGS__);
#include "RenderX/RenderXAPI.def"
#undef RENDERER_FUNC

} // namespace RxVK

} // namespace Rx
