#include "CommonGL.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "RenderXGL.h"
#include "RenderX/Log.h"
#include "ProLog/ProLog.h"

namespace Rx {

namespace RxGL {

	// Cache of pipeline descriptions, keyed by GL program ID.
	static std::unordered_map<uint32_t, PipelineDesc> PipelineCache;

	// Convert engine ShaderType to OpenGL shader enums
	static inline GLenum TOGLShaderType(ShaderStage stage) {
		switch (stage) {
		case ShaderStage::Vertex: return GL_VERTEX_SHADER;
		case ShaderStage::Fragment: return GL_FRAGMENT_SHADER;
		case ShaderStage::Geometry: return GL_GEOMETRY_SHADER;
		case ShaderStage::Compute: return GL_COMPUTE_SHADER;
		case ShaderStage::TessControl: return GL_TESS_CONTROL_SHADER;
		case ShaderStage::TessEvaluation: return GL_TESS_EVALUATION_SHADER;
		default:
			RENDERX_ERROR("Unknown ShaderType: {}", static_cast<int>(stage));
			return 0;
		}
	}

	static inline GLenum TOGLPrimitive(PrimitiveType p) {
		switch (p) {
		case PrimitiveType::Points: return GL_POINTS;
		case PrimitiveType::Lines: return GL_LINES;
		case PrimitiveType::LineStrip: return GL_LINE_STRIP;
		case PrimitiveType::Triangles: return GL_TRIANGLES;
		case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
		case PrimitiveType::TriangleFan: return GL_TRIANGLE_FAN;
		}
		return GL_TRIANGLES;
	}

	static inline GLenum TOGLCompare(CompareFunc f) {
		switch (f) {
		case CompareFunc::Never: return GL_NEVER;
		case CompareFunc::Less: return GL_LESS;
		case CompareFunc::Equal: return GL_EQUAL;
		case CompareFunc::LessEqual: return GL_LEQUAL;
		case CompareFunc::Greater: return GL_GREATER;
		case CompareFunc::NotEqual: return GL_NOTEQUAL;
		case CompareFunc::GreaterEqual: return GL_GEQUAL;
		case CompareFunc::Always: return GL_ALWAYS;
		}
		return GL_LESS;
	}

	static inline GLenum TOGLBlendFunc(BlendFunc f) {
		switch (f) {
		case BlendFunc::Zero: return GL_ZERO;
		case BlendFunc::One: return GL_ONE;
		case BlendFunc::SrcColor: return GL_SRC_COLOR;
		case BlendFunc::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
		case BlendFunc::DstColor: return GL_DST_COLOR;
		case BlendFunc::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
		case BlendFunc::SrcAlpha: return GL_SRC_ALPHA;
		case BlendFunc::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
		case BlendFunc::DstAlpha: return GL_DST_ALPHA;
		case BlendFunc::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
		case BlendFunc::ConstantColor: return GL_CONSTANT_COLOR;
		case BlendFunc::OneMinusConstantColor: return GL_ONE_MINUS_CONSTANT_COLOR;
		}
		return GL_ONE;
	}

	static inline GLenum TOGLBlendOp(BlendOp op) {
		switch (op) {
		case BlendOp::Add: return GL_FUNC_ADD;
		case BlendOp::Subtract: return GL_FUNC_SUBTRACT;
		case BlendOp::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
		case BlendOp::Min: return GL_MIN;
		case BlendOp::Max: return GL_MAX;
		}
		return GL_FUNC_ADD;
	}

	// Helper: Compile a single shader
	static inline GLuint compileShader(GLenum type, const std::string& source) {
		PROFILE_FUNCTION();
		GLuint id = glCreateShader(type);
		const char* src = source.c_str();
		glShaderSource(id, 1, &src, nullptr);
		glCompileShader(id);

		GLint result = 0;
		glGetShaderiv(id, GL_COMPILE_STATUS, &result);

		std::string shaderTypeStr;
		switch (type) {
		case GL_VERTEX_SHADER: shaderTypeStr = "Vertex"; break;
		case GL_FRAGMENT_SHADER: shaderTypeStr = "Fragment"; break;
		case GL_COMPUTE_SHADER: shaderTypeStr = "Compute"; break;
		default: shaderTypeStr = "Unknown"; break;
		}

		if (result == GL_FALSE) {
			GLint length = 0;
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> errorLog(static_cast<size_t>(length));
			glGetShaderInfoLog(id, length, &length, errorLog.data());

			RENDERX_ERROR("{} shader compilation failed:\n{}", shaderTypeStr, errorLog.data());
			glDeleteShader(id);
			return 0;
		}

		RENDERX_INFO("{} shader compiled successfully (ID: {}).", shaderTypeStr, id);
		return id;
	}

	ShaderHandle GLCreateShader(const ShaderDesc& desc) {
		PROFILE_FUNCTION();

		if (desc.source.empty()) {
			RENDERX_ERROR("OpenGL backend expects GLSL source in ShaderDesc::source");
			return ShaderHandle(0);
		}

		GLuint shader = compileShader(TOGLShaderType(desc.type), desc.source);
		if (shader == 0) {
			RENDERX_ERROR("Failed to create shader of type {}", static_cast<int>(desc.type));
			return ShaderHandle(0);
		}

		RENDERX_INFO("Shader created successfully | ID: {}", shader);
		return ShaderHandle(shader);
	}

	PipelineHandle GLCreateGraphicsPipeline(PipelineDesc& desc) {
		PROFILE_FUNCTION();

		GLuint program = glCreateProgram();


		for (auto shader : desc.shaders) {
			glAttachShader(program, shader.id);
		}

		glLinkProgram(program);

		GLint success = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			GLint length = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> errorLog(static_cast<size_t>(length));
			glGetProgramInfoLog(program, length, &length, errorLog.data());

			RENDERX_ERROR("Shader program linking error:\n{}", errorLog.data());

			glDeleteProgram(program);
			return PipelineHandle(0);
		}

		// Validate program
		glValidateProgram(program);
		glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
		if (!success) {
			RENDERX_WARN("Shader program validation failed | Program ID: {}", program);
		}

		// Shader deletion is handled by client-side code when appropriate
		RENDERX_INFO("Shader program linked successfully | Program ID: {}", program);

		// Only insert into the cache after a successful link
		PipelineCache.emplace(static_cast<uint32_t>(program), desc);

		return PipelineHandle(program);
	}

	// Binding functions (used by GL command list execution)
	void GLBindPipeline(const PipelineHandle handle) {
		PROFILE_FUNCTION();

		glUseProgram(handle.id);

		// Lookup the pipeline description from the cache safely
		auto it = PipelineCache.find(handle.id);
		if (it == PipelineCache.end()) {
			RENDERX_ERROR("Pipeline (program {}) not found in cache", handle.id);
			return;
		}

		// Return if the pipeline is Compute
		if (it->second.type == PipelineType::Compute)
			return;

		// Fill mode
		switch (it->second.rasterizer.fillMode) {
		case FillMode::Solid: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
		case FillMode::Wireframe: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
		case FillMode::Point: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
		}

		// Culling
		switch (PipelineCache[handle.id].rasterizer.cullMode) {
		case CullMode::None:
			glDisable(GL_CULL_FACE);
			break;
		case CullMode::Front:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			break;
		case CullMode::Back:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			break;
		case CullMode::FrontAndBack:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT_AND_BACK);
			break;
		}

		// Front winding
		glFrontFace(it->second.rasterizer.frontCounterClockwise ? GL_CCW : GL_CW);

		// Depth bias
		if (PipelineCache[handle.id].rasterizer.depthBias != 0.0f ||
			PipelineCache[handle.id].rasterizer.slopeScaledDepthBias != 0.0f) {
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(PipelineCache[handle.id].rasterizer.slopeScaledDepthBias,
				PipelineCache[handle.id].rasterizer.depthBias);
		}
		else {
			glDisable(GL_POLYGON_OFFSET_FILL);
		}

		// Depth-Stencil State
		if (it->second.depthStencil.depthEnable) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(TOGLCompare(it->second.depthStencil.depthFunc));
			glDepthMask(it->second.depthStencil.depthWriteEnable ? GL_TRUE : GL_FALSE);
		}
		else {
			glDisable(GL_DEPTH_TEST);
		}

		if (it->second.depthStencil.stencilEnable) {
			glEnable(GL_STENCIL_TEST);
			glStencilMask(it->second.depthStencil.stencilWriteMask);
		}
		else {
			glDisable(GL_STENCIL_TEST);
		}

		// Blending State
		if (it->second.blend.enable) {
			glEnable(GL_BLEND);

			glBlendFuncSeparate(
				TOGLBlendFunc(it->second.blend.srcColor),
				TOGLBlendFunc(it->second.blend.dstColor),
				TOGLBlendFunc(it->second.blend.srcAlpha),
				TOGLBlendFunc(it->second.blend.dstAlpha));

			glBlendEquationSeparate(
				TOGLBlendOp(it->second.blend.colorOp),
				TOGLBlendOp(it->second.blend.alphaOp));

			glBlendColor(it->second.blend.blendFactor.r,
				it->second.blend.blendFactor.g,
				it->second.blend.blendFactor.b,
				it->second.blend.blendFactor.a);
		}
		else {
			glDisable(GL_BLEND);
		}

		glUseProgram(handle.id);
	}

	// Accessor used by GL command list execution to fetch the
	// vertex input layout for a given pipeline.
	const PipelineDesc* GLGetPipelineDesc(const PipelineHandle handle) {
		auto it = PipelineCache.find(handle.id);
		if (it == PipelineCache.end())
			return nullptr;
		return &it->second;
	}

} // namespace RxGL

} // namespace Rx
