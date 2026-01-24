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



	//------------------------------------------------------------------------------
	// SHADER CREATION
	//------------------------------------------------------------------------------

	/**
	 * @brief Creates and compiles a GPU shader.
	 * Compiles from source or loads precompiled bytecode based on backend.
	 */
	RENDERX_API ShaderHandle CreateShader(const ShaderDesc& desc);

	//------------------------------------------------------------------------------
	// PIPELINE CREATION (GRAPHICS)
	//------------------------------------------------------------------------------

	/**
	 * @brief Creates a GPU graphics pipeline using a full PipelineDesc.
	 *
	 * @param desc Full graphics pipeline state description.
	 * @return Created graphics pipeline handle.
	 */
	RENDERX_API PipelineHandle CreateGraphicsPipeline(PipelineDesc& desc);

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
	RENDERX_API BufferHandle CreateBuffer(const BufferDesc& desc);

	RENDERX_API FramebufferHandle CreateFramebuffer(const FramebufferDesc& desc);
	RENDERX_API void DestroyFramebuffer(FramebufferHandle handle);

	RENDERX_API RenderPassHandle CreateRenderPass(const RenderPassDesc& desc);
	RENDERX_API void DestroyRenderPass(RenderPassHandle& handle);

	RENDERX_API CommandList CreateCommandList();
	RENDERX_API void DestroyCommandList(CommandList& cmdList);
	RENDERX_API void ExecuteCommandList(CommandList& cmdList);

	//------------------------------------------------------------------------------
	RENDERX_API void Init(const Window& info);
	RENDERX_API void ShutDown();

	// frame Lifecycle
	RENDERX_API void Begin();
	RENDERX_API void End();

	RENDERX_API SurfaceHandle CreateSurface(void* nativeWindow);
	RENDERX_API SwapchainHandle CreateSwapchain(SurfaceHandle surface);

	RENDERX_API void DestroySwapchain(SwapchainHandle);
	RENDERX_API void DestroySurface(SurfaceHandle);
	// temp
	RENDERX_API bool ShouldClose();

} // namespace RenderX
