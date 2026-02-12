#include "RenderX/RenderX.h"
#include "RenderX/Core.h"
#include "RenderX/DebugProfiler.h"
#include "OpenGL/GL_RenderX.h"
#include "Vulkan/VK_RenderX.h"
#include "ProLog/ProLog.h"
#include <cstring>

namespace Rx {

	// Global Variables
	RenderDispatchTable g_DispatchTable;
	GraphicsAPI API = GraphicsAPI::NONE;

	void Init(const InitDesc& window) {
#ifdef RENDERX_DEBUG
		LOG_INIT();
		RENDERX_TRACE("mahesh");
		Debug::ConfigureDetailedProfiling();
#else
		ProLog::ProfilerConfig config;
		config.enableProfiling = true;
		config.enableLogging = true;
		config.bufferSize = 500;
		config.autoFlush = true;
		ProLog::SetConfig(config);
#endif
		// If a backend is already active, shut it down cleanly so we can switch.
		if (API != GraphicsAPI::NONE && g_DispatchTable.BackendShutdown) {
			RENDERX_INFO("Init: shutting down active backend before reinitializing");
			g_DispatchTable.BackendShutdown();
			// Clear function pointers so stale calls are impossible.
			std::memset(&g_DispatchTable, 0, sizeof(g_DispatchTable));
			API = GraphicsAPI::NONE;
		}

		switch (window.api) {
		case GraphicsAPI::OPENGL: {
			RENDERX_INFO("Initializing OpenGL backend...");
#define X(_ret, _name, ...) g_DispatchTable._name = RxGL::GL##_name;
			// RENDERX_API(X)
#undef X

			if (g_DispatchTable.BackendInit) g_DispatchTable.BackendInit(window);

			API = window.api;
			RENDERX_INFO("OpenGL backend loaded successfully.");
			break;
		}

		case GraphicsAPI::VULKAN: {
			RENDERX_INFO("Initializing Vulkan backend...");
#define RX_BIND_FUNC(_ret, _name, _parms, _args) g_DispatchTable._name = RxVK::VK##_name;
			RENDERX_FUNC(RX_BIND_FUNC)
#undef X

			if (g_DispatchTable.BackendInit)
				g_DispatchTable.BackendInit(window);

			API = window.api;
			RENDERX_INFO("Vulkan backend loaded successfully.");
			break;
		}

		case GraphicsAPI::NONE:
			RENDERX_WARN("RendererAPI::None selected no rendering backend loaded.");
			break;

		default:
			RENDERX_ERROR("Unknown Renderer API requested!");
			break;
		}
	}

	void Shutdown() {
		// Call the active backend's shutdown if available.
		if (g_DispatchTable.BackendShutdown) {
			g_DispatchTable.BackendShutdown();
		}
		// Reset dispatch table and API so a new backend can be initialized safely.
		std::memset(&g_DispatchTable, 0, sizeof(g_DispatchTable));
		API = GraphicsAPI::NONE;

		PROFILE_END_SESSION();
		LOG_SHUTDOWN();
	}

#define RX_FORWARD_FUNC(_ret, _name, _parms, _args) \
	_ret _name _parms { return g_DispatchTable._name _args; }
    RENDERX_FUNC(RX_FORWARD_FUNC)
#undef RX_FORWARD

} // namespace Rx
