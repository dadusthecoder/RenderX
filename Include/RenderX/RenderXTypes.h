#pragma once
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Lgt {

	// Resource handles (opaque types)
	using VertexArrayHandle = uint32_t;
	using BufferHandle = uint32_t;
	using TextureHandle = uint32_t;
	using SamplerHandle = uint32_t;
	using ShaderHandle = uint32_t;
	using PipelineHandle = uint32_t;
	using FramebufferHandle = uint32_t;
	using RenderPassHandle = uint32_t;

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

	enum class RenderXAPI {
		None,
		OpenGL,
		Vulkan
	};

	enum class ShaderType {
		Vertex,
		Fragment,
		Geometry,
		TessControl,
		TessEvaluation,
		Compute
	};

	enum class BufferType { Vertex,
		Index,
		Uniform,
		Storage,
		Staging };

	enum class BufferUsage {
		Static,	 // Data never changes
		Dynamic, // Data changes occasionally
		Stream	 // Data changes every frame
	};

	enum class TextureType { Texture2D,
		Texture3D,
		TextureCube,
		Texture2DArray };

    enum class AttachmentLoadOp {
		Load,
		Clear,
		DontCare
	};

	enum class AttachmentStoreOp {
		Store,
		DontCare
	};

	enum class DataType {
		Float,
		Int,
		UInt,
		Short,
		UShort,
		Byte,
		UByte
	};

	enum class TextureFormat {
		// Color formats
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

		// Depth/stencil formats
		Depth16,
		Depth24,
		Depth32F,
		Depth24Stencil8,
		Depth32FStencil8,

		// Compressed formats
		BC1,
		BC2,
		BC3,
		BC4,
		BC5
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

		ClearColor(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f)
			: color(r, g, b, a) {
		}
		ClearColor(const Vec4& c) : color(c) {}
		ClearColor(const Vec3& c, float a = 1.0f) : color(c, a) {}
	};

	// Vertex input description
	struct VertexAttribute {
		uint32_t location;
		uint32_t count;
		DataType datatype;
		uint32_t offset;

		VertexAttribute(uint32_t loc, uint32_t count, DataType datatype, uint32_t off)
			: location(loc), count(count), datatype(datatype), offset(off) {
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

	struct VertexLayout {
		std::vector<VertexAttribute> attributes;
		std::vector<VertexBinding> bindings;
		uint32_t totalStride;
	};

	// Resource descriptions
	struct BufferDesc {
		BufferType type;
		BufferUsage usage;
		size_t size;
		const void* initialData;

		BufferDesc(BufferType t, size_t s, BufferUsage u = BufferUsage::Static,
			const void* data = nullptr)
			: type(t), usage(u), size(s), initialData(data) {
		}
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
		ShaderType type;
		std::string source;			   // GLSL source or HLSL
		std::vector<uint8_t> bytecode; // SPIR-V or compiled bytecode
		std::string entryPoint;		   // Entry function name (for HLSL/SPIR-V)

		ShaderDesc(ShaderType t, const std::string& src)
			: type(t), source(src), entryPoint("main") {
		}

		ShaderDesc(ShaderType t, const std::vector<uint8_t>& code,
			const std::string& entry = "main")
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
		BlendOp colorOp;
		BlendFunc srcAlpha;
		BlendFunc dstAlpha;
		BlendOp alphaOp;
		Vec4 blendFactor;

		BlendState()
			: enable(false), srcColor(BlendFunc::SrcAlpha),
			  dstColor(BlendFunc::OneMinusSrcAlpha), colorOp(BlendOp::Add),
			  srcAlpha(BlendFunc::One), dstAlpha(BlendFunc::Zero),
			  alphaOp(BlendOp::Add), blendFactor(1.0f) {
		}
	};

	// Pipeline description
	struct PipelineDesc {
		std::vector<ShaderHandle> shaders;
		VertexLayout vertexLayout;
		PrimitiveType primitiveType;
		RasterizerState rasterizer;
		DepthStencilState depthStencil;
		BlendState blend;
		RenderPassHandle renderPass;
		PipelineDesc()
			: primitiveType(PrimitiveType::Triangles), renderPass(INVALID_HANDLE) {
		}
	};

	// Render pass description
	struct AttachmentDesc {
		TextureFormat format;
		bool clear;
		ClearColor clearColor;
		float clearDepth;
		uint8_t clearStencil;

		AttachmentDesc(TextureFormat fmt)
			: format(fmt), clear(true), clearColor(), clearDepth(1.0f),
			  clearStencil(0) {
		}
	};

	struct RenderPassDesc {
		std::vector<AttachmentDesc> colorAttachments;
		AttachmentDesc depthStencilAttachment;
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
		int width, height;

		FramebufferDesc(int w, int h)
			: renderPass(INVALID_HANDLE), depthStencilAttachment(INVALID_HANDLE),
			  width(w), height(h) {
		}

		FramebufferDesc(const IVec2& size)
			: renderPass(INVALID_HANDLE), depthStencilAttachment(INVALID_HANDLE),
			  width(size.x), height(size.y) {
		}
	};

	// Draw command
	struct DrawCommand {
		PipelineHandle pipeline;
		std::vector<BufferHandle> vertexBuffers;
		std::vector<uint32_t> vertexOffsets;
		BufferHandle indexBuffer;
		uint32_t indexOffset;
		uint32_t indexCount;
		uint32_t vertexCount;
		uint32_t instanceCount;
		uint32_t firstInstance;

		DrawCommand()
			: pipeline(INVALID_HANDLE), indexBuffer(INVALID_HANDLE), indexOffset(0),
			  indexCount(0), vertexCount(0), instanceCount(1), firstInstance(0) {
		}
	};

	// Resource binding
	struct ResourceBinding {
		uint32_t binding;
		uint32_t set;
		enum Type {
			UniformBuffer,
			StorageBuffer,
			Texture,
			Sampler,
			CombinedTextureSampler
		} type;

		union {
			BufferHandle buffer;
			TextureHandle texture;
			SamplerHandle sampler;
		};

		uint32_t offset;
		uint32_t range;

		ResourceBinding()
			: binding(0), set(0), type(UniformBuffer), buffer(INVALID_HANDLE),
			  offset(0), range(0) {
		}
	};

} // namespace Lgx

namespace Lgx {

	// Statistics and debug info
	struct RenderStats {
		uint32_t drawCalls;
		uint32_t triangles;
		uint32_t vertices;
		uint32_t bufferBinds;
		uint32_t textureBinds;
		uint32_t shaderBinds;

		void Reset() {
			drawCalls = triangles = vertices = bufferBinds = textureBinds =
				shaderBinds = 0;
		}
	};
}; // namespace Lgx