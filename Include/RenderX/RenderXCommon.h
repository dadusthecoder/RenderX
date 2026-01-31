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


#define RENDERX_DEFINE_HANDLE(Name) \
	namespace HandleType {          \
		struct Name {};             \
	}                               \
	using Name##Handle = Handle<HandleType::Name>;


namespace Rx {
	// Base Handle Template
	template <typename Tag>
	struct Handle {
	public:
		using ValueType = uint64_t;
		static constexpr ValueType INVALID = 0;

		Handle(ValueType key) : id(key) {}
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
		uint32_t maxFramesInFlight = 3;
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
	ENABLE_BITMASK_OPERATORS(ShaderStage)


	// === Resource State Tracking ===
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

	ENABLE_BITMASK_OPERATORS(ResourceState)

	enum class ResourceGroupLifetime : uint8_t {
		Persistent,
		PerFrame
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
		std::vector<VertexBinding> vertexBindings;
	};

	// Resource descriptions for craeation
	enum class BufferFlags : uint32_t {
		Vertex = 1 << 0,
		Index = 1 << 1,
		Uniform = 1 << 2,
		Storage = 1 << 3,
		Indirect = 1 << 4,
		TransferSrc = 1 << 5,
		TransferDst = 1 << 6,
		Static = 1 << 8,
		Dynamic = 1 << 9,
		Streaming = 1 << 10,
	};
	ENABLE_BITMASK_OPERATORS(BufferFlags)

	inline bool IsValidBufferFlags(BufferFlags flags) {
		// Can't be both static and dynamic
		bool hasStatic = Has(flags, BufferFlags::Static);
		bool hasDynamic = Has(flags, BufferFlags::Dynamic);

		if (hasStatic && hasDynamic) {
			return false; // Invalid combination
		}

		// Must have at least one usage flag (not just Static/Dynamic)
		BufferFlags usageMask = BufferFlags::Vertex | BufferFlags::Index |
								BufferFlags::Uniform | BufferFlags::Storage |
								BufferFlags::Indirect | BufferFlags::TransferSrc |
								BufferFlags::TransferDst;

		if (Has(flags, usageMask)) {
			return false; // Must have at least one usage
		}

		return true;
	}

	enum class MemoryType : uint8_t {
		GpuOnly = 1 << 0,  // Device-local, no CPU access
		CpuToGpu = 1 << 1, // Host-visible, optimized for CPU writes
		GpuToCpu = 1 << 2, // Host-visible, optimized for CPU reads (cached)
		CpuOnly = 1 << 3,  // Host memory, rarely accessed by GPU
		Auto = 1 << 4,	   // Prefer device-local but allow fallback
	};

	struct BufferDesc {
		BufferFlags flags;
		MemoryType momory;
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
			: fillMode(FillMode::Solid), cullMode(CullMode::None),
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



	// === Sahder Resource Groups (NEW) ===
	// here ResourceGroup stands for Shader Resource Group
	/// Describes a single binding slot in a ResourceGroupLayout
	struct ResourceGroupLayoutItem {
		// Binding index (e.g., layout(binding = 0))
		uint32_t binding;
		// What kind of resource
		ResourceType type;
		// Which shader stages can access this
		ShaderStage stages;
		// Array size (1 for non-arrays, >1 for arrays)
		uint32_t count;

		ResourceGroupLayoutItem()
			: binding(0), type(ResourceType::ConstantBuffer),
			  stages(ShaderStage::All), count(1) {}

		ResourceGroupLayoutItem(uint32_t bind, ResourceType t, ShaderStage s = ShaderStage::All, uint32_t cnt = 1)
			: binding(bind), type(t), stages(s), count(cnt) {}

		// Convenience factory methods
		static ResourceGroupLayoutItem ConstantBuffer(uint32_t binding, ShaderStage stages = ShaderStage::All) {
			return ResourceGroupLayoutItem(binding, ResourceType::ConstantBuffer, stages, 1);
		}

		static ResourceGroupLayoutItem StorageBuffer(uint32_t binding, ShaderStage stages = ShaderStage::All, bool writable = false) {
			return ResourceGroupLayoutItem(binding,
				writable ? ResourceType::RWStorageBuffer : ResourceType::StorageBuffer,
				stages, 1);
		}

		static ResourceGroupLayoutItem Texture_SRV(uint32_t binding, ShaderStage stages = ShaderStage::All, uint32_t count = 1) {
			return ResourceGroupLayoutItem(binding, ResourceType::Texture_SRV, stages, count);
		}

		static ResourceGroupLayoutItem Texture_UAV(uint32_t binding, ShaderStage stages = ShaderStage::All, uint32_t count = 1) {
			return ResourceGroupLayoutItem(binding, ResourceType::Texture_UAV, stages, count);
		}

		static ResourceGroupLayoutItem Sampler(uint32_t binding, ShaderStage stages = ShaderStage::All) {
			return ResourceGroupLayoutItem(binding, ResourceType::Sampler, stages, 1);
		}

		static ResourceGroupLayoutItem CombinedTextureSampler(uint32_t binding, ShaderStage stages = ShaderStage::All) {
			return ResourceGroupLayoutItem(binding, ResourceType::CombinedTextureSampler, stages, 1);
		}
	};

	/// Describes the layout/structure of a pipeline inputs
	struct ResourceGroupLayout {
		std::vector<ResourceGroupLayoutItem> resourcebindings;
		uint32_t setIndex = 0;
		const char* debugName = nullptr;

		ResourceGroupLayout() = default;

		ResourceGroupLayout(const std::vector<ResourceGroupLayoutItem>& items)
			: resourcebindings(items) {}
	};

	/// Describes a single resource binding in a descriptor set instance
	struct ResourceGroupItem {
		uint32_t binding;
		ResourceType type;

		// Union for different resource types. CombinedTextureSampler needs both
		// texture and sampler stored simultaneously; place those into a small
		// struct inside the union to avoid overwriting when both are used.
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


		uint32_t arrayIndex = 0; // For Resource to arrays of descriptors

		ResourceGroupItem()
			: binding(0), type(ResourceType::ConstantBuffer), rawHandle(0) {}

		// Convenience factory methods
		static ResourceGroupItem ConstantBuffer(uint32_t binding, BufferViewHandle buf) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type = ResourceType::ConstantBuffer;
			item.bufferView = buf;
			return item;
		}

		static ResourceGroupItem StorageBuffer(uint32_t binding, BufferViewHandle buf, bool writable = false) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type = writable ? ResourceType::RWStorageBuffer : ResourceType::StorageBuffer;
			item.bufferView = buf;
			return item;
		}

		static ResourceGroupItem Texture_SRV(uint32_t binding, TextureViewHandle tex, uint32_t arrayIndex = 0) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type = ResourceType::Texture_SRV;
			item.textureView = tex;
			item.arrayIndex = arrayIndex;
			return item;
		}

		static ResourceGroupItem Texture_UAV(uint32_t binding, TextureViewHandle tex, uint32_t mipLevel = 0) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type = ResourceType::Texture_UAV;
			item.textureView = tex;
			return item;
		}

		static ResourceGroupItem Sampler(uint32_t binding, SamplerHandle samp) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type = ResourceType::Sampler;
			item.sampler = samp;
			return item;
		}

		static ResourceGroupItem CombinedTextureSampler(uint32_t binding, TextureViewHandle tex, SamplerHandle samp) {
			ResourceGroupItem item;
			item.binding = binding;
			item.type = ResourceType::CombinedTextureSampler;
			// Store both handles into the combined struct so the union doesn't
			// overwrite one with the other.
			item.combinedHandles.texture = tex;
			item.combinedHandles.sampler = samp;
			return item;
		}
	};

	/// Describes a ResourceGroup instance
	struct ResourceGroupDesc {
		PipelineLayoutHandle layout;			  // Must match layout used in pipeline
		std::vector<ResourceGroupItem> Resources; // Actual resources to bind
		ResourceGroupLifetime flags = ResourceGroupLifetime::Persistent;
		uint32_t setIndex = 0;
		const char* debugName = nullptr;

		ResourceGroupDesc() = default;

		ResourceGroupDesc(PipelineLayoutHandle layoutHandle, const std::vector<ResourceGroupItem>& items)
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
		PipelineLayoutHandle layout;

		PipelineDesc()
			: primitiveType(PrimitiveType::Triangles), renderPass(0) {}
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


	struct RENDERX_API CommandList {
		void open();
		void close();
		bool isOpen = false; // deprected
		uint64_t id;
		bool IsValid() const { return id != 0; };
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
		void writeBuffer(BufferHandle handle, void* data, uint32_t offset, uint32_t size);
		void setResourceGroup(const ResourceGroupHandle& handle);
	};

} // namespace  RenderX
