#pragma once
#include "RenderXCommon.h"


//------------------------------------------------------------------------------
// RENDERING HARDWARE INTERFACE (RHI)
//------------------------------------------------------------------------------
// Defines the runtime-dispatchable abstraction for graphics APIs.
// Backend modules (OpenGL, Vulkan, DirectX, etc.) provide actual
// implementations routed through the dispatch table.
//------------------------------------------------------------------------------

namespace Rx {

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

	RENDERX_API BufferViewHandle CreateBufferView(const BufferViewDesc& desc);
	RENDERX_API void DestroyBufferView(BufferViewHandle& handle);

	//------------------------------------------------------------------------------
	// RESOURCE GROUPS & DESCRIPTORS
	//------------------------------------------------------------------------------

	/**
	 * @brief Creates a resource group layout that describes bindings.
	 *
	 * @param desc ResourceGroupLayoutDesc specifying binding slots and types.
	 * @return Created resource group layout handle.
	 */
	RENDERX_API PipelineLayoutHandle CreatePipelineLayout(const ResourceGroupLayout* playouts, uint32_t layoutCount);

	/**
	 * @brief Creates a resource group instance with actual resource bindings.
	 *
	 * @param desc ResourceGroupDesc specifying layout and resources.
	 * @return Created resource group handle.
	 */
	RENDERX_API ResourceGroupHandle CreateResourceGroup(const ResourceGroupDesc& desc);

	/**
	 * @brief Destroys a resource group instance.
	 *
	 * @param handle Handle to resource group to destroy.
	 */
	RENDERX_API void DestroyResourceGroup(ResourceGroupHandle& handle);

	RENDERX_API FramebufferHandle CreateFramebuffer(const FramebufferDesc& desc);
	RENDERX_API void DestroyFramebuffer(FramebufferHandle handle);

	RENDERX_API RenderPassHandle CreateRenderPass(const RenderPassDesc& desc);
	RENDERX_API void DestroyRenderPass(RenderPassHandle& handle);
    RENDERX_API RenderPassHandle GetDefaultRenderPass();

	RENDERX_API CommandList CreateCommandList(uint32_t frameIndex);
	RENDERX_API void DestroyCommandList(CommandList& cmdList , uint32_t frameIndex );
	RENDERX_API void ExecuteCommandList(CommandList& cmdList);

	//------------------------------------------------------------------------------
	RENDERX_API void Init(const Window& info);
	RENDERX_API void Shutdown();

	// frame Lifecycle
	RENDERX_API void Begin(uint32_t frameIndex);
	RENDERX_API void End(uint32_t frameIndex);	
} // namespace Rx
