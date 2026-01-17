#pragma once
#include "RenderXCommon.h"



//------------------------------------------------------------------------------
// RENDERING HARDWARE INTERFACE (RHI)
//------------------------------------------------------------------------------
// Defines the runtime-dispatchable abstraction for graphics APIs.
// Backend modules (OpenGL, Vulkan, DirectX, etc.) provide actual
// implementations routed through the dispatch table.
//------------------------------------------------------------------------------

namespace RenderX {

	//------------------------------------------------------------------------------
	// API INITIALIZATION
	//------------------------------------------------------------------------------

	/**
	 * @brief Loads and initializes the selected rendering backend.
	 *
	 * Populates the global dispatch table with backend function pointers and
	 * prepares the graphics subsystem for usage.
	 *
	 * @param api The backend to initialize (e.g., RenderXAPI::OpenGL).
	 *
	 * @note Must be called before any rendering operations.
	 */
	void RENDERX_API SetBackend(GraphicsAPI api);

	//------------------------------------------------------------------------------
	// SHADER CREATION
	//------------------------------------------------------------------------------

	/**
	 * @brief Creates and compiles a GPU shader.
	 * Compiles from source or loads precompiled bytecode based on backend.
	 */
	ShaderHandle RENDERX_API CreateShader(const ShaderDesc& desc);

	//------------------------------------------------------------------------------
	// PIPELINE CREATION (GRAPHICS)
	//------------------------------------------------------------------------------

	/**
	 * @brief Creates a GPU graphics pipeline using a full PipelineDesc.
	 *
	 * @param desc Full graphics pipeline state description.
	 * @return Created graphics pipeline handle.
	 */
	const PipelineHandle RENDERX_API CreateGraphicsPipeline(PipelineDesc& desc);

	//------------------------------------------------------------------------------
	// BUFFER CREATION
	//------------------------------------------------------------------------------

	/**
	 * @brief Creates a GPU vertex buffer.
	 *
	 * @param size Size in bytes.
	 * @param data Optional pointer to initial data.
	 * @param use Buffer usage pattern.
	 */
	// const BufferHandle RENDERX_API CreateVertexBuffer(size_t size, const void* data, BufferUsage use);

	/**
	 * @brief Creates a GPU buffer.
	 * @param BufferDesc configuration of the buffer.
	 */
	const BufferHandle RENDERX_API CreateBuffer(const BufferDesc& desc);

	CommandList RENDERX_API CreateCommandList();
	void RENDERX_API DestroyCommandList(CommandList& cmdList);
	void RENDERX_API ExecuteCommandList(CommandList& cmdList);

	//------------------------------------------------------------------------------
	void RENDERX_API Init();
	void RENDERX_API ShutDown();

	// temp
	bool RENDERX_API ShouldClose();

} // namespace RenderX
