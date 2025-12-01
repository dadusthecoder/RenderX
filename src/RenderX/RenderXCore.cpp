
#include "RenderX/RenderXCore.h"

namespace RenderX {

	const ShaderHandle CreateShader(const ShaderDesc& desc) { return g_DispatchTable.CreateShader(desc); }
	const PipelineHandle CreatePipeline(PipelineDesc& desc) { return g_DispatchTable.CreatePipeline(desc); }

	const BufferHandle CreateVertexBuffer(size_t size, const void* data, BufferUsage use) { return g_DispatchTable.CreateVertexBuffer(size, data, use); }

	void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) { return g_DispatchTable.Draw(vertexCount, instanceCount, firstVertex, firstInstance); }
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) { return g_DispatchTable.DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance); }

	void BindPipeline(const PipelineHandle handle) { return g_DispatchTable.BindPipeline(handle); }
	void BindVertexBuffer(const BufferHandle handle) { return g_DispatchTable.BindVertexBuffer(handle); }
	void BindIndexBuffer(const BufferHandle handle) { return g_DispatchTable.BindIndexBuffer(handle); }

	void BeginFrame() { return g_DispatchTable.BeginFrame(); }
	void EndFrame() { return g_DispatchTable.EndFrame(); }
	bool ShouldClose() { return g_DispatchTable.ShouldClose(); }
}