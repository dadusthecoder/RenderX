#include "RenderX/RenderX.h"
#include "RenderX/Core.h"
#include "RenderX/DebugProfiler.h"
#include "OpenGL/GL_RenderX.h"
#include "Vulkan/VK_RenderX.h"
#include "Log.h"
#include "ProLog/ProLog.h"
#include <cstring>

namespace Rx {

	// Global Variables
	RenderDispatchTable g_DispatchTable;
	GraphicsAPI API = GraphicsAPI::NONE;

	void Init(const Window& window) {
#ifdef RENDERX_DEBUG
		LOG_INIT();
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
		if (API != GraphicsAPI::NONE && g_DispatchTable.Shutdown) {
			RENDERX_INFO("Init: shutting down active backend before reinitializing");
			g_DispatchTable.Shutdown();
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

			if (g_DispatchTable.Init) g_DispatchTable.Init(window);

			API = window.api;
			RENDERX_INFO("OpenGL backend loaded successfully.");
			break;
		}

		case GraphicsAPI::VULKAN: {
			RENDERX_INFO("Initializing Vulkan backend...");
#define RX_BIND_FUNC(_ret, _name, _parms, _args) g_DispatchTable._name = RxVK::VK##_name;
			RENDERX_FUNC(RX_BIND_FUNC)
#undef X

			if (g_DispatchTable.Init)
				g_DispatchTable.Init(window);

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
		if (g_DispatchTable.Shutdown) {
			g_DispatchTable.Shutdown();
		}
		// Reset dispatch table and API so a new backend can be initialized safely.
		std::memset(&g_DispatchTable, 0, sizeof(g_DispatchTable));
		API = GraphicsAPI::NONE;

		PROFILE_END_SESSION();
		LOG_SHUTDOWN();
	}

#define RX_FORWARD(_ret, _name, _parms, _args) \
	_ret _name _parms { return g_DispatchTable._name _args; }

	RX_FORWARD(PipelineLayoutHandle, CreatePipelineLayout, (const ResourceGroupLayoutHandle* layouts, uint32_t layoutCount), (layouts, layoutCount))
	RX_FORWARD(PipelineHandle, CreateGraphicsPipeline, (PipelineDesc & desc), (desc))
	RX_FORWARD(ShaderHandle, CreateShader, (const ShaderDesc& desc), (desc))
	RX_FORWARD(BufferHandle, CreateBuffer, (const BufferDesc& desc), (desc))
	RX_FORWARD(BufferViewHandle, CreateBufferView, (const BufferViewDesc& desc), (desc))
	RX_FORWARD(void, DestroyBufferView, (BufferViewHandle & handle), (handle))
	RX_FORWARD(RenderPassHandle, CreateRenderPass, (const RenderPassDesc& desc), (desc))
	RX_FORWARD(void, DestroyRenderPass, (RenderPassHandle & pass), (pass))
	RX_FORWARD(FramebufferHandle, CreateFramebuffer, (const FramebufferDesc& desc), (desc))
	RX_FORWARD(void, DestroyFramebuffer, (FramebufferHandle & framebuffer), (framebuffer))
	RX_FORWARD(ResourceGroupHandle, CreateResourceGroup, (const ResourceGroupDesc& desc), (desc))
	RX_FORWARD(void, DestroyResourceGroup, (ResourceGroupHandle & handle), (handle))
	RX_FORWARD(ResourceGroupLayoutHandle, CreateResourceGroupLayout, (const ResourceGroupLayout& desc), (desc))

} // namespace Rx
