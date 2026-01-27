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

// ==================== Assert Macros ====================
#ifdef _DEBUG
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

namespace RenderX {

	/// Lightweight strongly-typed wrapper around a 32-bit resource id.
	/// All GPU-resident objects in RenderX are referenced via Handle-based aliases.
	struct RENDERX_API Handle {
		uint32_t id = 0;

		Handle() = default;
		Handle(uint32_t handleId) : id(handleId) {}

		bool IsValid() const { return id != 0; }

		bool operator==(const Handle& other) const { return id == other.id; }
		bool operator!=(const Handle& other) const { return id != other.id; }
		bool operator<(const Handle& other) const { return id < other.id; }
	};

	using BufferHandle = Handle;
	using TextureHandle = Handle;
	using SamplerHandle = Handle;
	using ShaderHandle = Handle;
	using PipelineHandle = Handle;
	using FramebufferHandle = Handle;
	using RenderPassHandle = Handle;
	using SurfaceHandle = Handle;
	using SwapchainHandle = Handle;
	using SRGLayoutHandle = Handle;
	using BufferViewHandle = Handle;
	using TextureViewHandle = Handle;


	constexpr uint32_t INVALID_HANDLE = 0;

	// GLM type aliases for consistency
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



	// Enums and structs for various RenderX configurations and description
	enum class GraphicsAPI {
		None,
		OpenGL,
		Vulkan
	};

	enum class Platform : uint8_t {
		Windows,
		Linux,
		MacOS
	};

	struct Window {
		Platform platform;
		GraphicsAPI api;

		void* displayHandle;
		void* nativeHandle;
		uint32_t width, height;

		const char** instanceExtensions;
		uint32_t extensionCount;
	};


	//
	enum class BufferType : uint8_t {
		Vertex,
		Index,
		Uniform,
		Storage,
		Indirect
	};

	enum class BufferUsage : uint8_t {
		Static,	 // CPU upload once, GPU reads
		Dynamic, // CPU updates often
		Stream	 // CPU updates every frame
	};


	enum class TextureType {
		Texture2D,
		Texture3D,
		TextureCube,
		Texture2DArray
	};

	enum class LoadOp {
		Load,
		Clear,
		DontCare
	};

	enum class StoreOp {
		Store,
		DontCare
	};

	enum class DataFormat {
		R8,
		RG8,
		RGB8,
		RGBA8,
		R16F,
		RG16F,
		RGB16F,
		RGBA16F,
		R32F,
		RG32F,
		RGB32F,
		RGBA32F,
	};

	enum class TextureFormat {
		RGBA8,
		RGBA8_SRGB,
		BGRA8,
		BGRA8_SRGB,
		RGBA16F,
		RGBA32F,
		R8,
		R16F,
		R32F,
		Depth24Stencil8,
		Depth32F,
		BC1,
		BC1_SRGB,
		BC3,
		BC3_SRGB
	};


	enum class TextureFilter {
		Nearest,
		Linear,
		NearestMipmapNearest,
		NearestMipmapLinear,
		LinearMipmapNearest,
		LinearMipmapLinear
	};

	enum class TextureWrap { Repeat,
		MirroredRepeat,
		ClampToEdge,
		ClampToBorder };

	enum class PrimitiveType {
		Points,
		Lines,
		LineStrip,
		Triangles,
		TriangleStrip,
		TriangleFan
	};

	enum class CompareFunc {
		Never,
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always
	};

	enum class BlendFunc {
		Zero,
		One,
		SrcColor,
		OneMinusSrcColor,
		DstColor,
		OneMinusDstColor,
		SrcAlpha,
		OneMinusSrcAlpha,
		DstAlpha,
		OneMinusDstAlpha,
		ConstantColor,
		OneMinusConstantColor
	};

	enum class BlendOp { Add,
		Subtract,
		ReverseSubtract,
		Min,
		Max };

	enum class CullMode { None,
		Front,
		Back,
		FrontAndBack };

	enum class FillMode { Solid,
		Wireframe,
		Point };



	// Pipeline Type
	enum class PipelineType {
		Graphics,
		Compute
	};

	enum class ShaderStage : uint8_t {
		None = 0,
		Vertex = 1 << 0,
		Fragment = 1 << 1,
		Geometry = 1 << 2,
		TessControl = 1 << 3,
		TessEvaluation = 1 << 4,
		Compute = 1 << 5,
		All = Vertex | Fragment | Compute
	};

	template <>
	struct EnableBitMaskOperators<ShaderStage> {
		static constexpr bool enable = true;
	};

	// ==================== Resource State Tracking ====================
	enum class ResourceState : uint16_t {
		Undefined = 0,
		Common = 1 << 0,
		RenderTarget = 1 << 1,
		DepthWrite = 1 << 2,
		DepthRead = 1 << 3,
		ShaderRead = 1 << 4,
		ShaderWrite = 1 << 5,
		Present = 1 << 6,
		CopySrc = 1 << 7,
		CopyDst = 1 << 8,
		IndirectArgument = 1 << 9
	};

	template <>
	struct EnableBitMaskOperators<ResourceState> {
		static constexpr bool enable = true;
	};

	enum class SRGLifetime : uint8_t {
		Persistent,
		PerFrame,
		PerDraw
	};


	enum class ResourceType : uint8_t {

		// Uniform buffer / Constant buffer
		ConstantBuffer,
		// SSBO / Structured buffer (read-only)
		StorageBuffer,
		// RW Structured buffer
		RWStorageBuffer,
		// Shader resource view (read-only texture)
		Texture_SRV,
		// Unordered access view (read-write texture)
		Texture_UAV,

		Sampler,

		// Combined (OpenGL style)
		CombinedTextureSampler,

		// for future me
		AccelerationStructure // For raytracing

	};

	// Core structures
	struct Viewport {
		int x, y;
		int width, height;
		float minDepth, maxDepth;

		Viewport(int x = 0, int y = 0, int w = 0, int h = 0, float minD = 0.0f,
			float maxD = 1.0f)
			: x(x), y(y), width(w), height(h), minDepth(minD), maxDepth(maxD) {
		}

		Viewport(const IVec2& pos, const IVec2& size, float minD = 0.0f,
			float maxD = 1.0f)
			: x(pos.x), y(pos.y), width(size.x), height(size.y), minDepth(minD),
			  maxDepth(maxD) {
		}
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
		DataFormat datatype;
		uint32_t offset;

		VertexAttribute(uint32_t loc, uint32_t binding, DataFormat datatype, uint32_t off)
			: location(loc), binding(binding), datatype(datatype), offset(off) {
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
		std::vector<VertexBinding> bindings;
	};

	// Resource descriptions for craeation

	struct BufferDesc {
		BufferType type;
		BufferUsage usage;
		size_t size;
		uint32_t bindingCount;
		const void* initialData;
	};

	struct SamplerDesc {
		TextureFilter minFilter;
		TextureFilter magFilter;
		TextureWrap wrapU;
		TextureWrap wrapV;
		TextureWrap wrapW;
		float maxAnisotropy;
		bool enableCompare;
		CompareFunc compareFunc;
		Vec4 borderColor;

		SamplerDesc()
			: minFilter(TextureFilter::Linear), magFilter(TextureFilter::Linear),
			  wrapU(TextureWrap::Repeat), wrapV(TextureWrap::Repeat),
			  wrapW(TextureWrap::Repeat), maxAnisotropy(1.0f), enableCompare(false),
			  compareFunc(CompareFunc::Never), borderColor(0.0f, 0.0f, 0.0f, 1.0f) {
		}
	};

	struct TextureDesc {
		TextureType type;
		int width, height, depth;
		int mipLevels;
		TextureFormat format;
		const void* initialData;
		size_t dataSize;
		bool generateMips;

		TextureDesc(int w, int h, TextureFormat fmt,
			TextureType t = TextureType::Texture2D)
			: type(t), width(w), height(h), depth(1), mipLevels(1), format(fmt),
			  initialData(nullptr), dataSize(0), generateMips(false) {
		}

		TextureDesc(const IVec2& size, TextureFormat fmt,
			TextureType t = TextureType::Texture2D)
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
			: fillMode(FillMode::Solid), cullMode(CullMode::Back),
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
			: depthEnable(true), depthWriteEnable(true), depthFunc(CompareFunc::Less),
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
			  srcColor(BlendFunc::SrcAlpha),
			  dstColor(BlendFunc::OneMinusSrcAlpha),
			  srcAlpha(BlendFunc::One),
			  dstAlpha(BlendFunc::Zero),
			  colorOp(BlendOp::Add),
			  alphaOp(BlendOp::Add),
			  blendFactor(1.0f) {}
	};



	// ==================== Sahder Resource Groups (NEW) ====================
	// here SRG stands for Shader Resource Group
	/// Describes a single binding slot in a SRGLayout
	struct SRGLayoutItem {
		// Binding index (e.g., layout(binding = 0))
		uint32_t binding;
		// What kind of resource
		ResourceType type;
		// Which shader stages can access this
		ShaderStage stages;
		// Array size (1 for non-arrays, >1 for arrays)
		uint32_t count;

		SRGLayoutItem()
			: binding(0), type(ResourceType::ConstantBuffer),
			  stages(ShaderStage::All), count(1) {}

		SRGLayoutItem(uint32_t bind, ResourceType t, ShaderStage s = ShaderStage::All, uint32_t cnt = 1)
			: binding(bind), type(t), stages(s), count(cnt) {}

		// Convenience factory methods
		static SRGLayoutItem ConstantBuffer(uint32_t binding, ShaderStage stages = ShaderStage::All) {
			return SRGLayoutItem(binding, ResourceType::ConstantBuffer, stages, 1);
		}

		static SRGLayoutItem StorageBuffer(uint32_t binding, ShaderStage stages = ShaderStage::All, bool writable = false) {
			return SRGLayoutItem(binding,
				writable ? ResourceType::RWStorageBuffer : ResourceType::StorageBuffer,
				stages, 1);
		}

		static SRGLayoutItem Texture_SRV(uint32_t binding, ShaderStage stages = ShaderStage::All, uint32_t count = 1) {
			return SRGLayoutItem(binding, ResourceType::Texture_SRV, stages, count);
		}

		static SRGLayoutItem Texture_UAV(uint32_t binding, ShaderStage stages = ShaderStage::All, uint32_t count = 1) {
			return SRGLayoutItem(binding, ResourceType::Texture_UAV, stages, count);
		}

		static SRGLayoutItem Sampler(uint32_t binding, ShaderStage stages = ShaderStage::All) {
			return SRGLayoutItem(binding, ResourceType::Sampler, stages, 1);
		}

		static SRGLayoutItem CombinedTextureSampler(uint32_t binding, ShaderStage stages = ShaderStage::All) {
			return SRGLayoutItem(binding, ResourceType::CombinedTextureSampler, stages, 1);
		}
	};

	/// Describes the layout/structure of a descriptor set
	struct SRGLayoutDesc {
		std::vector<SRGLayoutItem> Resources;
		const char* debugName = nullptr;

		SRGLayoutDesc() = default;

		SRGLayoutDesc(uint32_t setIndex, const std::vector<SRGLayoutItem>& items)
			: Resources(items) {}
	};

	/// Describes a single resource binding in a descriptor set instance
	struct SRGItem {
		uint32_t binding;
		ResourceType type;

		// Union for different resource types
		union {
			BufferViewHandle buffer;
			TextureViewHandle texture;
			SamplerHandle sampler;
			uint64_t rawHandle;
		};


		uint32_t arrayIndex = 0; // For Resource to arrays of descriptors

		SRGItem()
			: binding(0), type(ResourceType::ConstantBuffer), rawHandle(0) {}

		// Convenience factory methods
		static SRGItem ConstantBuffer(uint32_t binding, BufferViewHandle buf, uint32_t offset = 0, uint32_t range = 0) {
			SRGItem item;
			item.binding = binding;
			item.type = ResourceType::ConstantBuffer;
			item.buffer = buf;
			return item;
		}

		static SRGItem StorageBuffer(uint32_t binding, BufferViewHandle buf, bool writable = false) {
			SRGItem item;
			item.binding = binding;
			item.type = writable ? ResourceType::RWStorageBuffer : ResourceType::StorageBuffer;
			item.buffer = buf;
			return item;
		}

		static SRGItem Texture_SRV(uint32_t binding, TextureViewHandle tex, uint32_t arrayIndex = 0) {
			SRGItem item;
			item.binding = binding;
			item.type = ResourceType::Texture_SRV;
			item.texture = tex;
			item.arrayIndex = arrayIndex;
			return item;
		}

		static SRGItem Texture_UAV(uint32_t binding, TextureViewHandle tex, uint32_t mipLevel = 0) {
			SRGItem item;
			item.binding = binding;
			item.type = ResourceType::Texture_UAV;
			item.texture = tex;
			return item;
		}

		static SRGItem Sampler(uint32_t binding, SamplerHandle samp) {
			SRGItem item;
			item.binding = binding;
			item.type = ResourceType::Sampler;
			item.sampler = samp;
			return item;
		}

		static SRGItem CombinedTextureSampler(uint32_t binding, TextureHandle tex, SamplerHandle samp) {
			SRGItem item;
			item.binding = binding;
			item.type = ResourceType::CombinedTextureSampler;
			item.texture = tex;
			item.sampler = samp; // Store both - backend will handle appropriately
			return item;
		}
	};

	/// Describes a descriptor set instance
	struct SRGDesc {
		SRGLayoutHandle layout;			// Must match layout used in pipeline
		std::vector<SRGItem> Resources; // Actual resources to bind
		SRGLifetime flags = SRGLifetime::Persistent;
		const char* debugName = nullptr;

		SRGDesc() = default;

		SRGDesc(SRGLayoutHandle layoutHandle, const std::vector<SRGItem>& items)
			: layout(layoutHandle), Resources(items) {}
	};


	// Pipeline description
	struct PipelineDesc {
		PipelineType type;
		std::vector<ShaderHandle> shaders;
		VertexInputState vertexInputState;
		PrimitiveType primitiveType;
		RasterizerState rasterizer;
		DepthStencilState depthStencil;
		BlendState blend;
		RenderPassHandle renderPass;
		SRGLayoutHandle SRGLayout;

		PipelineDesc()
			: primitiveType(PrimitiveType::Triangles), renderPass(INVALID_HANDLE) {
		}
	};

	struct ClearValue {
		ClearColor color;
		float depth = 1.0f;
		uint8_t stencil = 0;
	};


	// Render pass description
	struct AttachmentDesc {
		TextureFormat format;
		LoadOp loadOp = LoadOp::Clear;
		StoreOp storeOp = StoreOp::Store;

		ResourceState initialState = ResourceState::Undefined;
		ResourceState finalState = ResourceState::RenderTarget;

		AttachmentDesc(TextureFormat format = TextureFormat::RGBA8) : format(format) {}
	};

	struct DepthStencilAttachmentDesc {
		TextureFormat format = TextureFormat::Depth24Stencil8;

		LoadOp depthLoadOp = LoadOp::Clear;
		StoreOp depthStoreOp = StoreOp::Store;

		LoadOp stencilLoadOp = LoadOp::DontCare;
		StoreOp stencilStoreOp = StoreOp::DontCare;

		ResourceState initialState = ResourceState::Undefined;
		ResourceState finalState = ResourceState::DepthWrite;

		DepthStencilAttachmentDesc(TextureFormat format) : format(format) {}
	};

	struct RenderPassDesc {
		std::vector<AttachmentDesc> colorAttachments;
		DepthStencilAttachmentDesc depthStencilAttachment;
		bool hasDepthStencil;

		RenderPassDesc()
			: depthStencilAttachment(TextureFormat::Depth24Stencil8),
			  hasDepthStencil(false) {
		}
	};

	// Framebuffer description
	struct FramebufferDesc {
		RenderPassHandle renderPass;

		std::vector<TextureHandle> colorAttachments;
		TextureHandle depthStencilAttachment = INVALID_HANDLE;

		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t layers = 1;

		FramebufferDesc(int w, int h)
			: renderPass(INVALID_HANDLE), depthStencilAttachment(INVALID_HANDLE),
			  width(w), height(h) {
		}

		FramebufferDesc(const IVec2& size)
			: renderPass(INVALID_HANDLE), depthStencilAttachment(INVALID_HANDLE),
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

	struct RENDERX_API CommandList : public Handle {
		void open();
		void close();
		bool isOpen = false;

		void setPipeline(const PipelineHandle& pipeline);
		void setVertexBuffer(const BufferHandle& buffer, uint64_t offset = 0);
		void setIndexBuffer(const BufferHandle& buffer, uint64_t offset = 0);
		void draw(uint32_t vertexCount, uint32_t instanceCount = 1,
			uint32_t firstVertex = 0, uint32_t firstInstance = 0);
		void drawIndexed(uint32_t indexCount, int32_t vertexOffset = 0,
			uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t firstInstance = 0);

		void beginRenderPass(
			RenderPassHandle pass,
			const ClearValue*, uint32_t);

		void endRenderPass();
	};

} // namespace  RenderX
