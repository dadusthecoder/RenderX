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
#define RENDERER_FUNC(_ret, _name, ...) g_DispatchTable._name = RenderXGL::GL##_name;
#include "RenderX/RenderXAPI.def"
#undef RENDERER_FUNC

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
		return;
	}

	void ShutDown() {
		PROFILE_PRINT_STATS();
		PROFILE_END_SESSION();
		return;
	}

	// API Functions
	void Begin() { g_DispatchTable.Begin(); }
	void End() { g_DispatchTable.End(); }

	ShaderHandle CreateShader(const ShaderDesc& desc) { return g_DispatchTable.CreateShader(desc); }
	PipelineHandle CreateGraphicsPipeline(PipelineDesc& desc) { return g_DispatchTable.CreateGraphicsPipeline(desc); }

	RenderPassHandle CreateRenderPass(const RenderPassDesc& desc) { return g_DispatchTable.CreateRenderPass(desc); }
	void DestroyRenderPass(RenderPassHandle& pass) { g_DispatchTable.DestroyRenderPass(pass); }

	FramebufferHandle CreateFramebuffer(const FramebufferDesc& desc) { return g_DispatchTable.CreateFramebuffer(desc); }
	void DestroyFramebuffer(FramebufferHandle& framebuffer) { g_DispatchTable.DestroyFramebuffer(framebuffer); }

	BufferHandle CreateBuffer(const BufferDesc& desc) { return g_DispatchTable.CreateBuffer(desc); }
	bool ShouldClose() { return g_DispatchTable.ShouldClose(); }

	CommandList CreateCommandList() { return g_DispatchTable.CreateCommandList(); }
	void DestroyCommandList(CommandList& cmdList) { g_DispatchTable.DestroyCommandList(cmdList); }
	void ExecuteCommandList(CommandList& cmdList) { g_DispatchTable.ExecuteCommandList(cmdList); }

	void CommandList::open() { g_DispatchTable.CmdOpen(*this); }
	void CommandList::close() { g_DispatchTable.CmdClose(*this); }
	void CommandList::setPipeline(const PipelineHandle& pipeline) { g_DispatchTable.CmdSetPipeline(*this, pipeline); }
	void CommandList::setIndexBuffer(const BufferHandle& buffer, uint64_t offset) { g_DispatchTable.CmdSetIndexBuffer(*this, buffer, offset); }
	void CommandList::setVertexBuffer(const BufferHandle& buffer, uint64_t offset) { g_DispatchTable.CmdSetVertexBuffer(*this, buffer, offset); }
	void CommandList::draw(uint32_t vertexCount, uint32_t instanceCount,
		uint32_t firstVertex, uint32_t firstInstance) { g_DispatchTable.CmdDraw(*this, vertexCount, instanceCount, firstVertex, firstInstance); }
	void CommandList::drawIndexed(uint32_t indexCount, int32_t vertexOffset,
		uint32_t instanceCount, uint32_t firstIndex, uint32_t firstInstance) { g_DispatchTable.CmdDrawIndexed(*this, indexCount,
		vertexOffset, instanceCount, firstIndex, firstInstance); }

	void CommandList::beginRenderPass(RenderPassHandle pass,
		std::vector<ClearValue> clears) { g_DispatchTable.CmdBeginRenderPass(*this, pass, clears); }
	void CommandList::endRenderPass() { g_DispatchTable.CmdEndRenderPass(*this); }




} // namespace RenderX
