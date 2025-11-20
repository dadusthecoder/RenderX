#pragma once
#include "RenderXTypes.h"

//------------------------------------------------------------------------------
// PLATFORM DETECTION
//------------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
#define RENDERX_PLATFORM_WINDOWS
#elif defined(__APPLE__) || defined(__MACH__)
#define RENDERX_PLATFORM_MACOS
#elif defined(__linux__)
#define RENDERX_PLATFORM_LINUX
#else
#define RENDERX_PLATFORM_UNKNOWN
#endif

//------------------------------------------------------------------------------
// EXPORT / IMPORT MACROS
//------------------------------------------------------------------------------

#if defined(RENDERX_STATIC)
#define RENDERX_API
#else
#if defined(RENDERX_PLATFORM_WINDOWS)
#if defined(RENDERX_BUILD_DLL)
#define RENDERX_API __declspec(dllexport)
#else
#define RENDERX_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define RENDERX_API __attribute__((visibility("default")))
#else
#define RENDERX_API
#pragma warning Unknown dynamic link import / export semantics.
#endif
#endif

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
	void RENDERX_API LoadAPI(RenderXAPI api);

	//------------------------------------------------------------------------------
	// PIPELINE CREATION (GRAPHICS)
	//------------------------------------------------------------------------------

	/**
	 * @brief Creates a GPU graphics pipeline using a full PipelineDesc.
	 *
	 * @param desc Full graphics pipeline state description.
	 * @return Created graphics pipeline handle.
	 */
	const PipelineHandle RENDERX_API CreatePipelineGFX(const PipelineDesc& desc);

	/**
	 * @brief Creates a graphics pipeline from raw shader source.
	 *
	 * Default states (blend, raster, depth) will be automatically applied.
	 */
	const PipelineHandle RENDERX_API CreatePipelineGFX(const std::string& vertSrc,
		const std::string& fragSrc);

	/**
	 * @brief Creates a graphics pipeline from shader descriptors.
	 *
	 * Accepts source or precompiled shader data through ShaderDesc.
	 */
	const PipelineHandle RENDERX_API CreatePipelineGFX(const ShaderDesc& vertDesc,
		const ShaderDesc& fragDesc);

	//------------------------------------------------------------------------------
	// PIPELINE CREATION (COMPUTE)
	//------------------------------------------------------------------------------

	/**
	 * @brief Creates a compute pipeline using a full PipelineDesc.
	 */
	const PipelineHandle RENDERX_API CreatePipelineCOMP(const PipelineDesc& desc);

	/**
	 * @brief Creates a compute pipeline from raw compute shader source.
	 */
	const PipelineHandle RENDERX_API CreatePipelineCOMP(const std::string& compSrc);

	/**
	 * @brief Creates a compute pipeline from a compute shader descriptor.
	 */
	const PipelineHandle RENDERX_API CreatePipelineCOMP(const ShaderDesc& compDesc);

	//------------------------------------------------------------------------------
	// SHADER CREATION
	//------------------------------------------------------------------------------

	/**
	 * @brief Creates and compiles a GPU shader.
	 *
	 * Compiles from source or loads precompiled bytecode based on backend.
	 */
	const ShaderHandle RENDERX_API CreateShader(const ShaderDesc& desc);

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
	const BufferHandle RENDERX_API CreateVertexBuffer(size_t size, const void* data, BufferUsage use);

	//------------------------------------------------------------------------------
	// VERTEX INPUT LAYOUT & VERTEX ARRAY
	//------------------------------------------------------------------------------

	/**
	 * @brief Creates a vertex layout (attribute format, stride, semantics).
	 * @note In OpenGL, this configures the currently bound VAO , Bind The VAO Before Calling This.
	 * @note In DirectX/Vulkan, this defines the input assembly state for pipelines.
	 */
	void RENDERX_API CreateVertexLayout(const VertexLayout& layout);

	/**
	 * @brief Creates a GPU Vertex Array Object (VAO).
	 */
	const VertexArrayHandle RENDERX_API CreateVertexArray();

	/**
	 * @brief Binds an existing VAO.
	 * @note Required before setting up vertex layouts in OpenGL.
	 */
	void RENDERX_API BindVertexArray(const VertexArrayHandle handle);

	//------------------------------------------------------------------------------
	// RESOURCE BINDING
	//------------------------------------------------------------------------------

	/**
	 * @brief Binds a graphics or compute pipeline for subsequent draw/dispatch calls.
	 */
	void RENDERX_API BindPipeline(const PipelineHandle handle);

	/**
	 * @brief Binds a vertex buffer for input assembly .
	 * @note OPENGL requires binding the VAO first.
	 */
	void RENDERX_API BindVertexBuffer(const BufferHandle handle);

	/**
	 * @brief Binds an index buffer for indexed drawing.
	 */
	void RENDERX_API BindIndexBuffer(const BufferHandle handle);

	//------------------------------------------------------------------------------
	// DRAW COMMANDS
	//------------------------------------------------------------------------------

	/**
	 * @brief Issues a non-indexed instanced draw call .
	 *
	 * @param vertexCount Number of vertices.
	 * @param instanceCount Instance count for instanced rendering.
	 * @param firstVertex First vertex index.
	 * @param firstInstance First instance index.
	 */
	void RENDERX_API Draw(uint32_t vertexCount,
		uint32_t instanceCount,
		uint32_t firstVertex,
		uint32_t firstInstance);

	/**
	 * @brief Issues an indexed draw call.
	 *
	 * @param indexCount Number of indices.
	 * @param instanceCount Instance count.
	 * @param firstIndex Starting index.
	 * @param vertexOffset Added to vertex index base.
	 * @param firstInstance First instance.
	 */
	void RENDERX_API DrawIndexed(uint32_t indexCount,
		uint32_t instanceCount,
		uint32_t firstIndex,
		int32_t vertexOffset,
		uint32_t firstInstance);

	//------------------------------------------------------------------------------
	// FRAME CONTROL
	//------------------------------------------------------------------------------

	/**
	 * @brief Begins a new rendering frame or command batch.
	 */
	void RENDERX_API BeginFrame();

	/**
	 * @brief Ends and submits the currently recorded frame or command batch.
	 */
	void RENDERX_API EndFrame();


	//temp 
	bool RENDERX_API ShouldClose();

} // namespace RenderX
