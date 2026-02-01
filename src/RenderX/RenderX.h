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


	/**
	 * @brief Creates and compiles a GPU shader.
	 * Compiles from source or loads precompiled bytecode based on backend.
	 */
	RENDERX_EXPORT ShaderHandle CreateShader(const ShaderDesc& desc);

	// PIPELINE CREATION (GRAPHICS)

	/**
	 * @brief Creates a GPU graphics pipeline using a full PipelineDesc.
	 *
	 * @param desc Full graphics pipeline state description.
	 * @return Created graphics pipeline handle.
	 */
	RENDERX_EXPORT PipelineHandle CreateGraphicsPipeline(PipelineDesc& desc);

	// BUFFER CREATION

	/**
	 * @brief Creates a GPU vertex buffer.
	 *
	 * @param size Size in bytes.
	 * @param data Optional pointer to initial data.
	 * @param use Buffer usage pattern.
	 */
	// const BufferHandle RENDERX_EXPORT CreateVertexBuffer(size_t size, const void* data, BufferUsage use);

	/**
	 * @brief Creates a GPU buffer.
	 * @param BufferDesc configuration of the buffer.
	 */
	RENDERX_EXPORT BufferHandle CreateBuffer(const BufferDesc& desc);

	RENDERX_EXPORT BufferViewHandle CreateBufferView(const BufferViewDesc& desc);
	RENDERX_EXPORT void DestroyBufferView(BufferViewHandle& handle);

	// RESOURCE GROUPS & DESCRIPTORS

	/**
	 * @brief Creates a resource group layout that describes bindings.
	 *
	 * @param desc ResourceGroupLayoutDesc specifying binding slots and types.
	 * @return Created resource group layout handle.
	 */
	RENDERX_EXPORT PipelineLayoutHandle CreatePipelineLayout(const ResourceGroupLayout* playouts, uint32_t layoutCount);

	/**
	 * @brief Creates a resource group instance with actual resource bindings.
	 *
	 * @param desc ResourceGroupDesc specifying layout and resources.
	 * @return Created resource group handle.
	 */
	RENDERX_EXPORT ResourceGroupHandle CreateResourceGroup(const ResourceGroupDesc& desc);

	/**
	 * @brief Destroys a resource group instance.
	 *
	 * @param handle Handle to resource group to destroy.
	 */
	RENDERX_EXPORT void DestroyResourceGroup(ResourceGroupHandle& handle);

	RENDERX_EXPORT FramebufferHandle CreateFramebuffer(const FramebufferDesc& desc);
	RENDERX_EXPORT void DestroyFramebuffer(FramebufferHandle handle);

	RENDERX_EXPORT RenderPassHandle CreateRenderPass(const RenderPassDesc& desc);
	RENDERX_EXPORT void DestroyRenderPass(RenderPassHandle& handle);
    RENDERX_EXPORT RenderPassHandle GetDefaultRenderPass();

	RENDERX_EXPORT CommandList CreateCommandList(uint32_t frameIndex);
	RENDERX_EXPORT void DestroyCommandList(CommandList& cmdList , uint32_t frameIndex );
	RENDERX_EXPORT void ExecuteCommandList(CommandList& cmdList);

	RENDERX_EXPORT void Init(const Window& info);
	RENDERX_EXPORT void Shutdown();

	// frame Lifecycle
	RENDERX_EXPORT void Begin(uint32_t frameIndex);
	RENDERX_EXPORT void End(uint32_t frameIndex);	
} // namespace Rx
