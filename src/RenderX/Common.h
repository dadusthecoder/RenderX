#pragma once
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>
#include <vector>
#include "Flags.h"

#if defined(_MSC_VER)
#define RENDERX_DEBUGBREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define RENDERX_DEBUGBREAK() __builtin_trap()
#else
#define RENDERX_DEBUGBREAK() std::abort()
#endif

// === Assert Macros ===
#ifdef RENDERX_DEBUG
#define RENDERX_ASSERT(expr)                                    \
	{                                                           \
		if (!(expr)) {                                          \
			spdlog::error("Assertion Failed! Expr: {}", #expr); \
			RENDERX_DEBUGBREAK();                               \
		}                                                       \
	}

#define RENDERX_ASSERT_MSG(expr, msg, ...)                   \
	{                                                        \
		if (!(expr)) {                                       \
			spdlog::error("Assertion Failed: {} | Expr: {}", \
				fmt::format(msg, ##__VA_ARGS__), #expr);     \
			RENDERX_DEBUGBREAK();                            \
		}                                                    \
	}
#else
#define RENDERX_ASSERT(expr) (void)0
#define RENDERX_ASSERT_MSG(expr, msg, ...)
#endif

// PLATFORM DETECTION
#if defined(_WIN32) || defined(_WIN64)
#define RENDERX_PLATFORM_WINDOWS
#elif defined(__APPLE__) || defined(__MACH__)
#define RENDERX_PLATFORM_MACOS
#elif defined(__linux__)
#define RENDERX_PLATFORM_LINUX
#else
#define RENDERX_PLATFORM_UNKNOWN
#endif

// EXPORT / IMPORT MACROS
#if defined(RENDERX_STATIC)
#define RENDERX_EXPORT
#else
#if defined(RENDERX_PLATFORM_WINDOWS)
#if defined(RENDERX_BUILD_DLL)
#define RENDERX_EXPORT __declspec(dllexport)
#else
#define RENDERX_EXPORT __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define RENDERX_EXPORT __attribute__((visibility("default")))
#else
#define RENDERX_EXPORT
#pragma warning Unknown dynamic link import / export semantics.
#endif
#endif


#define RENDERX_DEFINE_HANDLE(Name) \
	namespace HandleType {          \
		struct Name {};             \
	}                               \
	using Name##Handle = Handle<HandleType::Name>;


#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h> // allows logging custom structs via operator<<



#pragma once
#include <memory>
#include <string>

namespace spdlog {
	class logger;
}

namespace Rx {

	class Log {
	public:
		RENDERX_EXPORT static void Init();
		RENDERX_EXPORT static void Shutdown();

		RENDERX_EXPORT static std::shared_ptr<spdlog::logger>& Core();
		RENDERX_EXPORT static std::shared_ptr<spdlog::logger>& Client();

		// Same-line console status (NO newline)
		RENDERX_EXPORT static void Status(const std::string& msg);
	private:
		RENDERX_EXPORT static std::shared_ptr<spdlog::logger> s_CoreLogger;
		RENDERX_EXPORT static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}


#ifdef RENDERX_DEBUG
#define RX_CORE_TRACE(...) ::Rx::Log::Core()->trace(__VA_ARGS__)
#define RX_CORE_INFO(...) ::Rx::Log::Core()->info(__VA_ARGS__)
#define RX_CORE_WARN(...) ::Rx::Log::Core()->warn(__VA_ARGS__)
#define RX_CORE_ERROR(...) ::Rx::Log::Core()->error(__VA_ARGS__)
#define RX_CORE_CRITICAL(...) ::Rx::Log::Core()->critical(__VA_ARGS__)
#define RX_CLIENT_TRACE(...) ::Rx::Log::Client()->trace(__VA_ARGS__)
#define RX_CLIENT_INFO(...) ::Rx::Log::Client()->info(__VA_ARGS__)
#define RX_CLIENT_WARN(...) ::Rx::Log::Client()->warn(__VA_ARGS__)
#define RX_CLIENT_ERROR(...) ::Rx::Log::Client()->error(__VA_ARGS__)
#define RX_CLIENT_CRITICAL(...) ::Rx::Log::Client()->critical(__VA_ARGS__)
#define RX_STATUS(msg) ::Rx::Log::Status(msg)
#else
#define RX_CORE_TRACE(...)
#define RX_CORE_INFO(...)
#define RX_CORE_WARN(...)
#define RX_CORE_ERROR(...)
#define RX_CORE_CRITICAL(...)
#define RX_STATUS(msg)
#endif

// for backward compatibility
#ifdef RENDERX_DEBUG
#define LOG_INIT() ::Rx::Log::Init()
#define RENDERX_TRACE(...) ::Rx::Log::Core()->trace(__VA_ARGS__)
#define RENDERX_INFO(...) ::Rx::Log::Core()->info(__VA_ARGS__)
#define RENDERX_WARN(...) ::Rx::Log::Core()->warn(__VA_ARGS__)
#define RENDERX_ERROR(...) ::Rx::Log::Core()->error(__VA_ARGS__)
#define RENDERX_CRITICAL(...) ::Rx::Log::Core()->critical(__VA_ARGS__)
#define LOG_SHUTDOWN() ::Rx::Log::Shutdown();
#else
#define LOG_INIT()
#define RENDERX_TRACE(...)
#define RENDERX_INFO(...)
#define RENDERX_WARN(...)
#define RENDERX_ERROR(...)
#define RENDERX_CRITICAL(...)
#define LOG_SHUTDOWN()
#endif
#define CLIENT_TRACE(...) ::Rx::Log::Client()->trace(__VA_ARGS__)
#define CLIENT_INFO(...) ::Rx::Log::Client()->info(__VA_ARGS__)
#define CLIENT_WARN(...) ::Rx::Log::Client()->warn(__VA_ARGS__)
#define CLIENT_ERROR(...) ::Rx::Log::Client()->error(__VA_ARGS__)
#define CLIENT_CRITICAL(...) ::Rx::Log::Client()->critical(__VA_ARGS__)

namespace Rx {

// RenderX API X-Macro
#define RENDERX_FUNC(X)                                                   \
                                                                          \
	/* RenderX Lifetime */                                                \
	X(void, BackendInit,                                                  \
		(const Window& window),                                           \
		(window))                                                         \
                                                                          \
	X(void, BackendShutdown,                                              \
		(),                                                               \
		())                                                               \
                                                                          \
	/* Pipeline Layout */                                                 \
	X(PipelineLayoutHandle, CreatePipelineLayout,                         \
		(const ResourceGroupLayoutHandle* layouts, uint32_t layoutCount), \
		(layouts, layoutCount))                                           \
                                                                          \
	/* Pipeline Graphics */                                               \
	X(PipelineHandle, CreateGraphicsPipeline,                             \
		(PipelineDesc & desc),                                            \
		(desc))                                                           \
                                                                          \
	X(ShaderHandle, CreateShader,                                         \
		(const ShaderDesc& desc),                                         \
		(desc))                                                           \
                                                                          \
	/* Resource Creation */                                               \
	/* Buffers */                                                         \
	X(BufferHandle, CreateBuffer,                                         \
		(const BufferDesc& desc),                                         \
		(desc))                                                           \
                                                                          \
	/* Buffer Views */                                                    \
	X(BufferViewHandle, CreateBufferView,                                 \
		(const BufferViewDesc& desc),                                     \
		(desc))                                                           \
                                                                          \
	X(void, DestroyBufferView,                                            \
		(BufferViewHandle & handle),                                      \
		(handle))                                                         \
                                                                          \
	/* Render Pass */                                                     \
	X(RenderPassHandle, CreateRenderPass,                                 \
		(const RenderPassDesc& desc),                                     \
		(desc))                                                           \
                                                                          \
	X(void, DestroyRenderPass,                                            \
		(RenderPassHandle & pass),                                        \
		(pass))                                                           \
                                                                          \
	/* Framebuffer */                                                     \
	X(FramebufferHandle, CreateFramebuffer,                               \
		(const FramebufferDesc& desc),                                    \
		(desc))                                                           \
                                                                          \
	X(void, DestroyFramebuffer,                                           \
		(FramebufferHandle & framebuffer),                                \
		(framebuffer))                                                    \
                                                                          \
	/* Resource Groups */                                                 \
	X(ResourceGroupHandle, CreateResourceGroup,                           \
		(const ResourceGroupDesc& desc),                                  \
		(desc))                                                           \
                                                                          \
	X(void, DestroyResourceGroup,                                         \
		(ResourceGroupHandle & handle),                                   \
		(handle))                                                         \
                                                                          \
	X(ResourceGroupLayoutHandle, CreateResourceGroupLayout,               \
		(const ResourceGroupLayout& desc),                                \
		(desc))                                                           \
                                                                          \
	X(CommandQueue*, GetGpuQueue,                                         \
		(QueueType type),                                                 \
		(type))


	// Base Handle Template
	template <typename Tag>
	struct Handle {
	public:
		using ValueType = uint64_t;
		static constexpr ValueType INVALID = 0;

		Handle(ValueType key) : id(key) {}
		Handle() = default;

		uint32_t generation() const {
			return static_cast<uint32_t>(id >> 32);
		}
		uint32_t index() const {
			return static_cast<uint32_t>(id & 0xFFFFFFFFu);
		}
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
	using Vec2 = glm::vec2;
	using Vec3 = glm::vec3;
	using Vec4 = glm::vec4;
	using Mat3 = glm::mat3;
	using Mat4 = glm::mat4;
	using IVec2 = glm::ivec2;
	using IVec3 = glm::ivec3;
	using IVec4 = glm::ivec4;
	using UVec2 = glm::uvec2;
	using UVec3 = glm::uvec3;
	using UVec4 = glm::uvec4;
	using Quat = glm::quat;

	enum class GraphicsAPI {
		NONE,
		OPENGL,
		VULKAN
	};

	enum class Platform : uint8_t {
		WINDOWS,
		LINUX,
		MACOS
	};

	struct Window {
		Platform platform;
		GraphicsAPI api;
		void* displayHandle;
		void* nativeHandle;
		uint32_t width;
		uint32_t height;
		const char** instanceExtensions;
		uint32_t extensionCount;
		uint32_t maxFramesInFlight = 3;
	};

	enum class TextureType {
		TEXTURE_2D,
		TEXTURE_3D,
		TEXTURE_CUBE,
		TEXTURE_2D_ARRAY
	};

	enum class LoadOp {
		LOAD,
		CLEAR,
		DONT_CARE
	};

	enum class StoreOp {
		STORE,
		DONT_CARE
	};

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

	enum class TextureWrap {
		REPEAT,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER
	};

	enum class Topology {
		POINTS,
		LINES,
		LINE_STRIP,
		TRIANGLES,
		TRIANGLE_STRIP,
		TRIANGLE_FAN
	};

	enum class CompareFunc {
		NEVER,
		LESS,
		EQUAL,
		LESS_EQUAL,
		GREATER,
		NOT_EQUAL,
		GREATER_EQUAL,
		ALWAYS
	};

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

	enum class BlendOp {
		ADD,
		SUBTRACT,
		REVERSE_SUBTRACT,
		MIN,
		MAX
	};

	enum class CullMode {
		NONE,
		FRONT,
		BACK,
		FRONT_AND_BACK
	};

	enum class FillMode {
		SOLID,
		WIREFRAME,
		POINT
	};

	enum class MemoryType : uint8_t {
		GPU_ONLY = 1 << 0,	 // Device-local, no CPU access
		CPU_TO_GPU = 1 << 1, // Host-visible, optimized for CPU writes
		GPU_TO_CPU = 1 << 2, // Host-visible, optimized for CPU reads (cached)
		CPU_ONLY = 1 << 3,	 // Host memory, rarely accessed by GPU
		AUTO = 1 << 4		 // Prefer device-local but allow fallback
	};

	enum class ResourceType : uint8_t {
		CONSTANT_BUFFER,		  // Uniform / Constant buffer
		STORAGE_BUFFER,			  // Read-only SSBO
		RW_STORAGE_BUFFER,		  // Read-write
		TEXTURE_SRV,			  // Read-only texture (sampled image)
		TEXTURE_UAV,			  // Read-write texture (storage image)
		SAMPLER,				  // Sampler state object
		COMBINED_TEXTURE_SAMPLER, // Combined (GL-style / convenience)
		ACCELERATION_STRUCTURE	  // Ray tracing TLAS / BLAS
	};


	enum class ShaderStage : uint8_t {
		NONE = 0,
		VERTEX = 1 << 0,
		FRAGMENT = 1 << 1,
		GEOMETRY = 1 << 2,
		TESS_CONTROL = 1 << 3,
		TESS_EVALUATION = 1 << 4,
		COMPUTE = 1 << 5,
		ALL = VERTEX | FRAGMENT | COMPUTE
	};
	ENABLE_BITMASK_OPERATORS(ShaderStage)


	enum class ResourceGroupFlags : uint8_t {
		NONE = 0,				  // No special flags
		STATIC = 1 << 0,		  // Long-lived (per-scene / per-material)
		DYNAMIC = 1 << 1,		  // Frequently updated (per-frame / per-draw)
		DYNAMIC_UNIFORM = 1 << 2, // Dynamic uniform buffers (Vulkan dynamic offsets)
		BINDLESS = 1 << 3,		  // Bindless descriptors
		BUFFER = 1 << 4			  // Descriptor-buffer based APIs (e.g. VK_EXT_descriptor_buffer)
	};
	ENABLE_BITMASK_OPERATORS(ResourceGroupFlags)

	enum class BufferFlags : uint16_t {
		VERTEX = 1 << 0,
		INDEX = 1 << 1,
		UNIFORM = 1 << 2,
		STORAGE = 1 << 3,
		INDIRECT = 1 << 4,
		TRANSFER_SRC = 1 << 5,
		TRANSFER_DST = 1 << 6,
		STATIC = 1 << 8,
		DYNAMIC = 1 << 9,
		STREAMING = 1 << 10
	};
	ENABLE_BITMASK_OPERATORS(BufferFlags)


	struct Viewport {
		int x, y;
		int width, height;
		float minDepth, maxDepth;

		Viewport(int x = 0, int y = 0, int w = 0, int h = 0, float minD = 0.0f,
			float maxD = 1.0f) : x(x),
								 y(y),
								 width(w),
								 height(h),
								 minDepth(minD),
								 maxDepth(maxD) {}

		Viewport(const IVec2& pos, const IVec2& size, float minD = 0.0f,
			float maxD = 1.0f) : x(pos.x),
								 y(pos.y),
								 width(size.x),
								 height(size.y),
								 minDepth(minD),
								 maxDepth(maxD) {}
	};

	struct Scissor {
		int x, y;
		int width, height;

		Scissor(int x = 0, int y = 0, int w = 0, int h = 0)
			: x(x), y(y), width(w), height(h) {
		}
		Scissor(const IVec2& pos, const IVec2& size)
			: x(pos.x), y(pos.y), width(size.x), height(size.y) {
		}
	};

	struct ClearColor {
		Vec4 color;

		ClearColor(float r = 0.30f, float g = 0.50f, float b = 0.0f, float a = 1.0f)
			: color(r, g, b, a) {
		}
		ClearColor(const Vec4& c) : color(c) {}
		ClearColor(const Vec3& c, float a = 1.0f) : color(c, a) {}
	};

	// Vertex input description
	struct VertexAttribute {
		uint32_t location;
		uint32_t binding;
		Format format;
		uint32_t offset;

		VertexAttribute(uint32_t loc, uint32_t binding, Format format, uint32_t off)
			: location(loc), binding(binding), format(format), offset(off) {
		}
	};

	struct VertexBinding {
		uint32_t binding;
		uint32_t stride;
		bool instanceData; // true for per-instance, false for per-vertex

		VertexBinding(uint32_t bind, uint32_t str, bool inst = false)
			: binding(bind), stride(str), instanceData(inst) {
		}
	};

	struct VertexInputState {
		std::vector<VertexAttribute> attributes;
		std::vector<VertexBinding> vertexBindings;
	};


	inline bool IsValidBufferFlags(BufferFlags flags) {
		// Can't be both static and dynamic
		bool hasStatic = Has(flags, BufferFlags::STATIC);
		bool hasDynamic = Has(flags, BufferFlags::DYNAMIC);

		if (hasStatic && hasDynamic) {
			return false; // Invalid combination
		}

		// Must have at least one usage flag (not just Static/Dynamic)
		BufferFlags usageMask = BufferFlags::VERTEX | BufferFlags::INDEX |
								BufferFlags::UNIFORM | BufferFlags::STORAGE |
								BufferFlags::INDIRECT | BufferFlags::TRANSFER_SRC |
								BufferFlags::TRANSFER_DST;

		if (Has(flags, usageMask)) {
			return false; // Must have at least one usage
		}

		return true;
	}

	struct BufferDesc {
		BufferFlags flags;
		MemoryType memoryType;
		size_t size;
		uint32_t bindingCount;
		const void* initialData;
	};

	struct BufferViewDesc {
		BufferHandle buffer;
		uint32_t offset = 0;
		uint32_t range = 0; // 0 = whole buffer
	};

	struct SamplerDesc {
		Filter minFilter;
		Filter magFilter;
		TextureWrap wrapU;
		TextureWrap wrapV;
		TextureWrap wrapW;
		float maxAnisotropy;
		bool enableCompare;
		CompareFunc compareFunc;
		Vec4 borderColor;

		SamplerDesc()
			: minFilter(Filter::LINEAR), magFilter(Filter::LINEAR),
			  wrapU(TextureWrap::REPEAT), wrapV(TextureWrap::REPEAT),
			  wrapW(TextureWrap::REPEAT), maxAnisotropy(1.0f), enableCompare(false),
			  compareFunc(CompareFunc::NEVER), borderColor(0.0f, 0.0f, 0.0f, 1.0f) {
		}
	};

	struct TextureDesc {
		TextureType type;
		int width, height, depth;
		int mipLevels;
		Format format;
		const void* initialData;
		size_t dataSize;
		bool generateMips;

		TextureDesc(int w, int h, Format fmt,
			TextureType t = TextureType::TEXTURE_2D)
			: type(t), width(w), height(h), depth(1), mipLevels(1), format(fmt),
			  initialData(nullptr), dataSize(0), generateMips(false) {
		}

		TextureDesc(const IVec2& size, Format fmt,
			TextureType t = TextureType::TEXTURE_2D)
			: type(t), width(size.x), height(size.y), depth(1), mipLevels(1),
			  format(fmt), initialData(nullptr), dataSize(0), generateMips(false) {
		}
	};

	struct ShaderDesc {
		ShaderStage type;
		std::string source;			   // GLSL source or HLSL
		std::vector<uint8_t> bytecode; // SPIR-V or compiled bytecode
		std::string entryPoint;		   // Entry function name (for HLSL/SPIR-V)

		ShaderDesc(ShaderStage t, const std::string& src)
			: type(t), source(src), entryPoint("main") {
		}

		ShaderDesc(ShaderStage t, const std::vector<uint8_t>& code,
			const std::string entry = "main")
			: type(t), bytecode(code), entryPoint(entry) {
		}
	};

	// Render state
	struct RasterizerState {
		FillMode fillMode;
		CullMode cullMode;
		bool frontCounterClockwise;
		float depthBias;
		float depthBiasClamp;
		float slopeScaledDepthBias;
		bool depthClipEnable;
		bool scissorEnable;
		bool multisampleEnable;

		RasterizerState()
			: fillMode(FillMode::SOLID), cullMode(CullMode::NONE),
			  frontCounterClockwise(false), depthBias(0.0f), depthBiasClamp(0.0f),
			  slopeScaledDepthBias(0.0f), depthClipEnable(true), scissorEnable(false),
			  multisampleEnable(false) {
		}
	};

	struct DepthStencilState {
		bool depthEnable;
		bool depthWriteEnable;
		CompareFunc depthFunc;
		bool stencilEnable;
		uint8_t stencilReadMask;
		uint8_t stencilWriteMask;
		// Stencil operations would go here...

		DepthStencilState()
			: depthEnable(true), depthWriteEnable(true), depthFunc(CompareFunc::LESS),
			  stencilEnable(false), stencilReadMask(0xFF), stencilWriteMask(0xFF) {
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
	};

	// Describes a single binding slot in a ResourceGroupLayout
	struct ResourceGroupLayoutItem {
		// Binding index (e.g., layout(binding = 0))
		uint32_t binding;
		// What kind of resource
		ResourceType type;
		// Which shader stages can access this
		ShaderStage stages;
		// Array size (1 for non-arrays, >1 for arrays) For Bindless indexing
		uint32_t count;

		ResourceGroupLayoutItem()
			: binding(0), type(ResourceType::CONSTANT_BUFFER),
			  stages(ShaderStage::ALL), count(1) {}

		ResourceGroupLayoutItem(uint32_t bind, ResourceType t, ShaderStage s = ShaderStage::ALL, uint32_t cnt = 1)
			: binding(bind), type(t), stages(s), count(cnt) {}

		// Convenience factory methods
		static ResourceGroupLayoutItem ConstantBuffer(uint32_t binding, ShaderStage stages = ShaderStage::ALL) {
			return ResourceGroupLayoutItem(binding, ResourceType::CONSTANT_BUFFER, stages, 1);
		}

		static ResourceGroupLayoutItem StorageBuffer(uint32_t binding, ShaderStage stages = ShaderStage::ALL, bool writable = false) {
			return ResourceGroupLayoutItem(binding,
				writable ? ResourceType::RW_STORAGE_BUFFER : ResourceType::STORAGE_BUFFER,
				stages, 1);
		}

		static ResourceGroupLayoutItem Texture_SRV(uint32_t binding, ShaderStage stages = ShaderStage::ALL, uint32_t count = 1) {
			return ResourceGroupLayoutItem(binding, ResourceType::TEXTURE_SRV, stages, count);
		}

		static ResourceGroupLayoutItem Texture_UAV(uint32_t binding, ShaderStage stages = ShaderStage::ALL, uint32_t count = 1) {
			return ResourceGroupLayoutItem(binding, ResourceType::TEXTURE_UAV, stages, count);
		}

		static ResourceGroupLayoutItem Sampler(uint32_t binding, ShaderStage stages = ShaderStage::ALL) {
			return ResourceGroupLayoutItem(binding, ResourceType::SAMPLER, stages, 1);
		}

		static ResourceGroupLayoutItem CombinedTextureSampler(uint32_t binding, ShaderStage stages = ShaderStage::ALL) {
			return ResourceGroupLayoutItem(binding, ResourceType::COMBINED_TEXTURE_SAMPLER, stages, 1);
		}
	};

	/// Describes the layout/structure of a pipeline inputs
	struct ResourceGroupLayout {
		std::vector<ResourceGroupLayoutItem> resourcebindings;
		ResourceGroupFlags flags = ResourceGroupFlags::STATIC;
		const char* debugName = nullptr;
		ResourceGroupLayout() = default;

		ResourceGroupLayout(const std::vector<ResourceGroupLayoutItem>& items)
			: resourcebindings(items) {}
	};

	/// Describes a single resource binding in a descriptor set instance
	struct ResourceGroupItem {
		uint32_t binding;
		ResourceType type;
		uint32_t arrayIndex = 0; // For Bindless Resource to arrays of descriptors

		struct CombinedHandles {
			TextureViewHandle texture;
			SamplerHandle sampler;
		};

		union {
			BufferViewHandle bufferView;
			TextureViewHandle textureView;
			SamplerHandle sampler;
			uint64_t rawHandle;
			CombinedHandles combinedHandles;
		};



		ResourceGroupItem()
			: binding(0), type(ResourceType::CONSTANT_BUFFER), rawHandle(0) {}

		// Convenience factory methods
		static ResourceGroupItem ConstantBuffer(uint32_t binding, BufferViewHandle buf) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type = ResourceType::CONSTANT_BUFFER;
			item.bufferView = buf;
			return item;
		}

		static ResourceGroupItem StorageBuffer(uint32_t binding, BufferViewHandle buf, bool writable = false) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type = writable ? ResourceType::RW_STORAGE_BUFFER : ResourceType::STORAGE_BUFFER;
			item.bufferView = buf;
			return item;
		}

		static ResourceGroupItem Texture_SRV(uint32_t binding, TextureViewHandle tex, uint32_t arrayIndex = 0) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type = ResourceType::TEXTURE_SRV;
			item.textureView = tex;
			item.arrayIndex = arrayIndex;
			return item;
		}

		static ResourceGroupItem Texture_UAV(uint32_t binding, TextureViewHandle tex, uint32_t mipLevel = 0) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type = ResourceType::TEXTURE_UAV;
			item.textureView = tex;
			return item;
		}

		static ResourceGroupItem Sampler(uint32_t binding, SamplerHandle samp) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type = ResourceType::SAMPLER;
			item.sampler = samp;
			return item;
		}

		static ResourceGroupItem CombinedTextureSampler(uint32_t binding, TextureViewHandle tex, SamplerHandle samp) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type = ResourceType::COMBINED_TEXTURE_SAMPLER;
			// Store both handles into the combined struct so the union doesn't
			// overwrite one with the other.
			item.combinedHandles.texture = tex;
			item.combinedHandles.sampler = samp;
			return item;
		}
	};

	/// Describes a ResourceGroup instance
	struct ResourceGroupDesc {
		PipelineLayoutHandle pipelinelayout;	  // Must match layout used in pipeline
		ResourceGroupLayoutHandle layout;		  //
		std::vector<ResourceGroupItem> Resources; // Actual resources to bind
		uint64_t dynamicOffset = 0;
		const char* debugName = nullptr;
		ResourceGroupDesc() = default;
		ResourceGroupDesc(PipelineLayoutHandle layoutHandle, const std::vector<ResourceGroupItem>& items)
			: pipelinelayout(layoutHandle), Resources(items) {}
	};


	// Pipeline description
	struct PipelineDesc {
		std::vector<ShaderHandle> shaders;
		VertexInputState vertexInputState;
		Topology primitiveType;
		RasterizerState rasterizer;
		DepthStencilState depthStencil;
		BlendState blend;
		RenderPassHandle renderPass;
		PipelineLayoutHandle layout;

		PipelineDesc()
			: primitiveType(Topology::TRIANGLES), renderPass(0) {}
	};

	struct ClearValue {
		ClearColor color;
		float depth = 1.0f;
		uint8_t stencil = 0;
	};


	// Render pass description
	struct AttachmentDesc {
		Format format;
		LoadOp loadOp = LoadOp::CLEAR;
		StoreOp storeOp = StoreOp::STORE;
		AttachmentDesc(Format format = Format::RGBA8_SRGB) : format(format) {}
	};

	struct DepthStencilAttachmentDesc {
		Format format = Format::D24_UNORM_S8_UINT;
		LoadOp depthLoadOp = LoadOp::CLEAR;
		StoreOp depthStoreOp = StoreOp::STORE;
		LoadOp stencilLoadOp = LoadOp::DONT_CARE;
		StoreOp stencilStoreOp = StoreOp::DONT_CARE;
		DepthStencilAttachmentDesc(Format format) : format(format) {}
	};

	struct RenderPassDesc {
		std::vector<AttachmentDesc> colorAttachments;
		DepthStencilAttachmentDesc depthStencilAttachment;
		bool hasDepthStencil;

		RenderPassDesc()
			: depthStencilAttachment(Format::D24_UNORM_S8_UINT),
			  hasDepthStencil(false) {
		}
	};

	// Framebuffer description
	struct FramebufferDesc {
		RenderPassHandle renderPass;

		std::vector<TextureHandle> colorAttachments;
		TextureHandle depthStencilAttachment;

		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t layers = 1;

		FramebufferDesc(int w, int h)
			: renderPass(0), depthStencilAttachment(0),
			  width(w), height(h) {
		}

		FramebufferDesc(const IVec2& size)
			: renderPass(0), depthStencilAttachment(0),
			  width(size.x), height(size.y) {
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
			: drawCalls(0), triangles(0), vertices(0), bufferBinds(0), textureBinds(0),
			  shaderBinds(0) {
		}

		void Reset() {
			drawCalls = triangles = vertices = bufferBinds = textureBinds =
				shaderBinds = 0;
		}
	};

	enum class CommandListState : uint8_t {
		INITIAL,	// Just created, ready to begin recording
		RECORDING,	// Currently recording commands
		EXECUTABLE, // Recording finished, ready to submit
		SUBMITTED,	// Submitted to queue, waiting for execution
		COMPLETED,	// Execution finished
		INVALID		// Error state or destroyed
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

		Timeline(uint64_t v = 0) : value(v) {}

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
		virtual void open() = 0;
		virtual void close() = 0;

		virtual void setPipeline(const PipelineHandle& pipeline) = 0;
		virtual void setVertexBuffer(const BufferHandle& buffer, uint64_t offset = 0) = 0;
		virtual void setIndexBuffer(const BufferHandle& buffer, uint64_t offset = 0) = 0;

		virtual void draw(
			uint32_t vertexCount,
			uint32_t instanceCount = 1,
			uint32_t firstVertex = 0,
			uint32_t firstInstance = 0) = 0;

		virtual void drawIndexed(
			uint32_t indexCount,
			int32_t vertexOffset = 0,
			uint32_t instanceCount = 1,
			uint32_t firstIndex = 0,
			uint32_t firstInstance = 0) = 0;

		virtual void beginRenderPass(
			RenderPassHandle pass,
			const void* clearValues,
			uint32_t clearCount) = 0;

		virtual void endRenderPass() = 0;

		virtual void writeBuffer(
			BufferHandle handle,
			const void* data,
			uint32_t offset,
			uint32_t size) = 0;

		virtual void setResourceGroup(
			const ResourceGroupHandle& handle) = 0;
	};


	// Synchronization dependency between queues
	struct QueueDependency {
		QueueType waitQueue; // Queue that needs to wait
		Timeline waitValue;	 // Timeline value to wait for

		QueueDependency(QueueType queue, Timeline value)
			: waitQueue(queue), waitValue(value) {}
	};


	// This Class Allocates a cpu side Recoding object CommandList
	//(Maps to VkCommandPool in vulkan or ID3D12CommandAllocator in DX12)
	class RENDERX_EXPORT CommandAllocator {
	public:
		// Allocate a new CommandList in INITIAL State
		virtual CommandList* Allocate() = 0;

		// Resets a Command List To the INITIAL state
		virtual void Reset(CommandList* list) = 0;

		// Sets a CommadList to a INVALID state
		virtual void Free(CommandList* list) = 0;

		// Resets All the CommandLsit Allocated From this Allocator To the Initial State;
		virtual void Reset() = 0;
	};

	// Command List Submission
	// Describes how to submit a command list
	struct SubmitInfo {
		// Command list to submit
		CommandList* commandList = nullptr;
		uint32_t commandListCount;
		// Dependencies - wait for these timeline values before executing
		std::vector<QueueDependency> waitDependencies;
		SubmitInfo() = default;
		SubmitInfo(CommandList* cmd, uint32_t count)
			: commandList(cmd), commandListCount(count) {}
	};

	// Queue Capabilities
	// Information about a queue's capabilities
	struct QueueInfo {
		QueueType type;
		uint32_t familyIndex;					 // Internal queue family
		bool supportsPresent;					 // Can present to swapchain
		bool supportsTimestamps;				 // Can query GPU timestamps
		uint32_t minImageTransferGranularity[3]; // Optimal transfer block size

		QueueInfo()
			: type(QueueType::GRAPHICS), familyIndex(0), supportsPresent(false), supportsTimestamps(false), minImageTransferGranularity{ 1, 1, 1 } {}
	};

	// Command Queue Interface
	class RENDERX_EXPORT CommandQueue {
	public:
		virtual ~CommandQueue() = default;

		/// Queue Information
		/// Get queue type
		/*	virtual QueueType GetType() const = 0;*/

		/*	/// Get detailed queue info
			virtual QueueInfo GetInfo() const = 0;*/

		/// CommandAllcoator Management
		/// Create a CommandAllcoator for this queue
		/// @param debugName Optional name for debugging
		virtual CommandAllocator* CreateCommandAllocator(const char* debugName = nullptr) = 0;

		/// Destroy a cCommandAllcoator
		virtual void DestroyCommandAllocator(CommandAllocator* allocator) = 0;

		/// Submission
		/// Submit a single command list
		/// @return Timeline value that will be signaled when complete
		virtual Timeline Submit(CommandList* commandList) = 0;

		/// Submit with dependencies and custom signal value
		virtual Timeline Submit(const SubmitInfo& submitInfo) = 0;

		/// Synchronization

		/// Wait for a specific timeline value on CPU!
		/// @param timeout Timeout in nanoseconds (UINT64_MAX = infinite)
		/// @return true if wait succeeded, false if timeout
		virtual bool Wait(Timeline value, uint64_t timeout = UINT64_MAX) = 0;

		/// Wait for the queue to be idle on CPU! (all submitted work complete)
		virtual void WaitIdle() = 0;

		/// Check if a timeline value has been reached (non-blocking)
		virtual bool Poll(Timeline value) = 0;

		/// Get the current timeline value (last completed submission)
		virtual Timeline Completed() = 0;

		/// Get the next timeline value that will be signaled
		virtual Timeline Submitted() const = 0;

		/// Features
		/// Get GPU timestamp frequency (ticks per second)
		virtual float TimestampFrequency() const = 0;
	};

} // namespace  RenderX
