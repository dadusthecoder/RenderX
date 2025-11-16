#include "RenderX/RenderX.h"
#include "RenderX/RenderXCore.h"
#include "Backend/OpenGL/RenderXGL.h"
#include "Log.h"
#include <cstring>

namespace Lgt {

	// Global Variables
	RenderDispatchTable g_DispatchTable;
	RenderXAPI API = RenderXAPI::None;

	void LoadAPI(RenderXAPI api) {
		RENDERX_LOG_INIT();
		RENDERX_INFO("Loading Renderer API...");

		switch (api) {
		case RenderXAPI::OpenGL: {
			RENDERX_INFO("Initializing OpenGL backend...");
#define RENDERER_FUNC(_ret, _name, ...)g_DispatchTable._name = RenderXGL::GL##_name;
#include "RenderX/RenderXAPI.def"
#undef RENDERER_FUNC

			if (g_DispatchTable.Init)
				g_DispatchTable.Init();

				API = api;
			RENDERX_INFO("OpenGL backend loaded successfully.");
			break;
		}

		case RenderXAPI::None:
			RENDERX_WARN("RendererAPI::None selected no rendering backend loaded.");
			break;

		default:
			RENDERX_ERROR("Unknown Renderer API requested!");
			break;
		}
	}


} // namespace Lng
