#include "GL_Common.h"

namespace Rx {
namespace RxGL {

	// Simple in-memory store for resource groups used by the OpenGL backend.
	static std::unordered_map<uint64_t, ResourceGroupDesc> s_ResourceGroups;
	static uint32_t s_NextResourceGroupIndex = 1;

} // namespace RxGL
} // namespace Rx

Rx::ResourceGroupHandle Rx::RxGL::GLCreateResourceGroup(const ResourceGroupDesc& desc) {
	PROFILE_FUNCTION();
	ResourceGroupHandle handle;
	
	if (!desc.layout.IsValid()) {
		RENDERX_WARN("GLCreateResourceGroup: Invalid pipeline layout");
		return handle;
	}

	uint64_t id = (static_cast<uint64_t>(2) << 32) | static_cast<uint64_t>(s_NextResourceGroupIndex++);
	s_ResourceGroups[id] = desc;
	handle.id = id;
	
	RENDERX_INFO("GL: Created ResourceGroup (id={})", handle.id);
	return handle;
}

void Rx::RxGL::GLDestroyResourceGroup(ResourceGroupHandle& handle) {
	PROFILE_FUNCTION();
	if (!handle.IsValid()) return;
	
	auto it = s_ResourceGroups.find(handle.id);
	if (it != s_ResourceGroups.end()) {
		s_ResourceGroups.erase(it);
		RENDERX_INFO("GL: Destroyed ResourceGroup (id={})", handle.id);
	}
	
	handle.id = 0;
}