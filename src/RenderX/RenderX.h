#pragma once
#include "RenderXTypes.h"

//------------------------------------------------------------------------------
// RENDERING HARDWARE INTERFACE (RHI)
//------------------------------------------------------------------------------
// Defines the runtime-dispatchable abstraction for graphics APIs.
// Backend modules (e.g., OpenGL, Vulkan, DirectX) provide the actual
// implementations, which are routed dynamically through the dispatch table.
//------------------------------------------------------------------------------

namespace Lgt {

	//------------------------------------------------------------------------------
	// PUBLIC API WRAPPERS
	//------------------------------------------------------------------------------
	// Inline wrappers that forward calls to the active backend via the global
	// dispatch table. These provide backend-agnostic access to GPU operations.
	//------------------------------------------------------------------------------


#pragma once

// Detect platform
#if defined(_WIN32) || defined(_WIN64)
  #define RENDERX_PLATFORM_WINDOWS
#elif defined(__APPLE__) || defined(__MACH__)
  #define RENDERX_PLATFORM_MACOS
#elif defined(__linux__)
  #define RENDERX_PLATFORM_LINUX
#else
  #define RENDERX_PLATFORM_UNKNOWN
#endif

// Export / import macros
#if defined(RENDERX_STATIC)
  // Building or using static library
  #define RENDERX_API
#else
  // Shared library (DLL / .so / .dylib)
  #if defined(RENDERX_PLATFORM_WINDOWS)
    #if defined(RENDERX_BUILD_DLL)
      #define RENDERX_API __declspec(dllexport)
    #else
      #define RENDERX_API __declspec(dllimport)
    #endif
  #elif defined(__GNUC__) || defined(__clang__)
    // GCC / Clang on Linux, macOS, MinGW, etc.
    #define RENDERX_API __attribute__((visibility("default")))
  #else
    // Unknown compiler/platform
    #define RENDERX_API
    #pragma warning Unknown dynamic link import/export semantics.
  #endif
#endif


	//------------------------------------------------------------------------------
	// API INITIALIZATION
	//------------------------------------------------------------------------------

	/**
	 * @brief Loads and initializes the selected rendering backend.
	 *
	 * Populates the global dispatch table (`g_DispatchTable`) with backend
	 * function pointers and prepares the graphics subsystem for use.
	 *
	 * @param api The backend to initialize (e.g., RendererAPI::OpenGL).
	 *
	 * @note Must be called before any rendering operations.
	 */
	void RENDERX_API LoadAPI(RenderXAPI api);

	//------------------------------------------------------------------------------
	// INLINE CREATION WRAPPERS (DIRECT DISPATCH)
	//------------------------------------------------------------------------------

	/**
	 * @brief Creates a GPU graphics pipeline.
	 *
	 * Use `PipelineDesc` to specify pipeline states (blend, raster, depth, etc.),
	 * or `ShaderDesc` when providing precompiled shader bytecode.
	 *
	 * @param desc Pipeline description.
	 * @return Handle to the created graphics pipeline.
	 */
	 const PipelineHandle RENDERX_API CreatePipelineGFX(const PipelineDesc& desc);

	/// @brief Creates a graphics pipeline from raw shader source code with Default Pipeline States(blend, raster, depth, etc.)..
	 const PipelineHandle RENDERX_API CreatePipelineGFX(const std::string& vertSrc, const std::string& fragSrc);

	/**
	 *@brief Creates a graphics pipeline using precompiled shader descriptors or can also use the source code from ShaderDesc
	 * with Default Pipeline States(blend, raster, depth, etc.).
	.*/
	 const PipelineHandle RENDERX_API CreatePipelineGFX(const ShaderDesc& vertDesc, const ShaderDesc& fragDesc);

	/**
	 * @brief Creates a compute pipeline.
	 * Use `PipelineDesc` to specify pipeline states (blend, raster, depth, etc.),
	 * Similar to graphics pipelines, this encapsulates a compute shader and
	 * associated state for dispatching GPU workloads.
	 */
	 const PipelineHandle RENDERX_API CreatePipelineCOMP(const PipelineDesc& desc);

	/// @brief Creates a Compute  pipeline from raw shader source code with Default Pipeline States(blend, raster, depth, etc.).
	 const PipelineHandle RENDERX_API CreatePipelineCOMP(const std::string& compSrc);

	/// @brief Creates a Compute pipeline  Default Pipeline States (blend, raster, depth, etc.) using precompiled shader descriptors or can also use the source code.
	 const PipelineHandle RENDERX_API CreatePipelineCOMP(const ShaderDesc& compDesc);


	/**
	 * @brief Creates and compiles a GPU shader.
	 *
	 * Compiles shader source or loads precompiled bytecode depending on backend.
	 *
	 * @param desc Shader description (stage, source, parameters).
	 * @return Handle to the created shader.
	 */
	 const ShaderHandle RENDERX_API CreateShader(const ShaderDesc& desc);

	/**
	 * @brief Creates a GPU vertex buffer.
	 *
	 * Allocates and uploads vertex data to GPU memory.
	 *
	 * @param size Size of the data in bytes.
	 * @param data Pointer to vertex data (nullable).
	 * @param use  Usage pattern (Static, Dynamic, Stream).
	 * @return Handle to the created buffer.
	 */
	 const BufferHandle RENDERX_API CreateVertexBuffer(size_t size, const void* data, BufferUsage use);
	 const VertexLayoutHandle RENDERX_API CreateVertexLayout(const VertexLayout& layout);
	 const void RENDERX_API BindLayout(const VertexLayoutHandle handle);

	 const void RENDERX_API Draw(uint32_t vertexCount , uint32_t instanceCount , uint32_t firstVertex , uint32_t firstInstance);
	 const void RENDERX_API DrawIndexed(uint32_t indexCount , uint32_t instanceCount , uint32_t firstIndex , int32_t vertexOffset , uint32_t firstInstance);
	 const void RENDERX_API BindPipeline(const PipelineHandle handle);		
	 const void RENDERX_API BindVertexBuffer(const BufferHandle handle);
	 const void RENDERX_API BindIndexBuffer(const BufferHandle handle);
	 const void RENDERX_API Begin();
	 const void RENDERX_API End();

} // namespace Lng
