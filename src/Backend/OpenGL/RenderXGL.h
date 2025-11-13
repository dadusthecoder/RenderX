#pragma once
#include "RenderX/RenderXTypes.h"
#include <string>

// sorting
//  rendercommands
namespace Lgt {

namespace RenderXGL {

// Declear Backend Functions
#define RENDERER_FUNC(_ret, _name, ...) _ret GL##_name(__VA_ARGS__);
#include "RenderX/RenderXAPI.def"
#undef RENDERER_FUNC

} // namespace RenderXGL

} // namespace Lng
