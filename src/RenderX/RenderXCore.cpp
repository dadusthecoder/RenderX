
#include "RenderX/RenderXCore.h"

namespace RenderX {
	const PipelineHandle CreatePipelineGFX(const PipelineDesc& desc) { return g_DispatchTable.CreatePipelineGFX_Desc(desc); }
	const PipelineHandle CreatePipelineGFX(const std::string& vertSrc, const std::string& fragSrc) { return g_DispatchTable.CreatePipelineGFX_Src(vertSrc, fragSrc); }
	const PipelineHandle CreatePipelineGFX(const ShaderDesc& vertDesc, const ShaderDesc& fragDesc) { return g_DispatchTable.CreatePipelineGFX_Shader(vertDesc, fragDesc); }

	const PipelineHandle CreatePipelineCOMP(const PipelineDesc& desc) { return g_DispatchTable.CreatePipelineCOMP_Desc(desc); }
	const PipelineHandle CreatePipelineCOMP(const std::string& compSrc) { return g_DispatchTable.CreatePipelineCOMP_Src(compSrc); }
	const PipelineHandle CreatePipelineCOMP(const ShaderDesc& compDesc) { return g_DispatchTable.CreatePipelineCOMP_Shader(compDesc); }

	const ShaderHandle CreateShader(const ShaderDesc& desc) { return g_DispatchTable.CreateShader(desc); }

	const VertexArrayHandle CreateVertexArray() { return g_DispatchTable.CreateVertexArray(); }
	const BufferHandle CreateVertexBuffer(size_t size, const void* data, BufferUsage use) { return g_DispatchTable.CreateVertexBuffer(size, data, use); }
	void CreateVertexLayout(const VertexLayout& layout) { return g_DispatchTable.CreateVertexLayout(layout); }

	void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) { return g_DispatchTable.Draw(vertexCount, instanceCount, firstVertex, firstInstance); }
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) { return g_DispatchTable.DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance); }

	void BindPipeline(const PipelineHandle handle) { return g_DispatchTable.BindPipeline(handle); }
	void BindVertexBuffer(const BufferHandle handle) { return g_DispatchTable.BindVertexBuffer(handle); }
	void BindIndexBuffer(const BufferHandle handle) { return g_DispatchTable.BindIndexBuffer(handle); }
	void BindVertexArray(const VertexArrayHandle handle) { return g_DispatchTable.BindVertexArray(handle); }


	void BeginFrame() { return g_DispatchTable.BeginFrame(); }
	void EndFrame() { return g_DispatchTable.EndFrame(); }
	bool ShouldClose() { return g_DispatchTable.ShouldClose(); }
}