#pragma once
#include "RenderX/RenderXCommon.h"

namespace RenderX {

namespace RenderXVK {

#undef RENDERER_FUNC
#define RENDERER_FUNC(_ret, _name, ...) _ret VK##_name(__VA_ARGS__);
#include "RenderX/RenderXAPI.def"
#undef RENDERER_FUNC

} // namespace RenderXVK

} // namespace RenderX
