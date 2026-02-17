#pragma once
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>

#include "RX_Flags.h"

#if defined(_MSC_VER)
#define RENDERX_DEBUGBREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define RENDERX_DEBUGBREAK() __builtin_trap()
#else
#define RENDERX_DEBUGBREAK() std::abort()
#endif


#ifdef RX_DEBUG_BUILD
#define RENDERX_ASSERT(expr)                                                                                           \
	{                                                                                                                  \
		if (!(expr)) {                                                                                                 \
			spdlog::error("Assertion Failed! Expr: {}", #expr);                                                        \
			RENDERX_DEBUGBREAK();                                                                                      \
		}                                                                                                              \
	}

#define RENDERX_ASSERT_MSG(expr, msg, ...)                                                                             \
	{                                                                                                                  \
		if (!(expr)) {                                                                                                 \
			spdlog::error("Assertion Failed: {} | Expr: {}", fmt::format(msg, ##__VA_ARGS__), #expr);                  \
			RENDERX_DEBUGBREAK();                                                                                      \
		}                                                                                                              \
	}
#else
#define RENDERX_ASSERT(expr) (void)0
#define RENDERX_ASSERT_MSG(expr, msg, ...)
#endif


// EXPORT / IMPORT MACROS
#ifndef RX_BUILD_DLL
#define RENDERX_EXPORT
#else
#if defined(RX_PLATFORM_WINDOWS)
#pragma warning(disable : 4390)
#if defined(RX_BUILD_DLL)
#pragma warning(disable : 4251)
#define RENDERX_EXPORT __declspec(dllexport)
#else
#define RENDERX_EXPORT __declspec(dllimport)
#endif
#elif defined(RX_PLATFORM_LINUX) || defined(__GNUC__) || defined(__clang__)
#define RENDERX_EXPORT __attribute__((visibility("default")))
#else
#define RENDERX_EXPORT
#pragma warning Unknown dynamic link import / export semantics.
#endif
#endif

#define STRFY(s)        #s
#define XSTRFY(X)       STRFY(X)
#define PRINT(msg, ...) std::cout << XSTRFY(__VA_ARGS__) << std::endl

#define RENDERX_DEFINE_HANDLE(Name)                                                                                    \
	namespace HandleType {                                                                                             \
		struct Name {};                                                                                                \
	}                                                                                                                  \
	using Name##Handle = Handle<HandleType::Name>;

namespace spdlog {
	class logger;
}

namespace Rx {
	// Frame-based logging accumulator
	class RENDERX_EXPORT FrameLogger {
	public:
		struct LogEntry {
			std::string                           message;
			spdlog::level::level_enum             level;
			uint32_t                              count = 1;
			std::chrono::steady_clock::time_point last_time;
		};
		void AddMessage(const std::string& key, const std::string& msg, spdlog::level::level_enum level);
		void FlushFrame(std::shared_ptr<spdlog::logger>& logger);
		void Clear();
	private:
		std::unordered_map<std::string, LogEntry> frame_logs_;
		std::mutex                                mutex_;
	};

	class Log {
	public:
		RENDERX_EXPORT static void Init();
		RENDERX_EXPORT static void Shutdown();

		RENDERX_EXPORT static std::shared_ptr<spdlog::logger>& Core();
		RENDERX_EXPORT static std::shared_ptr<spdlog::logger>& Client();

		// Frame-based logging
		RENDERX_EXPORT static void BeginFrame();
		RENDERX_EXPORT static void EndFrame();
		RENDERX_EXPORT static void
		FrameLog(const std::string& key, const std::string& msg, spdlog::level::level_enum level = spdlog::level::info);

		RENDERX_EXPORT static void LogStatus(const std::string& msg);
	private:
		RENDERX_EXPORT static std::shared_ptr<spdlog::logger> s_CoreLogger;
		RENDERX_EXPORT static std::shared_ptr<spdlog::logger> s_ClientLogger;
		RENDERX_EXPORT static std::unique_ptr<FrameLogger>    s_FrameLogger;
	};
}

// Frame logging macros
#ifdef RX_DEBUG_BUILD
#define RX_FRAME_TRACE(key, msg, ...)                                                                                  \
	::Rx::Log::FrameLog(key, fmt::format("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__), spdlog::level::trace)
#define RX_FRAME_INFO(key, msg, ...)                                                                                   \
	::Rx::Log::FrameLog(key, fmt::format("[{}] " msg, __func__, ##__VA_ARGS__), spdlog::level::info)
#define RX_FRAME_WARN(key, msg, ...)                                                                                   \
	::Rx::Log::FrameLog(key, fmt::format("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__), spdlog::level::warn)

#define RX_BEGIN_FRAME() ::Rx::Log::BeginFrame()
#define RX_END_FRAME()   ::Rx::Log::EndFrame()
#else
#define RX_FRAME_TRACE(key, msg, ...)
#define RX_FRAME_INFO(key, msg, ...)
#define RX_FRAME_WARN(key, msg, ...)
#define RX_BEGIN_FRAME()
#define RX_END_FRAME()
#endif

// Regular logging macros (same as before)
#ifdef RX_DEBUG_BUILD
#define RX_CORE_TRACE(msg, ...)    ::Rx::Log::Core()->trace("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__)
#define RX_CORE_INFO(msg, ...)     ::Rx::Log::Core()->info("[{}] " msg, __func__, ##__VA_ARGS__)
#define RX_CORE_WARN(msg, ...)     ::Rx::Log::Core()->warn("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__)
#define RX_CORE_ERROR(msg, ...)    ::Rx::Log::Core()->error("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__)
#define RX_CORE_CRITICAL(msg, ...) ::Rx::Log::Core()->critical("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__)

#define RX_CLIENT_TRACE(msg, ...)    ::Rx::Log::Client()->trace("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__)
#define RX_CLIENT_INFO(msg, ...)     ::Rx::Log::Client()->info("[{}] " msg, __func__, ##__VA_ARGS__)
#define RX_CLIENT_WARN(msg, ...)     ::Rx::Log::Client()->warn("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__)
#define RX_CLIENT_ERROR(msg, ...)    ::Rx::Log::Client()->error("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__)
#define RX_CLIENT_CRITICAL(msg, ...) ::Rx::Log::Client()->critical("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__)

#define RX_STATUS(msg, ...) ::Rx::Log::LogStatus(fmt::format(msg, ##__VA_ARGS__))
#else
#define RX_CORE_TRACE(msg, ...)
#define RX_CORE_INFO(msg, ...)
#define RX_CORE_WARN(msg, ...)
#define RX_CORE_ERROR(msg, ...)
#define RX_CORE_CRITICAL(msg, ...)
#define RX_CLIENT_TRACE(msg, ...)
#define RX_CLIENT_INFO(msg, ...)
#define RX_CLIENT_WARN(msg, ...)
#define RX_CLIENT_ERROR(msg, ...)
#define RX_CLIENT_CRITICAL(msg, ...)
#define RX_STATUS(msg)
#endif

// for backward compatibility
#ifdef RX_DEBUG_BUILD
#define LOG_INIT()                 ::Rx::Log::Init()
#define RENDERX_TRACE(msg, ...)    ::Rx::Log::Core()->trace("[{}]: " msg, __func__, ##__VA_ARGS__)
#define RENDERX_INFO(...)          ::Rx::Log::Core()->info(__VA_ARGS__)
#define RENDERX_WARN(msg, ...)     ::Rx::Log::Core()->warn("[{}]: " msg, __func__, ##__VA_ARGS__)
#define RENDERX_ERROR(msg, ...)    ::Rx::Log::Core()->error("[{}]: " msg, __func__, ##__VA_ARGS__)
#define RENDERX_CRITICAL(msg, ...) ::Rx::Log::Core()->critical("[{}]: " msg, __func__, ##__VA_ARGS__)
#define LOG_SHUTDOWN()             ::Rx::Log::Shutdown();
#else
#define LOG_INIT()
#define RENDERX_TRACE(msg, ...)
#define RENDERX_INFO(msg, ...)
#define RENDERX_WARN(msg, ...)
#define RENDERX_ERROR(msg, ...)
#define RENDERX_CRITICAL(msg, ...)
#define LOG_SHUTDOWN()
#endif

#ifdef RX_DEBUG_BUILD
#define CLIENT_TRACE(msg, ...)    ::Rx::Log::Client()->trace("[{}]: " msg, __func__, ##__VA_ARGS__)
#define CLIENT_INFO(msg, ...)     ::Rx::Log::Client()->info("[{}]: " msg, __func__, ##__VA_ARGS__)
#define CLIENT_WARN(msg, ...)     ::Rx::Log::Client()->warn("[{}]: " msg, __func__, ##__VA_ARGS__)
#define CLIENT_ERROR(msg, ...)    ::Rx::Log::Client()->error("[{}]: " msg, __func__, ##__VA_ARGS__)
#define CLIENT_CRITICAL(msg, ...) ::Rx::Log::Client()->critical("[{}]: " msg, __func__, ##__VA_ARGS__)
#else
#define CLIENT_TRACE(msg, ...)
#define CLIENT_INFO(msg, ...)
#define CLIENT_WARN(msg, ...)
#define CLIENT_ERROR(msg, ...)
#define CLIENT_CRITICAL(msg, ...)
#endif

namespace Rx {

// RenderX API X-Macro
#define RENDERX_FUNC(X)                                                                                                \
                                                                                                                       \
	/* RenderX Lifetime */                                                                                             \
	X(void, BackendInit, (const InitDesc& window), (window))                                                           \
                                                                                                                       \
	X(void, BackendShutdown, (), ())                                                                                   \
                                                                                                                       \
	/* Pipeline Layout */                                                                                              \
	X(PipelineLayoutHandle,                                                                                            \
	  CreatePipelineLayout,                                                                                            \
	  (const ResourceGroupLayoutHandle* layouts, uint32_t layoutCount),                                                \
	  (layouts, layoutCount))                                                                                          \
                                                                                                                       \
	/* Pipeline Graphics */                                                                                            \
	X(PipelineHandle, CreateGraphicsPipeline, (PipelineDesc & desc), (desc))                                           \
                                                                                                                       \
	X(ShaderHandle, CreateShader, (const ShaderDesc& desc), (desc))                                                    \
                                                                                                                       \
	X(void, DestroyShader, (ShaderHandle & handle), (handle))                                                          \
                                                                                                                       \
	/* Resource Creation */                                                                                            \
	/* Buffers */                                                                                                      \
	X(BufferHandle, CreateBuffer, (const BufferDesc& desc), (desc))                                                    \
                                                                                                                       \
	/* Buffer Views */                                                                                                 \
	X(BufferViewHandle, CreateBufferView, (const BufferViewDesc& desc), (desc))                                        \
                                                                                                                       \
	X(void, DestroyBufferView, (BufferViewHandle & handle), (handle))                                                  \
                                                                                                                       \
	/* Render Pass */                                                                                                  \
	X(RenderPassHandle, CreateRenderPass, (const RenderPassDesc& desc), (desc))                                        \
                                                                                                                       \
	X(void, DestroyRenderPass, (RenderPassHandle & pass), (pass))                                                      \
                                                                                                                       \
	/* Framebuffer */                                                                                                  \
	X(FramebufferHandle, CreateFramebuffer, (const FramebufferDesc& desc), (desc))                                     \
                                                                                                                       \
	X(void, DestroyFramebuffer, (FramebufferHandle & framebuffer), (framebuffer))                                      \
                                                                                                                       \
	/* Resource Groups */                                                                                              \
	X(ResourceGroupHandle, CreateResourceGroup, (const ResourceGroupDesc& desc), (desc))                               \
                                                                                                                       \
	X(void, DestroyResourceGroup, (ResourceGroupHandle & handle), (handle))                                            \
                                                                                                                       \
	X(ResourceGroupLayoutHandle, CreateResourceGroupLayout, (const ResourceGroupLayoutDesc& desc), (desc))             \
                                                                                                                       \
	X(void*, MapBuffer, (BufferHandle handle), (handle))                                                               \
                                                                                                                       \
	X(TextureHandle, CreateTexture, (const TextureDesc& desc), (desc))                                                 \
	X(void, DestroyTexture, (TextureHandle & handle), (handle))                                                        \
                                                                                                                       \
	X(TextureViewHandle, CreateTextureView, (const TextureViewDesc& desc), (desc))                                     \
	X(void, DestroyTextureView, (TextureViewHandle & handle), (handle))                                                \
                                                                                                                       \
	X(CommandQueue*, GetGpuQueue, (QueueType type), (type))                                                            \
                                                                                                                       \
	X(Swapchain*, CreateSwapchain, (const SwapchainDesc& desc), (desc))                                                \
                                                                                                                       \
	X(void, DestroySwapchain, (Swapchain * swapchain), (swapchain))




	// Base Handle Template
	template <typename Tag> struct Handle {
	public:
		using ValueType                    = uint64_t;
		static constexpr ValueType INVALID = 0;

		Handle(ValueType key)
		    : id(key) {}
		Handle() = default;

		bool IsValid() const { return id != INVALID; }

		// operators
		bool operator==(const Handle& o) const { return id == o.id; }
		bool operator!=(const Handle& o) const { return id != o.id; }
		bool operator<(const Handle& o) const { return id < o.id; }

		ValueType id = INVALID;
	};

	// Typed Handle Definitions
	RENDERX_DEFINE_HANDLE(Buffer)
	RENDERX_DEFINE_HANDLE(BufferView)
	RENDERX_DEFINE_HANDLE(Texture)
	RENDERX_DEFINE_HANDLE(TextureView)
	RENDERX_DEFINE_HANDLE(Sampler)
	RENDERX_DEFINE_HANDLE(Shader)
	RENDERX_DEFINE_HANDLE(Pipeline)
	RENDERX_DEFINE_HANDLE(PipelineLayout)
	RENDERX_DEFINE_HANDLE(Framebuffer)
	RENDERX_DEFINE_HANDLE(RenderPass)
	RENDERX_DEFINE_HANDLE(ResourceGroup)
	RENDERX_DEFINE_HANDLE(QueryPool)
	RENDERX_DEFINE_HANDLE(ResourceGroupLayout)

	// GLM type aliases for consistency Temp (only using them for the testing/debug)
	using Vec2  = glm::vec2;
	using Vec3  = glm::vec3;
	using Vec4  = glm::vec4;
	using Mat3  = glm::mat3;
	using Mat4  = glm::mat4;
	using IVec2 = glm::ivec2;
	using IVec3 = glm::ivec3;
	using IVec4 = glm::ivec4;
	using UVec2 = glm::uvec2;
	using UVec3 = glm::uvec3;
	using UVec4 = glm::uvec4;
	using Quat  = glm::quat;

	// Validation categories
	enum class ValidationCategory : uint32_t {
		NONE            = 0,
		HANDLE          = 1 << 0, // Handle validity checks
		STATE           = 1 << 1, // State machine validation
		RESOURCE        = 1 << 2, // Resource usage validation
		SYNCHRONIZATION = 1 << 3, // Sync primitive validation
		MEMORY          = 1 << 4, // Memory access validation
		PIPELINE        = 1 << 5, // Pipeline state validation
		DESCRIPTOR      = 1 << 6, // Resource binding validation
		COMMAND_LIST    = 1 << 7, // Command recording validation
		RENDER_PASS     = 1 << 8, // Render pass validation
		ALL             = 0xFFFFFFFF
	};
	ENABLE_BITMASK_OPERATORS(ValidationCategory)

	// pending
	enum class ResourceState : uint32_t {
		UNDEFINED        = 0,
		COMMON           = 1 << 0, // Generic read state (D3D12 COMMON)
		VERTEX_BUFFER    = 1 << 1,
		INDEX_BUFFER     = 1 << 2,
		CONSTANT_BUFFER  = 1 << 3,
		SHADER_RESOURCE  = 1 << 4, // Read-only texture/buffer in shader
		UNORDERED_ACCESS = 1 << 5, // Read-write UAV/storage buffer
		RENDER_TARGET    = 1 << 6,
		DEPTH_WRITE      = 1 << 7,
		DEPTH_READ       = 1 << 8,
		TRANSFER_SRC     = 1 << 9,
		TRANSFER_DST     = 1 << 10,
		PRESENT          = 1 << 11,


		// TODO
		INDIRECT_ARGUMENT            = 1 << 12, // not supported
		ACCELERATION_STRUCTURE_READ  = 1 << 13, // not supported
		ACCELERATION_STRUCTURE_WRITE = 1 << 14, // not supported
		RESOLVE_SRC                  = 1 << 15, // not supported
		RESOLVE_DST                  = 1 << 16  // not supported

	};
	ENABLE_BITMASK_OPERATORS(ResourceState)

	enum class GraphicsAPI { NONE, OPENGL, VULKAN };

	enum class Platform : uint8_t { WINDOWS, LINUX, MACOS };

	struct InitDesc {
		GraphicsAPI api;
		void*       displayHandle;
		void*       nativeWindowHandle;
		// vulkan only
		const char** instanceExtensions;
		uint32_t     extensionCount;
	};

	enum class TextureType { TEXTURE_1D, TEXTURE_2D, TEXTURE_3D, TEXTURE_CUBE, TEXTURE_1D_ARRAY, TEXTURE_2D_ARRAY };

	enum class LoadOp { LOAD, CLEAR, DONT_CARE };

	enum class StoreOp { STORE, DONT_CARE };

	enum class Format : uint16_t {
		UNDEFINED = 0,
		R8_UNORM,
		RG8_UNORM,
		RGBA8_UNORM,
		RGBA8_SRGB,
		BGRA8_UNORM,
		BGRA8_SRGB,
		R16_SFLOAT,
		RG16_SFLOAT,
		RGBA16_SFLOAT,
		R32_SFLOAT,
		RG32_SFLOAT,
		RGB32_SFLOAT,
		RGBA32_SFLOAT,
		D24_UNORM_S8_UINT,
		D32_SFLOAT,
		BC1_RGBA_UNORM,
		BC1_RGBA_SRGB,
		BC3_UNORM,
		BC3_SRGB
	};

	enum class Filter {
		NEAREST,
		LINEAR,
		NEAREST_MIPMAP_NEAREST,
		NEAREST_MIPMAP_LINEAR,
		LINEAR_MIPMAP_NEAREST,
		LINEAR_MIPMAP_LINEAR
	};

	enum class TextureWrap { REPEAT, MIRRORED_REPEAT, CLAMP_TO_EDGE, CLAMP_TO_BORDER };

	enum class Topology { POINTS, LINES, LINE_STRIP, TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN };

	enum class CompareFunc { NEVER, LESS, EQUAL, LESS_EQUAL, GREATER, NOT_EQUAL, GREATER_EQUAL, ALWAYS };

	enum class BlendFunc {
		ZERO,
		ONE,
		SRC_COLOR,
		ONE_MINUS_SRC_COLOR,
		DST_COLOR,
		ONE_MINUS_DST_COLOR,
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		DST_ALPHA,
		ONE_MINUS_DST_ALPHA,
		CONSTANT_COLOR,
		ONE_MINUS_CONSTANT_COLOR
	};

	enum class BlendOp { ADD, SUBTRACT, REVERSE_SUBTRACT, MIN, MAX };

	enum class CullMode { NONE, FRONT, BACK, FRONT_AND_BACK };

	enum class FillMode { SOLID, WIREFRAME, POINT };

	enum class MemoryType : uint8_t {
		GPU_ONLY   = 1 << 0, // Device-local, no CPU access
		CPU_TO_GPU = 1 << 1, // Host-visible, optimized for CPU writes
		GPU_TO_CPU = 1 << 2, // Host-visible, optimized for CPU reads (cached)
		CPU_ONLY   = 1 << 3, // Host memory, rarely accessed by GPU
		AUTO       = 1 << 4  // Prefer device-local but allow fallback
	};

	enum class ResourceType : uint8_t {
		CONSTANT_BUFFER,          // Uniform / Constant buffer
		STORAGE_BUFFER,           // Read-only SSBO
		RW_STORAGE_BUFFER,        // Read-write
		TEXTURE_SRV,              // Read-only texture (sampled image)
		TEXTURE_UAV,              // Read-write texture (storage image)
		SAMPLER,                  // Sampler state object
		COMBINED_TEXTURE_SAMPLER, // Combined (GL-style / convenience)
		ACCELERATION_STRUCTURE    // Ray tracing TLAS / BLAS
	};


	enum class PipelineStage : uint8_t {
		NONE            = 0,
		VERTEX          = 1 << 0,
		FRAGMENT        = 1 << 1,
		GEOMETRY        = 1 << 2,
		TESS_CONTROL    = 1 << 3,
		TESS_EVALUATION = 1 << 4,
		COMPUTE         = 1 << 5,
		ALL             = VERTEX | FRAGMENT | COMPUTE | GEOMETRY | TESS_CONTROL | TESS_EVALUATION
	};
	ENABLE_BITMASK_OPERATORS(PipelineStage)

	enum class ResourceGroupFlags : uint8_t {
		NONE            = 0,      // No special flags
		STATIC          = 1 << 0, // Long-lived (per-scene / per-material)
		DYNAMIC         = 1 << 1, // Frequently updated (per-frame / per-draw)
		DYNAMIC_UNIFORM = 1 << 2, // Dynamic uniform buffers (Vulkan dynamic offsets)
		BINDLESS        = 1 << 3, // Bindless descriptors
		BUFFER          = 1 << 4  // Descriptor-buffer based APIs (e.g. VK_EXT_descriptor_buffer)
	};
	ENABLE_BITMASK_OPERATORS(ResourceGroupFlags)

	enum class BufferUsage : uint16_t {
		VERTEX       = 1 << 0,
		INDEX        = 1 << 1,
		UNIFORM      = 1 << 2,
		STORAGE      = 1 << 3,
		INDIRECT     = 1 << 4,
		TRANSFER_SRC = 1 << 5,
		TRANSFER_DST = 1 << 6,
		STATIC       = 1 << 8,
		DYNAMIC      = 1 << 9,
		STREAMING    = 1 << 10
	};
	ENABLE_BITMASK_OPERATORS(BufferUsage)

	struct Viewport {
		int   x, y;
		int   width, height;
		float minDepth, maxDepth;

		Viewport(int x = 0, int y = 0, int w = 0, int h = 0, float minD = 0.0f, float maxD = 1.0f)
		    : x(x),
		      y(y),
		      width(w),
		      height(h),
		      minDepth(minD),
		      maxDepth(maxD) {}

		Viewport(const IVec2& pos, const IVec2& size, float minD = 0.0f, float maxD = 1.0f)
		    : x(pos.x),
		      y(pos.y),
		      width(size.x),
		      height(size.y),
		      minDepth(minD),
		      maxDepth(maxD) {}

		// Factory methods
		static Viewport FromSize(int width, int height) { return Viewport(0, 0, width, height); }

		static Viewport FromSize(const IVec2& size) { return Viewport(0, 0, size.x, size.y); }
	};

	struct Scissor {
		int x, y;
		int width, height;

		Scissor(int x = 0, int y = 0, int w = 0, int h = 0)
		    : x(x),
		      y(y),
		      width(w),
		      height(h) {}
		Scissor(const IVec2& pos, const IVec2& size)
		    : x(pos.x),
		      y(pos.y),
		      width(size.x),
		      height(size.y) {}

		// Factory methods
		static Scissor FromSize(int width, int height) { return Scissor(0, 0, width, height); }

		static Scissor FromSize(const IVec2& size) { return Scissor(0, 0, size.x, size.y); }
	};

	struct ClearColor {
		Vec4 color;

		ClearColor(float r = 0.30f, float g = 0.50f, float b = 0.0f, float a = 1.0f)
		    : color(r, g, b, a) {}
		ClearColor(const Vec4& c)
		    : color(c) {}
		ClearColor(const Vec3& c, float a = 1.0f)
		    : color(c, a) {}

		// Common presets
		static ClearColor Black() { return ClearColor(0.0f, 0.0f, 0.0f, 1.0f); }
		static ClearColor White() { return ClearColor(1.0f, 1.0f, 1.0f, 1.0f); }
		static ClearColor Red() { return ClearColor(1.0f, 0.0f, 0.0f, 1.0f); }
		static ClearColor Green() { return ClearColor(0.0f, 1.0f, 0.0f, 1.0f); }
		static ClearColor Blue() { return ClearColor(0.0f, 0.0f, 1.0f, 1.0f); }
		static ClearColor CornflowerBlue() { return ClearColor(0.392f, 0.584f, 0.929f, 1.0f); }
		static ClearColor DarkGray() { return ClearColor(0.169f, 0.169f, 0.169f, 1.0f); }
		static ClearColor Transparent() { return ClearColor(0.0f, 0.0f, 0.0f, 0.0f); }
	};

	// Vertex input description
	struct VertexAttribute {
		uint32_t location;
		uint32_t binding;
		Format   format;
		uint32_t offset;

		VertexAttribute(uint32_t loc, uint32_t binding, Format format, uint32_t off)
		    : location(loc),
		      binding(binding),
		      format(format),
		      offset(off) {}

		// Factory methods for common formats
		static VertexAttribute Float(uint32_t location, uint32_t binding, uint32_t offset) {
			return VertexAttribute(location, binding, Format::R32_SFLOAT, offset);
		}

		static VertexAttribute Vec2(uint32_t location, uint32_t binding, uint32_t offset) {
			return VertexAttribute(location, binding, Format::RG32_SFLOAT, offset);
		}

		static VertexAttribute Vec3(uint32_t location, uint32_t binding, uint32_t offset) {
			return VertexAttribute(location, binding, Format::RGB32_SFLOAT, offset);
		}

		static VertexAttribute Vec4(uint32_t location, uint32_t binding, uint32_t offset) {
			return VertexAttribute(location, binding, Format::RGBA32_SFLOAT, offset);
		}
	};

	struct VertexBinding {
		uint32_t binding;
		uint32_t stride;
		bool     instanceData = false; // true for per-instance, false for per-vertex

		VertexBinding(uint32_t bind, uint32_t str, bool inst = false)
		    : binding(bind),
		      stride(str),
		      instanceData(inst) {}

		// Factory methods
		static VertexBinding PerVertex(uint32_t binding, uint32_t stride) {
			return VertexBinding(binding, stride, false);
		}

		static VertexBinding PerInstance(uint32_t binding, uint32_t stride) {
			return VertexBinding(binding, stride, true);
		}
	};

	struct VertexInputState {
		std::vector<VertexAttribute> attributes;
		std::vector<VertexBinding>   vertexBindings;

		VertexInputState() = default;

		VertexInputState& addAttribute(const VertexAttribute& attr) {
			attributes.push_back(attr);
			return *this;
		}

		VertexInputState& addBinding(const VertexBinding& binding) {
			vertexBindings.push_back(binding);
			return *this;
		}

		// Common vertex layouts
		static VertexInputState Position3D() {
			VertexInputState state;
			state.addBinding(VertexBinding::PerVertex(0, sizeof(Vec3)));
			state.addAttribute(VertexAttribute::Vec3(0, 0, 0));
			return state;
		}

		static VertexInputState PositionColor() {
			VertexInputState state;
			state.addBinding(VertexBinding::PerVertex(0, sizeof(Vec3) + sizeof(Vec4)));
			state.addAttribute(VertexAttribute::Vec3(0, 0, 0));
			state.addAttribute(VertexAttribute::Vec4(1, 0, sizeof(Vec3)));
			return state;
		}

		static VertexInputState PositionTexCoord() {
			VertexInputState state;
			state.addBinding(VertexBinding::PerVertex(0, sizeof(Vec3) + sizeof(Vec2)));
			state.addAttribute(VertexAttribute::Vec3(0, 0, 0));
			state.addAttribute(VertexAttribute::Vec2(1, 0, sizeof(Vec3)));
			return state;
		}

		static VertexInputState PositionNormalTexCoord() {
			VertexInputState state;
			state.addBinding(VertexBinding::PerVertex(0, sizeof(Vec3) * 2 + sizeof(Vec2)));
			state.addAttribute(VertexAttribute::Vec3(0, 0, 0));
			state.addAttribute(VertexAttribute::Vec3(1, 0, sizeof(Vec3)));
			state.addAttribute(VertexAttribute::Vec2(2, 0, sizeof(Vec3) * 2));
			return state;
		}
	};


	inline bool IsValidBufferFlags(BufferUsage flags) {
		bool hasStatic  = Has(flags, BufferUsage::STATIC);
		bool hasDynamic = Has(flags, BufferUsage::DYNAMIC);

		if (hasStatic && hasDynamic) {
			return false;
		}

		// Must have at least one usage flag (not just Static/Dynamic)
		BufferUsage usageMask = BufferUsage::VERTEX | BufferUsage::INDEX | BufferUsage::UNIFORM | BufferUsage::STORAGE |
		                        BufferUsage::INDIRECT | BufferUsage::TRANSFER_SRC | BufferUsage::TRANSFER_DST;

		if (!Has(usageMask, flags)) {
			return false; // Must have at least one usage
		}

		return true;
	}

	struct BufferDesc {
		BufferUsage usage;
		MemoryType  memoryType;
		size_t      size;
		uint32_t    bindingCount = 0;
		const void* initialData  = nullptr;

		BufferDesc() = default;

		BufferDesc(BufferUsage usage, MemoryType memType, size_t sz, const void* data = nullptr)
		    : usage(usage),
		      memoryType(memType),
		      size(sz),
		      initialData(data) {}

		// Factory methods for common buffer types
		static BufferDesc
		VertexBuffer(size_t size, MemoryType memType = MemoryType::GPU_ONLY, const void* data = nullptr) {
			return BufferDesc(BufferUsage::VERTEX | BufferUsage::TRANSFER_DST, memType, size, data);
		}

		static BufferDesc
		IndexBuffer(size_t size, MemoryType memType = MemoryType::GPU_ONLY, const void* data = nullptr) {
			return BufferDesc(BufferUsage::INDEX | BufferUsage::TRANSFER_DST, memType, size, data);
		}

		static BufferDesc
		UniformBuffer(size_t size, MemoryType memType = MemoryType::CPU_TO_GPU, const void* data = nullptr) {
			BufferDesc desc(BufferUsage::UNIFORM, memType, size, data);
			return desc;
		}

		static BufferDesc
		StorageBuffer(size_t size, MemoryType memType = MemoryType::GPU_ONLY, const void* data = nullptr) {
			return BufferDesc(BufferUsage::STORAGE, memType, size, data);
		}

		static BufferDesc StagingBuffer(size_t size) {
			return BufferDesc(BufferUsage::TRANSFER_SRC, MemoryType::CPU_TO_GPU, size, nullptr);
		}

		static BufferDesc ReadbackBuffer(size_t size) {
			return BufferDesc(BufferUsage::TRANSFER_DST, MemoryType::GPU_TO_CPU, size, nullptr);
		}

		// Dynamic buffers (frequently updated)
		static BufferDesc DynamicVertexBuffer(size_t size) {
			return BufferDesc(BufferUsage::VERTEX | BufferUsage::DYNAMIC, MemoryType::CPU_TO_GPU, size, nullptr);
		}

		static BufferDesc DynamicUniformBuffer(size_t size) {
			return BufferDesc(BufferUsage::UNIFORM | BufferUsage::DYNAMIC, MemoryType::CPU_TO_GPU, size, nullptr);
		}
	};

	struct BufferCopyRegion {
		uint32_t srcOffset = 0;
		uint32_t dstOffset = 0;
		uint32_t size      = 0;

		BufferCopyRegion() = default;
		BufferCopyRegion(uint32_t src, uint32_t dst, uint32_t sz)
		    : srcOffset(src),
		      dstOffset(dst),
		      size(sz) {}

		static BufferCopyRegion FullBuffer(uint32_t size) { return BufferCopyRegion(0, 0, size); }
	};

	struct BufferViewDesc {
		BufferHandle buffer;
		uint32_t     offset = 0;
		uint32_t     range  = 0; // 0 = whole buffer

		BufferViewDesc() = default;
		BufferViewDesc(BufferHandle buf, uint32_t off = 0, uint32_t rng = 0)
		    : buffer(buf),
		      offset(off),
		      range(rng) {}

		static BufferViewDesc WholeBuffer(BufferHandle buffer) { return BufferViewDesc(buffer, 0, 0); }
	};

	struct SamplerDesc {
		Filter      minFilter;
		Filter      magFilter;
		TextureWrap wrapU;
		TextureWrap wrapV;
		TextureWrap wrapW;
		float       maxAnisotropy;
		bool        enableCompare;
		CompareFunc compareFunc;
		Vec4        borderColor;

		SamplerDesc()
		    : minFilter(Filter::LINEAR),
		      magFilter(Filter::LINEAR),
		      wrapU(TextureWrap::REPEAT),
		      wrapV(TextureWrap::REPEAT),
		      wrapW(TextureWrap::REPEAT),
		      maxAnisotropy(1.0f),
		      enableCompare(false),
		      compareFunc(CompareFunc::NEVER),
		      borderColor(0.0f, 0.0f, 0.0f, 1.0f) {}

		// Common sampler presets
		static SamplerDesc LinearRepeat() {
			SamplerDesc desc;
			desc.minFilter = Filter::LINEAR;
			desc.magFilter = Filter::LINEAR;
			desc.wrapU = desc.wrapV = desc.wrapW = TextureWrap::REPEAT;
			return desc;
		}

		static SamplerDesc LinearClamp() {
			SamplerDesc desc;
			desc.minFilter = Filter::LINEAR;
			desc.magFilter = Filter::LINEAR;
			desc.wrapU = desc.wrapV = desc.wrapW = TextureWrap::CLAMP_TO_EDGE;
			return desc;
		}

		static SamplerDesc NearestRepeat() {
			SamplerDesc desc;
			desc.minFilter = Filter::NEAREST;
			desc.magFilter = Filter::NEAREST;
			desc.wrapU = desc.wrapV = desc.wrapW = TextureWrap::REPEAT;
			return desc;
		}

		static SamplerDesc NearestClamp() {
			SamplerDesc desc;
			desc.minFilter = Filter::NEAREST;
			desc.magFilter = Filter::NEAREST;
			desc.wrapU = desc.wrapV = desc.wrapW = TextureWrap::CLAMP_TO_EDGE;
			return desc;
		}

		static SamplerDesc Anisotropic(float maxAniso = 16.0f) {
			SamplerDesc desc;
			desc.minFilter     = Filter::LINEAR_MIPMAP_LINEAR;
			desc.magFilter     = Filter::LINEAR;
			desc.maxAnisotropy = maxAniso;
			desc.wrapU = desc.wrapV = desc.wrapW = TextureWrap::REPEAT;
			return desc;
		}

		static SamplerDesc ShadowMap() {
			SamplerDesc desc;
			desc.minFilter = Filter::LINEAR;
			desc.magFilter = Filter::LINEAR;
			desc.wrapU = desc.wrapV = desc.wrapW = TextureWrap::CLAMP_TO_BORDER;
			desc.enableCompare                   = true;
			desc.compareFunc                     = CompareFunc::LESS;
			desc.borderColor                     = Vec4(1.0f);
			return desc;
		}
	};

	enum class TextureUsageFlags : uint32_t {
		NONE          = 0,
		TRANSFER_SRC  = 1 << 0, // Can be copied from
		TRANSFER_DST  = 1 << 1, // Can be copied to
		SAMPLED       = 1 << 2, // Shader sampling
		STORAGE       = 1 << 3, // Read/write in compute
		RENDER_TARGET = 1 << 4, // Color attachment
		DEPTH_STENCIL = 1 << 5, // Depth/stencil attachment
	};
	ENABLE_BITMASK_OPERATORS(TextureUsageFlags);

	struct TextureDesc {
		TextureType       type;
		uint32_t          width;
		uint32_t          height;
		uint32_t          depth;
		uint32_t          mipLevels;
		uint32_t          arrayLayers;
		uint32_t          sampleCount;
		Format            format;
		TextureUsageFlags usage;
		MemoryType        memoryType;
		bool              generateMips;
		const char*       debugName;

		// Constructors
		TextureDesc(uint32_t    w   = 1,
		            uint32_t    h   = 1,
		            Format      fmt = Format::RGBA8_UNORM,
		            TextureType t   = TextureType::TEXTURE_2D)
		    : type(t),
		      width(w),
		      height(h),
		      depth(1),
		      mipLevels(1),
		      arrayLayers(1),
		      sampleCount(1),
		      format(fmt),
		      usage(TextureUsageFlags::SAMPLED),
		      memoryType(MemoryType::GPU_ONLY),
		      generateMips(false),
		      debugName(nullptr) {}

		TextureDesc(const IVec2& size, Format fmt = Format::RGBA8_UNORM, TextureType t = TextureType::TEXTURE_2D)
		    : TextureDesc(size.x, size.y, fmt, t) {}

		// Factory methods for common texture types
		static TextureDesc Texture2D(uint32_t width, uint32_t height, Format format = Format::RGBA8_UNORM) {
			TextureDesc desc(width, height, format, TextureType::TEXTURE_2D);
			desc.usage = TextureUsageFlags::SAMPLED | TextureUsageFlags::TRANSFER_DST;
			return desc;
		}

		static TextureDesc RenderTarget(uint32_t width, uint32_t height, Format format = Format::RGBA8_UNORM) {
			TextureDesc desc(width, height, format, TextureType::TEXTURE_2D);
			desc.usage = TextureUsageFlags::RENDER_TARGET | TextureUsageFlags::SAMPLED;
			return desc;
		}

		static TextureDesc DepthStencil(uint32_t width, uint32_t height, Format format = Format::D24_UNORM_S8_UINT) {
			TextureDesc desc(width, height, format, TextureType::TEXTURE_2D);
			desc.usage = TextureUsageFlags::DEPTH_STENCIL;
			return desc;
		}

		static TextureDesc StorageTexture(uint32_t width, uint32_t height, Format format = Format::RGBA8_UNORM) {
			TextureDesc desc(width, height, format, TextureType::TEXTURE_2D);
			desc.usage = TextureUsageFlags::STORAGE | TextureUsageFlags::SAMPLED;
			return desc;
		}

		static TextureDesc Cubemap(uint32_t size, Format format = Format::RGBA8_UNORM) {
			TextureDesc desc(size, size, format, TextureType::TEXTURE_CUBE);
			desc.arrayLayers = 6;
			desc.usage       = TextureUsageFlags::SAMPLED | TextureUsageFlags::TRANSFER_DST;
			return desc;
		}

		static TextureDesc
		TextureArray(uint32_t width, uint32_t height, uint32_t layers, Format format = Format::RGBA8_UNORM) {
			TextureDesc desc(width, height, format, TextureType::TEXTURE_2D_ARRAY);
			desc.arrayLayers = layers;
			desc.usage       = TextureUsageFlags::SAMPLED | TextureUsageFlags::TRANSFER_DST;
			return desc;
		}

		TextureDesc& setMips(uint32_t levels) {
			mipLevels = levels;
			return *this;
		}

		TextureDesc& setGeneratedMips() {
			generateMips = true;
			return *this;
		}

		TextureDesc& setUsage(TextureUsageFlags newUsage) {
			usage = newUsage;
			return *this;
		}

		TextureDesc& setDebugName(const char* name) {
			debugName = name;
			return *this;
		}
	};

	struct TextureViewDesc {
		TextureHandle texture;
		TextureType   viewType = TextureType::TEXTURE_2D;
		Format        format   = Format::UNDEFINED;
		uint32_t      baseMipLevel;
		uint32_t      mipLevelCount;
		uint32_t      baseArrayLayer;
		uint32_t      arrayLayerCount;
		const char*   debugName;

		TextureViewDesc()
		    : baseMipLevel(0),
		      mipLevelCount(1),
		      baseArrayLayer(0),
		      arrayLayerCount(1),
		      debugName(nullptr) {}

		TextureViewDesc(TextureHandle tex)
		    : texture(tex),
		      baseMipLevel(0),
		      mipLevelCount(1),
		      baseArrayLayer(0),
		      arrayLayerCount(1),
		      debugName(nullptr) {}

		static TextureViewDesc Default(TextureHandle texture) { return TextureViewDesc(texture); }

		static TextureViewDesc SingleMip(TextureHandle texture, uint32_t mipLevel) {
			TextureViewDesc desc(texture);
			desc.baseMipLevel  = mipLevel;
			desc.mipLevelCount = 1;
			return desc;
		}

		static TextureViewDesc CubeFace(TextureHandle texture, uint32_t faceIndex) {
			TextureViewDesc desc(texture);
			desc.viewType        = TextureType::TEXTURE_2D;
			desc.baseArrayLayer  = faceIndex;
			desc.arrayLayerCount = 1;
			return desc;
		}

		static TextureViewDesc ArrayLayer(TextureHandle texture, uint32_t layerIndex) {
			TextureViewDesc desc(texture);
			desc.baseArrayLayer  = layerIndex;
			desc.arrayLayerCount = 1;
			return desc;
		}
	};

	struct TextureCopyRegion {
		uint32_t srcMipLevel   = 0;
		uint32_t srcArrayLayer = 0;
		IVec3    srcOffset     = IVec3(0);

		uint32_t dstMipLevel   = 0;
		uint32_t dstArrayLayer = 0;
		IVec3    dstOffset     = IVec3(0);

		IVec3 extent = IVec3(0);

		static TextureCopyRegion FullTexture(uint32_t width, uint32_t height, uint32_t depth = 1) {
			TextureCopyRegion region;
			region.extent = IVec3(width, height, depth);
			return region;
		}
	};

	struct ShaderDesc {
		PipelineStage        type;
		std::string          source;
		std::vector<uint8_t> bytecode;
		std::string          entryPoint;

		ShaderDesc(PipelineStage t = PipelineStage::VERTEX, const std::string& src = "")
		    : type(t),
		      source(src),
		      entryPoint("main") {}

		ShaderDesc(PipelineStage t, const std::vector<uint8_t>& code, const std::string entry = "main")
		    : type(t),
		      bytecode(code),
		      entryPoint(entry) {}

		// Factory methods
		static ShaderDesc FromSource(PipelineStage stage, const std::string& source) {
			return ShaderDesc(stage, source);
		}

		static ShaderDesc FromBytecode(PipelineStage stage, const std::vector<uint8_t>& bytecode) {
			return ShaderDesc(stage, bytecode);
		}

		static ShaderDesc VertexShader(const std::string& source) { return ShaderDesc(PipelineStage::VERTEX, source); }

		static ShaderDesc FragmentShader(const std::string& source) {
			return ShaderDesc(PipelineStage::FRAGMENT, source);
		}

		static ShaderDesc ComputeShader(const std::string& source) {
			return ShaderDesc(PipelineStage::COMPUTE, source);
		}
	};

	// Render state
	struct RasterizerState {
		FillMode fillMode;
		CullMode cullMode;
		bool     frontCounterClockwise;
		float    depthBias;
		float    depthBiasClamp;
		float    slopeScaledDepthBias;
		bool     depthClipEnable;
		bool     scissorEnable;
		bool     multisampleEnable;

		RasterizerState()
		    : fillMode(FillMode::SOLID),
		      cullMode(CullMode::NONE),
		      frontCounterClockwise(false),
		      depthBias(0.0f),
		      depthBiasClamp(0.0f),
		      slopeScaledDepthBias(0.0f),
		      depthClipEnable(true),
		      scissorEnable(false),
		      multisampleEnable(false) {}

		// Common presets
		static RasterizerState Default() { return RasterizerState(); }

		static RasterizerState CullBack() {
			RasterizerState state;
			state.cullMode = CullMode::BACK;
			return state;
		}

		static RasterizerState CullFront() {
			RasterizerState state;
			state.cullMode = CullMode::FRONT;
			return state;
		}

		static RasterizerState Wireframe() {
			RasterizerState state;
			state.fillMode = FillMode::WIREFRAME;
			state.cullMode = CullMode::NONE;
			return state;
		}

		static RasterizerState ShadowMap() {
			RasterizerState state;
			state.cullMode             = CullMode::FRONT;
			state.depthBias            = 1.25f;
			state.slopeScaledDepthBias = 1.75f;
			return state;
		}
	};

	struct DepthStencilState {
		bool        depthEnable;
		bool        depthWriteEnable;
		bool        stencilEnable;
		CompareFunc depthFunc;
		uint8_t     stencilReadMask;
		uint8_t     stencilWriteMask;

		DepthStencilState()
		    : depthEnable(true),
		      depthWriteEnable(true),
		      depthFunc(CompareFunc::LESS),
		      stencilEnable(false),
		      stencilReadMask(0xFF),
		      stencilWriteMask(0xFF) {}

		// Common presets
		static DepthStencilState Default() { return DepthStencilState(); }

		static DepthStencilState DepthReadOnly() {
			DepthStencilState state;
			state.depthEnable      = true;
			state.depthWriteEnable = false;
			return state;
		}

		static DepthStencilState NoDepth() {
			DepthStencilState state;
			state.depthEnable      = false;
			state.depthWriteEnable = false;
			return state;
		}

		static DepthStencilState DepthEqual() {
			DepthStencilState state;
			state.depthFunc = CompareFunc::EQUAL;
			return state;
		}
	};

	struct BlendState {
		bool enable;

		BlendFunc srcColor;
		BlendFunc dstColor;
		BlendFunc srcAlpha;
		BlendFunc dstAlpha;

		BlendOp colorOp;
		BlendOp alphaOp;

		Vec4 blendFactor;

		BlendState()
		    : enable(false),
		      srcColor(BlendFunc::SRC_ALPHA),
		      dstColor(BlendFunc::ONE_MINUS_SRC_ALPHA),
		      srcAlpha(BlendFunc::ONE),
		      dstAlpha(BlendFunc::ZERO),
		      colorOp(BlendOp::ADD),
		      alphaOp(BlendOp::ADD),
		      blendFactor(1.0f) {}

		// Common presets
		static BlendState Disabled() {
			BlendState state;
			state.enable = false;
			return state;
		}

		static BlendState AlphaBlend() {
			BlendState state;
			state.enable   = true;
			state.srcColor = BlendFunc::SRC_ALPHA;
			state.dstColor = BlendFunc::ONE_MINUS_SRC_ALPHA;
			state.srcAlpha = BlendFunc::ONE;
			state.dstAlpha = BlendFunc::ZERO;
			return state;
		}

		static BlendState additive() {
			BlendState state;
			state.enable   = true;
			state.srcColor = BlendFunc::ONE;
			state.dstColor = BlendFunc::ONE;
			state.srcAlpha = BlendFunc::ONE;
			state.dstAlpha = BlendFunc::ONE;
			return state;
		}

		static BlendState Multiply() {
			BlendState state;
			state.enable   = true;
			state.srcColor = BlendFunc::DST_COLOR;
			state.dstColor = BlendFunc::ZERO;
			state.srcAlpha = BlendFunc::DST_ALPHA;
			state.dstAlpha = BlendFunc::ZERO;
			return state;
		}

		static BlendState Premultiplied() {
			BlendState state;
			state.enable   = true;
			state.srcColor = BlendFunc::ONE;
			state.dstColor = BlendFunc::ONE_MINUS_SRC_ALPHA;
			state.srcAlpha = BlendFunc::ONE;
			state.dstAlpha = BlendFunc::ONE_MINUS_SRC_ALPHA;
			return state;
		}
	};


	// Describes a single binding slot in a ResourceGroupLayout
	struct ResourceGroupLayoutItem {
		// Binding index (e.g., layout(binding = 0))
		uint32_t binding;
		// What kind of resource
		ResourceType type;
		// Which shader stages can access this
		PipelineStage stages;
		// Array size (1 for non-arrays, >1 for arrays) For Bindless indexing
		uint32_t count;

		ResourceGroupLayoutItem()
		    : binding(0),
		      type(ResourceType::CONSTANT_BUFFER),
		      stages(PipelineStage::ALL),
		      count(1) {}

		ResourceGroupLayoutItem(uint32_t bind, ResourceType t, PipelineStage s = PipelineStage::ALL, uint32_t cnt = 1)
		    : binding(bind),
		      type(t),
		      stages(s),
		      count(cnt) {}

		// Convenience factory methods
		static ResourceGroupLayoutItem ConstantBuffer(uint32_t binding, PipelineStage stages = PipelineStage::ALL) {
			return ResourceGroupLayoutItem(binding, ResourceType::CONSTANT_BUFFER, stages, 1);
		}

		static ResourceGroupLayoutItem
		StorageBuffer(uint32_t binding, PipelineStage stages = PipelineStage::ALL, bool writable = false) {
			return ResourceGroupLayoutItem(
			    binding, writable ? ResourceType::RW_STORAGE_BUFFER : ResourceType::STORAGE_BUFFER, stages, 1);
		}

		static ResourceGroupLayoutItem
		Texture_SRV(uint32_t binding, PipelineStage stages = PipelineStage::ALL, uint32_t count = 1) {
			return ResourceGroupLayoutItem(binding, ResourceType::TEXTURE_SRV, stages, count);
		}

		static ResourceGroupLayoutItem
		Texture_UAV(uint32_t binding, PipelineStage stages = PipelineStage::ALL, uint32_t count = 1) {
			return ResourceGroupLayoutItem(binding, ResourceType::TEXTURE_UAV, stages, count);
		}

		static ResourceGroupLayoutItem Sampler(uint32_t binding, PipelineStage stages = PipelineStage::ALL) {
			return ResourceGroupLayoutItem(binding, ResourceType::SAMPLER, stages, 1);
		}

		static ResourceGroupLayoutItem CombinedTextureSampler(uint32_t      binding,
		                                                      PipelineStage stages = PipelineStage::ALL) {
			return ResourceGroupLayoutItem(binding, ResourceType::COMBINED_TEXTURE_SAMPLER, stages, 1);
		}
	};

	/// Describes the layout/structure of a pipeline inputs
	struct ResourceGroupLayoutDesc {
		std::vector<ResourceGroupLayoutItem> resourcebindings;
		ResourceGroupFlags                   flags     = ResourceGroupFlags::STATIC;
		const char*                          debugName = nullptr;

		ResourceGroupLayoutDesc() = default;

		ResourceGroupLayoutDesc(const std::vector<ResourceGroupLayoutItem>& items)
		    : resourcebindings(items) {}

		ResourceGroupLayoutDesc& addBinding(const ResourceGroupLayoutItem& item) {
			resourcebindings.push_back(item);
			return *this;
		}

		ResourceGroupLayoutDesc& setFlags(ResourceGroupFlags f) {
			flags = f;
			return *this;
		}

		ResourceGroupLayoutDesc& setDebugName(const char* name) {
			debugName = name;
			return *this;
		}
	};

	/// Describes a single resource binding in a descriptor set instance
	struct ResourceGroupItem {
		uint32_t     binding;
		ResourceType type;
		uint32_t     arrayIndex = 0; // For Bindless Resource to arrays of descriptors

		struct CombinedHandles {
			TextureViewHandle texture;
			SamplerHandle     sampler;
		};

		union {
			BufferViewHandle  bufferView;
			TextureViewHandle textureView;
			SamplerHandle     sampler;
			uint64_t          rawHandle;
			CombinedHandles   combinedHandles;
		};

		ResourceGroupItem()
		    : binding(0),
		      type(ResourceType::CONSTANT_BUFFER),
		      rawHandle(0) {}

		// Convenience factory methods
		static ResourceGroupItem ConstantBuffer(uint32_t binding, BufferViewHandle buf) {
			ResourceGroupItem item;
			item.binding    = binding;
			item.type       = ResourceType::CONSTANT_BUFFER;
			item.bufferView = buf;
			return item;
		}

		static ResourceGroupItem StorageBuffer(uint32_t binding, BufferViewHandle buf, bool writable = false) {
			ResourceGroupItem item;
			item.binding    = binding;
			item.type       = writable ? ResourceType::RW_STORAGE_BUFFER : ResourceType::STORAGE_BUFFER;
			item.bufferView = buf;
			return item;
		}

		static ResourceGroupItem Texture_SRV(uint32_t binding, TextureViewHandle tex, uint32_t arrayIndex = 0) {
			ResourceGroupItem item;
			item.binding     = binding;
			item.type        = ResourceType::TEXTURE_SRV;
			item.textureView = tex;
			item.arrayIndex  = arrayIndex;
			return item;
		}

		static ResourceGroupItem Texture_UAV(uint32_t binding, TextureViewHandle tex, uint32_t mipLevel = 0) {
			ResourceGroupItem item;
			item.binding     = binding;
			item.type        = ResourceType::TEXTURE_UAV;
			item.textureView = tex;
			return item;
		}

		static ResourceGroupItem Sampler(uint32_t binding, SamplerHandle samp) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type    = ResourceType::SAMPLER;
			item.sampler = samp;
			return item;
		}

		static ResourceGroupItem CombinedTextureSampler(uint32_t binding, TextureViewHandle tex, SamplerHandle samp) {
			ResourceGroupItem item;
			item.binding                 = binding;
			item.type                    = ResourceType::COMBINED_TEXTURE_SAMPLER;
			item.combinedHandles.texture = tex;
			item.combinedHandles.sampler = samp;
			return item;
		}
	};

	/// Describes a ResourceGroup instance
	struct ResourceGroupDesc {
		PipelineLayoutHandle           pipelinelayout;
		ResourceGroupLayoutHandle      layout;
		std::vector<ResourceGroupItem> Resources;
		uint32_t                       dynamicOffset = 0;
		const char*                    debugName     = nullptr;

		ResourceGroupDesc() = default;

		ResourceGroupDesc(PipelineLayoutHandle layoutHandle, const std::vector<ResourceGroupItem>& items)
		    : pipelinelayout(layoutHandle),
		      Resources(items) {}

		ResourceGroupDesc& addResource(const ResourceGroupItem& item) {
			Resources.push_back(item);
			return *this;
		}

		ResourceGroupDesc& setDebugName(const char* name) {
			debugName = name;
			return *this;
		}
	};


	// Pipeline description
	struct PipelineDesc {
		std::vector<ShaderHandle> shaders;
		VertexInputState          vertexInputState{};
		Topology                  primitiveType{};
		RasterizerState           rasterizer{};
		DepthStencilState         depthStencil{};
		BlendState                blend{};
		PipelineLayoutHandle      layout;
		std::vector<Format>       colorFromats;
		Format                    depthFormat = Format::UNDEFINED;
		RenderPassHandle          renderPass;

		PipelineDesc()
		    : primitiveType(Topology::TRIANGLES),
		      renderPass(0) {}

		PipelineDesc& addShader(ShaderHandle shader) {
			shaders.push_back(shader);
			return *this;
		}

		PipelineDesc& setVertexInput(const VertexInputState& state) {
			vertexInputState = state;
			return *this;
		}

		PipelineDesc& setTopology(Topology topo) {
			primitiveType = topo;
			return *this;
		}

		PipelineDesc& setRasterizer(const RasterizerState& state) {
			rasterizer = state;
			return *this;
		}

		PipelineDesc& setDepthStencil(const DepthStencilState& state) {
			depthStencil = state;
			return *this;
		}

		PipelineDesc& setBlend(const BlendState& state) {
			blend = state;
			return *this;
		}

		PipelineDesc& setLayout(PipelineLayoutHandle layoutHandle) {
			layout = layoutHandle;
			return *this;
		}

		PipelineDesc& addColorFormat(Format format) {
			colorFromats.push_back(format);
			return *this;
		}

		PipelineDesc& setDepthFormat(Format format) {
			depthFormat = format;
			return *this;
		}
	};

	struct ClearValue {
		ClearColor color;
		float      depth   = 1.0f;
		uint8_t    stencil = 0;

		ClearValue() = default;
		ClearValue(const ClearColor& col)
		    : color(col) {}
		ClearValue(float d, uint8_t s = 0)
		    : depth(d),
		      stencil(s) {}

		static ClearValue Color(const ClearColor& col) { return ClearValue(col); }

		static ClearValue Depth(float d = 1.0f) { return ClearValue(d); }

		static ClearValue DepthStencil(float d, uint8_t s) { return ClearValue(d, s); }
	};


	// Render pass description
	struct AttachmentDesc {
		Format            format  = Format::BGRA8_SRGB;
		LoadOp            loadOp  = LoadOp::CLEAR;
		StoreOp           storeOp = StoreOp::STORE;
		TextureViewHandle handle;

		AttachmentDesc(Format fmt = Format::BGRA8_SRGB)
		    : format(fmt) {}

		static AttachmentDesc RenderTarget(TextureViewHandle view, Format fmt = Format::BGRA8_SRGB) {
			AttachmentDesc desc(fmt);
			desc.handle  = view;
			desc.loadOp  = LoadOp::CLEAR;
			desc.storeOp = StoreOp::STORE;
			return desc;
		}

		static AttachmentDesc LoadAndStore(TextureViewHandle view, Format fmt = Format::BGRA8_SRGB) {
			AttachmentDesc desc(fmt);
			desc.handle  = view;
			desc.loadOp  = LoadOp::LOAD;
			desc.storeOp = StoreOp::STORE;
			return desc;
		}
	};

	struct DepthStencilAttachmentDesc {
		TextureViewHandle handle;
		Format            format         = Format::D24_UNORM_S8_UINT;
		LoadOp            depthLoadOp    = LoadOp::CLEAR;
		StoreOp           depthStoreOp   = StoreOp::STORE;
		LoadOp            stencilLoadOp  = LoadOp::DONT_CARE;
		StoreOp           stencilStoreOp = StoreOp::DONT_CARE;

		DepthStencilAttachmentDesc(Format format = Format::D24_UNORM_S8_UINT)
		    : format(format) {}

		static DepthStencilAttachmentDesc Default(TextureViewHandle view, Format fmt = Format::D24_UNORM_S8_UINT) {
			DepthStencilAttachmentDesc desc(fmt);
			desc.handle = view;
			return desc;
		}

		static DepthStencilAttachmentDesc DepthReadOnly(TextureViewHandle view,
		                                                Format            fmt = Format::D24_UNORM_S8_UINT) {
			DepthStencilAttachmentDesc desc(fmt);
			desc.handle       = view;
			desc.depthLoadOp  = LoadOp::LOAD;
			desc.depthStoreOp = StoreOp::DONT_CARE;
			return desc;
		}
	};


	struct RenderPassDesc {
		std::vector<AttachmentDesc> colorAttachments;
		DepthStencilAttachmentDesc  depthStencilAttachment;
		bool                        hasDepthStencil;

		RenderPassDesc()
		    : depthStencilAttachment(Format::D24_UNORM_S8_UINT),
		      hasDepthStencil(false) {}

		RenderPassDesc& addColorAttachment(const AttachmentDesc& attachment) {
			colorAttachments.push_back(attachment);
			return *this;
		}

		RenderPassDesc& setDepthStencil(const DepthStencilAttachmentDesc& attachment) {
			depthStencilAttachment = attachment;
			hasDepthStencil        = true;
			return *this;
		}
	};

	struct FramebufferDesc {
		std::vector<TextureViewHandle> colorAttachments;
		TextureViewHandle              depthStencilAttachment;
		uint32_t                       width  = 0;
		uint32_t                       height = 0;
		uint32_t                       layers = 1;

		FramebufferDesc(int w = 0, int h = 0)
		    : depthStencilAttachment(0),
		      width(w),
		      height(h) {}

		FramebufferDesc(const IVec2& size)
		    : depthStencilAttachment(0),
		      width(size.x),
		      height(size.y) {}

		FramebufferDesc& addColorAttachment(TextureViewHandle view) {
			colorAttachments.push_back(view);
			return *this;
		}

		FramebufferDesc& setDepthStencil(TextureViewHandle view) {
			depthStencilAttachment = view;
			return *this;
		}

		FramebufferDesc& setSize(uint32_t w, uint32_t h) {
			width  = w;
			height = h;
			return *this;
		}
	};

	struct RenderingDesc {
		int                         width;
		int                         height;
		std::vector<AttachmentDesc> colorAttachments;
		DepthStencilAttachmentDesc  depthStencilAttachment{ Format::D24_UNORM_S8_UINT };
		bool                        hasDepthStencil;

		RenderingDesc(int w = 0, int h = 0)
		    : width(w),
		      height(h),
		      hasDepthStencil(false) {}

		RenderingDesc& addColorAttachment(const AttachmentDesc& attachment) {
			colorAttachments.push_back(attachment);
			return *this;
		}

		RenderingDesc& setDepthStencil(const DepthStencilAttachmentDesc& attachment) {
			depthStencilAttachment = attachment;
			hasDepthStencil        = true;
			return *this;
		}

		RenderingDesc& setSize(int w, int h) {
			width  = w;
			height = h;
			return *this;
		}
	};

	// Statistics and debug info for a single frame.
	struct RenderStats {
		uint32_t drawCalls;
		uint32_t triangles;
		uint32_t vertices;
		uint32_t bufferBinds;
		uint32_t textureBinds;
		uint32_t shaderBinds;

		RenderStats()
		    : drawCalls(0),
		      triangles(0),
		      vertices(0),
		      bufferBinds(0),
		      textureBinds(0),
		      shaderBinds(0) {}

		void Reset() { drawCalls = triangles = vertices = bufferBinds = textureBinds = shaderBinds = 0; }
	};

	enum class CommandListState : uint8_t {
		INITIAL,    // Just created, ready to begin recording
		RECORDING,  // Currently recording commands
		EXECUTABLE, // Recording finished, ready to submit
		SUBMITTED,  // Submitted to queue, waiting for execution
		COMPLETED,  // Execution finished
		INVALID     // Error state or destroyed
	};

	inline const char* CommandListStateToString(CommandListState state) {
		switch (state) {
		case CommandListState::INITIAL: return "INITIAL";
		case CommandListState::RECORDING: return "RECORDING";
		case CommandListState::EXECUTABLE: return "EXECUTABLE";
		case CommandListState::SUBMITTED: return "SUBMITTED";
		case CommandListState::COMPLETED: return "COMPLETED";
		case CommandListState::INVALID: return "INVALID";
		default: return "UNKNOWN";
		}
	}

	// Queue Types
	enum class QueueType : uint8_t {
		GRAPHICS, // Graphics + compute(if the GPU supports) + transfer operations
		COMPUTE,  // Compute + transfer operations
		TRANSFER  // Transfer operations only
	};

	// Synchronization Primitives
	struct Timeline {
		uint64_t value;

		Timeline(uint64_t v = 0)
		    : value(v) {}

		bool operator==(const Timeline& other) const { return value == other.value; }
		bool operator!=(const Timeline& other) const { return value != other.value; }
		bool operator<(const Timeline& other) const { return value < other.value; }
		bool operator<=(const Timeline& other) const { return value <= other.value; }
		bool operator>(const Timeline& other) const { return value > other.value; }
		bool operator>=(const Timeline& other) const { return value >= other.value; }

		Timeline& operator++() {
			++value;
			return *this;
		}
		Timeline operator++(int) {
			Timeline temp = *this;
			++value;
			return temp;
		}
	};

	class RENDERX_EXPORT CommandList {
	public:
		virtual ~CommandList() = default;

		virtual void open()  = 0;
		virtual void close() = 0;

		virtual void setPipeline(const PipelineHandle& pipeline)                      = 0;
		virtual void setVertexBuffer(const BufferHandle& buffer, uint64_t offset = 0) = 0;
		virtual void setIndexBuffer(const BufferHandle& buffer, uint64_t offset = 0)  = 0;
		virtual void setResourceGroup(const ResourceGroupHandle& handle)              = 0;
		virtual void setFramebuffer(FramebufferHandle handle)                         = 0;

		virtual void beginRenderPass(RenderPassHandle pass, const void* clearValues, uint32_t clearCount) = 0;
		virtual void endRenderPass()                                                                      = 0;

		virtual void beginRendering(const RenderingDesc& desc) = 0;
		virtual void endRendering()                            = 0;

		virtual void writeBuffer(BufferHandle handle, const void* data, uint32_t offset, uint32_t size) = 0;
		virtual void copyBuffer(BufferHandle src, BufferHandle dst, const BufferCopyRegion& region)     = 0;
		virtual void
		copyBufferToTexture(BufferHandle srcBuffer, TextureHandle dstTexture, const TextureCopyRegion& region) = 0;
		virtual void
		copyTexture(TextureHandle srcTexture, TextureHandle dstTexture, const TextureCopyRegion& region) = 0;
		virtual void
		copyTextureToBuffer(TextureHandle srcTexture, BufferHandle dstBuffer, const TextureCopyRegion& region) = 0;

		virtual void draw(uint32_t vertexCount,
		                  uint32_t instanceCount = 1,
		                  uint32_t firstVertex   = 0,
		                  uint32_t firstInstance = 0)        = 0;
		virtual void drawIndexed(uint32_t indexCount,
		                         int32_t  vertexOffset  = 0,
		                         uint32_t instanceCount = 1,
		                         uint32_t firstIndex    = 0,
		                         uint32_t firstInstance = 0) = 0;
	};
	// Synchronization dependency between queues
	struct QueueDependency {
		QueueType waitQueue; // Queue that needs to wait
		Timeline  waitValue; // Timeline value to wait for

		QueueDependency(QueueType queue = QueueType::GRAPHICS, Timeline value = Timeline(0))
		    : waitQueue(queue),
		      waitValue(value) {}
	};


	// This Class Allocates a cpu side Recoding object CommandList
	class RENDERX_EXPORT CommandAllocator {
	public:
		virtual ~CommandAllocator() = default;

		virtual CommandList* Allocate()               = 0;
		virtual void         Reset(CommandList* list) = 0;
		virtual void         Free(CommandList* list)  = 0;
		virtual void         Reset()                  = 0;
	};
	// Command List Submission
	struct SubmitInfo {
		CommandList*                 commandList       = nullptr;
		uint32_t                     commandListCount  = 0;
		bool                         writesToSwapchain = false;
		std::vector<QueueDependency> waitDependencies;

		SubmitInfo() = default;

		SubmitInfo(CommandList* cmd, uint32_t count = 1)
		    : commandList(cmd),
		      commandListCount(count) {}

		static SubmitInfo Single(CommandList* cmd) { return SubmitInfo(cmd, 1); }

		SubmitInfo& setSwapchainWrite() {
			writesToSwapchain = true;
			return *this;
		}

		SubmitInfo& addDependency(const QueueDependency& dep) {
			waitDependencies.push_back(dep);
			return *this;
		}
	};

	// Queue Capabilities
	struct QueueInfo {
		QueueType type;
		uint32_t  familyIndex;
		bool      supportsPresent;
		bool      supportsTimestamps;
		uint32_t  minImageTransferGranularity[3];

		QueueInfo()
		    : type(QueueType::GRAPHICS),
		      familyIndex(0),
		      supportsPresent(false),
		      supportsTimestamps(false),
		      minImageTransferGranularity{ 1, 1, 1 } {}
	};

	// Command Queue Interface
	class RENDERX_EXPORT CommandQueue {
	public:
		virtual ~CommandQueue() = default;

		virtual CommandAllocator* CreateCommandAllocator(const char* debugName = nullptr) = 0;
		virtual void              DestroyCommandAllocator(CommandAllocator* allocator)    = 0;

		virtual Timeline Submit(CommandList* commandList)     = 0;
		virtual Timeline Submit(const SubmitInfo& submitInfo) = 0;

		virtual bool     Wait(Timeline value, uint64_t timeout = UINT64_MAX) = 0;
		virtual void     WaitIdle()                                          = 0;
		virtual bool     Poll(Timeline value)                                = 0;
		virtual Timeline Completed()                                         = 0;
		virtual Timeline Submitted() const                                   = 0;

		virtual float TimestampFrequency() const = 0;
	};

	struct SwapchainDesc {
		uint32_t width;
		uint32_t height;
		uint32_t preferredImageCount = 3;
		Format   preferredFromat     = Format::BGRA8_SRGB;
		uint32_t maxFramesInFlight   = 3;

		SwapchainDesc(uint32_t w = 0, uint32_t h = 0)
		    : width(w),
		      height(h) {}

		SwapchainDesc(const IVec2& size)
		    : width(size.x),
		      height(size.y) {}

		static SwapchainDesc Default(uint32_t width, uint32_t height) { return SwapchainDesc(width, height); }

		SwapchainDesc& setImageCount(uint32_t count) {
			preferredImageCount = count;
			return *this;
		}

		SwapchainDesc& setFormat(Format fmt) {
			preferredFromat = fmt;
			return *this;
		}

		SwapchainDesc& setMaxFramesInFlight(uint32_t count) {
			maxFramesInFlight = count;
			return *this;
		}
	};

	class RENDERX_EXPORT Swapchain {
	public:
		virtual uint32_t          AcquireNextImage()                      = 0;
		virtual void              Present(uint32_t imageIndex)            = 0;
		virtual void              Resize(uint32_t width, uint32_t height) = 0;
		virtual TextureViewHandle GetImageView(uint32_t imageindex) const = 0;
		virtual Format            GetFormat() const                       = 0;
		virtual uint32_t          GetWidth() const                        = 0;
		virtual uint32_t          GetHeight() const                       = 0;
		virtual uint32_t          GetImageCount() const                   = 0;
	};

} // namespace Rx