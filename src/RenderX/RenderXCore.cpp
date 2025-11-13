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
}