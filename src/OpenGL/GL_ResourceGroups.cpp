#include "CommonGL.h"

namespace Rx {
namespace RxGL {
	ResourceGroupLayoutHandle GLCreateResourceGroupLayout(const ResourceGroupLayoutDesc& desc) {
		RENDERX_ASSERT(false && "OpenGL ResourceGroupLayout not implemented yet");
		(void)desc;
		return ResourceGroupLayoutHandle{};
	}
} // namespace  RxGL

} // namespace  Rx
