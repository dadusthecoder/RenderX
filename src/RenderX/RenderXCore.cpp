#include "RenderXCore.h"
#include "RenderXCore.h"
#include "RenderX.h"
namespace Lgt {
	const PipelineHandle CreatePipelineGFX(const PipelineDesc& desc) { return g_DispatchTable.CreatePipelineGFX_Desc(desc); }
	const PipelineHandle CreatePipelineGFX(const std::string& vertSrc, const std::string& fragSrc) { return g_DispatchTable.CreatePipelineGFX_Src(vertSrc, fragSrc); }
	const PipelineHandle CreatePipelineGFX(const ShaderDesc& vertDesc, const ShaderDesc& fragDesc) { return g_DispatchTable.CreatePipelineGFX_Shader(vertDesc, fragDesc); }

	const PipelineHandle CreatePipelineCOMP(const PipelineDesc& desc) { return g_DispatchTable.CreatePipelineCOMP_Desc(desc); }
	const PipelineHandle CreatePipelineCOMP(const std::string& compSrc) { return g_DispatchTable.CreatePipelineCOMP_Src(compSrc); }
	const PipelineHandle CreatePipelineCOMP(const ShaderDesc& compDesc) { return g_DispatchTable.CreatePipelineCOMP_Shader(compDesc); }

	const ShaderHandle CreateShader(const ShaderDesc& desc) { return g_DispatchTable.CreateShader(desc); }

	const BufferHandle CreateVertexBuffer(size_t size, const void* data, BufferUsage use) { return g_DispatchTable.CreateVertexBuffer(size, data, use); }

	const void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
		return g_DispatchTable.Draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}
	const void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
		return g_DispatchTable.DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}
	const void BindPipeline(const PipelineHandle handle) {
		return g_DispatchTable.BindPipeline(handle);
	}
	const void BindVertexBuffer(const BufferHandle handle) {
		return g_DispatchTable.BindVertexBuffer(handle);
	}
	const void BindIndexBuffer(const BufferHandle handle) {
		return g_DispatchTable.BindIndexBuffer(handle);
	}

	const VertexLayoutHandle CreateVertexLayout(const VertexLayout& layout) {
		return g_DispatchTable.CreateVertexLayout(layout);
	}

	const void BindLayout(const VertexLayoutHandle handle) {
		return g_DispatchTable.BindLayout(handle);
	}

	const void Begin() {
		return g_DispatchTable.Begin();
	}
	const void End() {
		return g_DispatchTable.End();
	}
}