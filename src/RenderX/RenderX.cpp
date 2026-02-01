#include "RenderX/RenderX.h"
#include "RenderX/RenderXCore.h"
#include "RenderX/DebugProfiler.h"
#include "OpenGL/RenderXGL.h"
#include "Vulkan/RenderXVK.h"
#include "Log.h"
#include "ProLog/ProLog.h"
#include <cstring>

namespace Rx {

	// Global Variables
	RenderDispatchTable g_DispatchTable;
	GraphicsAPI API = GraphicsAPI::None;

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
		if (API != GraphicsAPI::None && g_DispatchTable.Shutdown) {
			RENDERX_INFO("Init: shutting down active backend before reinitializing");
			g_DispatchTable.Shutdown();
			// Clear function pointers so stale calls are impossible.
			std::memset(&g_DispatchTable, 0, sizeof(g_DispatchTable));
			API = GraphicsAPI::None;
		}

		switch (window.api) {
		case GraphicsAPI::OpenGL: {
			RENDERX_INFO("Initializing OpenGL backend...");
#define X(_ret, _name, ...) g_DispatchTable._name = RxGL::GL##_name;
			RENDERX_API(X)
#undef X

			if (g_DispatchTable.Init)
				g_DispatchTable.Init(window);

			API = window.api;
			RENDERX_INFO("OpenGL backend loaded successfully.");
			break;
		}

		case GraphicsAPI::Vulkan: {
			RENDERX_INFO("Initializing Vulkan backend...");
#define X(_ret, _name, ...) g_DispatchTable._name = RxVK::VK##_name;
			RENDERX_API(X)
#undef X

			if (g_DispatchTable.Init)
				g_DispatchTable.Init(window);

			API = window.api;
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

	void Shutdown() {
		// Call the active backend's shutdown if available.
		if (g_DispatchTable.Shutdown) {
			g_DispatchTable.Shutdown();
		}
		// Reset dispatch table and API so a new backend can be initialized safely.
		std::memset(&g_DispatchTable, 0, sizeof(g_DispatchTable));
		API = GraphicsAPI::None;

		PROFILE_END_SESSION();
		LOG_SHUTDOWN();
	}

	// API Functions
	void Begin(uint32_t frameIndex) { return g_DispatchTable.Begin(frameIndex); }
	void End(uint32_t frameIndex) { g_DispatchTable.End(frameIndex); }

	ShaderHandle CreateShader(const ShaderDesc& desc) { return g_DispatchTable.CreateShader(desc); }
	PipelineHandle CreateGraphicsPipeline(PipelineDesc& desc) { return g_DispatchTable.CreateGraphicsPipeline(desc); }

	RenderPassHandle CreateRenderPass(const RenderPassDesc& desc) { return g_DispatchTable.CreateRenderPass(desc); }
	void DestroyRenderPass(RenderPassHandle& pass) { g_DispatchTable.DestroyRenderPass(pass); }
	RenderPassHandle GetDefaultRenderPass() { return g_DispatchTable.GetDefaultRenderPass(); }

	FramebufferHandle CreateFramebuffer(const FramebufferDesc& desc) { return g_DispatchTable.CreateFramebuffer(desc); }
	void DestroyFramebuffer(FramebufferHandle& framebuffer) { g_DispatchTable.DestroyFramebuffer(framebuffer); }

	BufferHandle CreateBuffer(const BufferDesc& desc) { return g_DispatchTable.CreateBuffer(desc); }

	BufferViewHandle CreateBufferView(const BufferViewDesc& desc) { return g_DispatchTable.CreateBufferView(desc); }
	void DestroyBufferView(BufferViewHandle& handle) { g_DispatchTable.DestroyBufferView(handle); }

	PipelineLayoutHandle CreatePipelineLayout(const ResourceGroupLayout* playouts, uint32_t layoutCount) { return g_DispatchTable.CreatePipelineLayout(playouts, layoutCount); }
	ResourceGroupHandle CreateResourceGroup(const ResourceGroupDesc& desc) { return g_DispatchTable.CreateResourceGroup(desc); }
	void DestroyResourceGroup(ResourceGroupHandle& handle) { g_DispatchTable.DestroyResourceGroup(handle); }

	CommandList CreateCommandList(uint32_t frameIndex) { return g_DispatchTable.CreateCommandList(frameIndex); }
	void DestroyCommandList(CommandList& cmdList, uint32_t frameIndex) { g_DispatchTable.DestroyCommandList(cmdList, frameIndex); }
	void ExecuteCommandList(CommandList& cmdList) { g_DispatchTable.ExecuteCommandList(cmdList); }

	void CommandList::open() { g_DispatchTable.CmdOpen(*this); }
	void CommandList::close() { g_DispatchTable.CmdClose(*this); }
	void CommandList::setPipeline(const PipelineHandle& pipeline) { g_DispatchTable.CmdSetPipeline(*this, pipeline); }
	void CommandList::setIndexBuffer(const BufferHandle& buffer, uint64_t offset) { g_DispatchTable.CmdSetIndexBuffer(*this, buffer, offset); }
	void CommandList::setVertexBuffer(const BufferHandle& buffer, uint64_t offset) { g_DispatchTable.CmdSetVertexBuffer(*this, buffer, offset); }
	void CommandList::setResourceGroup(const ResourceGroupHandle& handle) { g_DispatchTable.CmdSetResourceGroup(*this, handle); }
	void CommandList::draw(uint32_t vertexCount, uint32_t instanceCount,
		uint32_t firstVertex, uint32_t firstInstance) { g_DispatchTable.CmdDraw(*this, vertexCount, instanceCount, firstVertex, firstInstance); }
	void CommandList::drawIndexed(uint32_t indexCount, int32_t vertexOffset,
		uint32_t instanceCount, uint32_t firstIndex, uint32_t firstInstance) { g_DispatchTable.CmdDrawIndexed(*this, indexCount,
		vertexOffset, instanceCount, firstIndex, firstInstance); }

	void CommandList::beginRenderPass(RenderPassHandle pass,
		const ClearValue* clears, uint32_t clearCount) { g_DispatchTable.CmdBeginRenderPass(*this, pass, clears, clearCount); }
	void CommandList::endRenderPass() { g_DispatchTable.CmdEndRenderPass(*this); }

	void CommandList::writeBuffer(BufferHandle handle, void* data, uint32_t offset, uint32_t size) {
		g_DispatchTable.CmdWriteBuffer(*this, handle, data, offset, size);
	}



} // namespace Rx
