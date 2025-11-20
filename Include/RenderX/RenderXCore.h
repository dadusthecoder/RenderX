#pragma once
#include "RenderX.h"

namespace RenderX {

	// #define RENDERER_FUNC(_ret, _name, ...) _ret _name(__VA_ARGS__);
	// //inti
	// RENDERER_FUNC(void, Init)

	// //frame Start/End
	// RENDERER_FUNC(void , Begin)
	// RENDERER_FUNC(void , End)

	// //pipeline Graphics
	// RENDERER_FUNC(const PipelineHandle, CreatePipelineGFX_Desc,const PipelineDesc &desc)
	// RENDERER_FUNC(const PipelineHandle, CreatePipelineGFX_Src, const std::string &vertsrc, const std::string &fragsrc)
	// RENDERER_FUNC(const PipelineHandle, CreatePipelineGFX_Shader, const ShaderDesc& vertdesc, const ShaderDesc& fragdesc)

	// //pepline Compute
	// RENDERER_FUNC(const PipelineHandle, CreatePipelineCOMP_Desc, const PipelineDesc& desc)
	// RENDERER_FUNC(const PipelineHandle, CreatePipelineCOMP_Shader, const ShaderDesc& compdesc)
	// RENDERER_FUNC(const PipelineHandle, CreatePipelineCOMP_Src, const std::string& compsrc)

	// //resource Creation
	// RENDERER_FUNC(const ShaderHandle, CreateShader, const ShaderDesc &desc)

	// RENDERER_FUNC(const BufferHandle, CreateVertexBuffer, size_t size, void *data,BufferUsage use)
	// RENDERER_FUNC(void, CreateVertexLayout, const VertexLayout& layout)
	// RENDERER_FUNC(const VertexArrayHandle, CreateVertexArray)

	// //drawing commandss
	// RENDERER_FUNC(void,Draw,uint32_t vertexCount , uint32_t instanceCount , uint32_t firstVertex , uint32_t firstInstance)
	// RENDERER_FUNC(void,DrawIndexed,uint32_t indexCount , uint32_t instanceCount , uint32_t firstIndex , int32_t vertexOffset , uint32_t firstInstance)

	// //
	// RENDERER_FUNC(void,BindPipeline, const PipelineHandle handle)
	// RENDERER_FUNC(void,BindVertexArray, const VertexArrayHandle handle)
	// RENDERER_FUNC(void,BindVertexBuffer, const BufferHandle handle)
	// RENDERER_FUNC(void,BindIndexBuffer, const BufferHandle handle)

	// #undef RENDERER_FUNC

	//------------------------------------------------------------------------------
	// RENDER DISPATCH TABLE
	//------------------------------------------------------------------------------

	/**
	 * @struct RenderDispatchTable
	 * @brief Function table holding the active backend�s GPU entry points.
	 *
	 * When a graphics API is loaded, its function pointers populate this table,
	 * allowing the engine to remain API-agnostic at runtime.
	 */

	struct RenderDispatchTable {
#define RENDERER_FUNC(_ret, _name, ...)           \
	using _name##_func_t = _ret (*)(__VA_ARGS__); \
	_name##_func_t _name;
#include "RenderXAPI.def"
#undef RENDERER_FUNC
	};

	//------------------------------------------------------------------------------
	// GLOBAL STATE
	//------------------------------------------------------------------------------

	/// @brief Global dispatch table instance bound to the active backend.
	extern RenderDispatchTable RENDERX_API g_DispatchTable;

	/// @brief Enum representing the currently active rendering backend.
	extern RenderXAPI RENDERX_API API;
}