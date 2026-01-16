#include "RenderX/RenderX.h"
#include "RenderX/RenderXCore.h"
#include "Backend/OpenGL/RenderXGL.h"
#include "Backend/Vulkan/RenderXVK.h"
#include "Log.h"
#include "ProLog/ProLog.h"
#include <cstring>

namespace RenderX {

	// Global Variables
	RenderDispatchTable g_DispatchTable;
	GraphicsAPI API = GraphicsAPI::None;

	void SetBackend(GraphicsAPI api) {
		RENDERX_INFO("Loading Renderer API...");
		PROFILE_FUNCTION();
		switch (api) {
		case GraphicsAPI::OpenGL: {
			RENDERX_INFO("Initializing OpenGL backend...");
			// #define RENDERER_FUNC(_ret, _name, ...) g_DispatchTable._name = RenderXGL::GL##_name;
			// #include "RenderX/RenderXAPI.def"
			// #undef RENDERER_FUNC

			if (g_DispatchTable.Init)
				g_DispatchTable.Init();

			API = api;
			RENDERX_INFO("OpenGL backend loaded successfully.");
			break;
		}


		case GraphicsAPI::Vulkan: {
			RENDERX_INFO("Initializing Vulkan backend...");
#define RENDERER_FUNC(_ret, _name, ...) g_DispatchTable._name = RenderXVK::VK##_name;
#include "RenderX/RenderXAPI.def"
#undef RENDERER_FUNC

			if (g_DispatchTable.Init)
				g_DispatchTable.Init();

			API = api;
			RENDERX_INFO("Vulkan backend loaded successfully.");
			break;
		}

		case GraphicsAPI::None:
			RENDERX_WARN("RendererAPI::None selected no rendering backend loaded.");
			break;

		default:
			RENDERX_ERROR("Unknown Renderer API requested!");
			break;
		}
	}

	void Init() {
		ProLog::ProfilerConfig config;
		config.enableProfiling = true;
		config.enableLogging = true;
		config.bufferSize = 500;
		config.autoFlush = true;
		ProLog::SetConfig(config);
		PROFILE_START_SESSION("RenderX", "RenderX.json");
		RENDERX_LOG_INIT();
		return ;
	}

	void ShutDown() {
		PROFILE_PRINT_STATS();
		PROFILE_END_SESSION();
		return;
	}

	// API Functions
	ShaderHandle CreateShader(const ShaderDesc& desc) { return g_DispatchTable.CreateShader(desc); }
	const PipelineHandle CreateGraphicsPipeline(PipelineDesc& desc) { return g_DispatchTable.CreateGraphicsPipeline(desc); }
	const BufferHandle CreateBuffer(const BufferDesc& desc) { return g_DispatchTable.CreateBuffer(desc); }
	bool ShouldClose() { return g_DispatchTable.ShouldClose(); }
	CommandList CreateCommandList() { return g_DispatchTable.CreateCommandList(); }
	void DestroyCommandList(const CommandList& cmdList) { g_DispatchTable.DestroyCommandList(cmdList); }
	void ExecuteCommandList(const CommandList& cmdList) { g_DispatchTable.ExecuteCommandList(cmdList); }

} // namespace RenderX
