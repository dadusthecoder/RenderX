#include <GL/glew.h>

#include "RenderXGL.h"
#include "RenderX/Log.h"
#include "ProLog/ProLog.h"

namespace RenderX {


	// Convert engine ShaderType to OpenGL shader enums
	const static inline GLenum TOGLShaderType(ShaderType type) {
		switch (type) {
		case ShaderType::Vertex: return GL_VERTEX_SHADER;
		case ShaderType::Fragment: return GL_FRAGMENT_SHADER;
		case ShaderType::Geometry: return GL_GEOMETRY_SHADER;
		case ShaderType::Compute: return GL_COMPUTE_SHADER;
		case ShaderType::TessControl: return GL_TESS_CONTROL_SHADER;
		case ShaderType::TessEvaluation: return GL_TESS_EVALUATION_SHADER;
		default:
			RENDERX_ERROR("Unknown ShaderType: {}", static_cast<int>(type));
			return 0;
		}
	}

	// Helper: Compile a single shader
	const static inline GLuint compileShader(GLenum type, const std::string& source) {
		PROFILE_FUNCTION();
		GLuint id = glCreateShader(type);
		const char* src = source.c_str();
		glShaderSource(id, 1, &src, nullptr);
		glCompileShader(id);

		GLint result = 0;
		glGetShaderiv(id, GL_COMPILE_STATUS, &result);

		std::string shaderTypeStr;
		switch (type) {
		case GL_VERTEX_SHADER:
			shaderTypeStr = "Vertex";
			break;
		case GL_FRAGMENT_SHADER:
			shaderTypeStr = "Fragment";
			break;
		case GL_COMPUTE_SHADER:
			shaderTypeStr = "Compute";
			break;
		default:
			shaderTypeStr = "Unknown";
			break;
		}

		if (result == GL_FALSE) {
			GLint length = 0;
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> errorLog(length);
			glGetShaderInfoLog(id, length, &length, errorLog.data());

			RENDERX_ERROR("{} shader compilation failed:\n{}", shaderTypeStr, errorLog.data());
			glDeleteShader(id);
			return 0;
		}

		RENDERX_INFO("{} shader compiled successfully (ID: {}).", shaderTypeStr, id);
		return id;
	}

	const ShaderHandle RenderXGL::GLCreateShader(const ShaderDesc& desc) {
		PROFILE_FUNCTION();
		GLuint shader = compileShader(TOGLShaderType(desc.type), desc.source);
		if (shader == 0) {
			RENDERX_ERROR("Failed to create shader of type {}", (int)desc.type);
			return ShaderHandle(0);
		}
		RENDERX_INFO("Shader created successfully | ID: {}", shader);
		return ShaderHandle(shader);
	}

	const PipelineHandle RenderXGL::GLCreatePipelineGFX_Src(const std::string& vertsrc,
		const std::string& fragsrc) {
		PROFILE_FUNCTION();
		GLuint program = glCreateProgram();
		GLuint vs = compileShader(GL_VERTEX_SHADER, vertsrc);
		GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragsrc);

		if (vs == 0 || fs == 0) {
			RENDERX_ERROR("Shader compilation failed program creation aborted.");
			if (vs)
				glDeleteShader(vs);
			if (fs)
				glDeleteShader(fs);
			glDeleteProgram(program);
			return PipelineHandle(0);
		}

		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);

		GLint success = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			GLint length;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> errorLog(length);
			glGetProgramInfoLog(program, length, &length, errorLog.data());

			RENDERX_ERROR("Shader program linking error:\n{}", errorLog.data());

			glDeleteShader(vs);
			glDeleteShader(fs);
			glDeleteProgram(program);
			return PipelineHandle(0);
		}

		// Validate program
		glValidateProgram(program);
		glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
		if (!success)
			RENDERX_WARN("Shader program validation failed | Program ID: {}", program);

		// Cleanup shaders after linking
		glDeleteShader(vs);
		glDeleteShader(fs);

		RENDERX_INFO("Shader program linked successfully | Program ID: {}", program);
		return PipelineHandle(program);
	}

	const PipelineHandle RenderXGL::GLCreatePipelineGFX_Desc(const PipelineDesc& desc) {
		PROFILE_FUNCTION();
		RENDERX_WARN("GLCreatePipelineGFX_Desc not yet implemented.");
		return PipelineHandle(0);
	}

	const PipelineHandle RenderXGL::GLCreatePipelineGFX_Shader(const ShaderDesc& vertdesc,
		const ShaderDesc& fragdesc) {
		PROFILE_FUNCTION();
		RENDERX_WARN("GLCreatePipelineGFX_Shader not yet implemented.");
		return PipelineHandle(0);
	}

	const PipelineHandle RenderXGL::GLCreatePipelineCOMP_Desc(const PipelineDesc& desc) {
		PROFILE_FUNCTION();
		RENDERX_WARN("GLCreatePipelineCOMP_Desc not yet implemented.");
		return PipelineHandle(0);
	}

	const PipelineHandle RenderXGL::GLCreatePipelineCOMP_Shader(const ShaderDesc& shaderdesc) {
		PROFILE_FUNCTION();
		RENDERX_WARN("GLCreatePipelineCOMP_Shader not yet implemented.");
		return PipelineHandle(0);
	}

	const PipelineHandle RenderXGL::GLCreatePipelineCOMP_Src(const std::string& compsrc) {
		PROFILE_FUNCTION();
		GLuint cs = compileShader(GL_COMPUTE_SHADER, compsrc);
		if (!cs) {
			RENDERX_ERROR("Compute shader compilation failed cannot create program.");
			return PipelineHandle(0);
		}

		GLuint program = glCreateProgram();
		glAttachShader(program, cs);
		glLinkProgram(program);

		GLint success = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			GLint length;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> errorLog(length);
			glGetProgramInfoLog(program, length, &length, errorLog.data());

			RENDERX_ERROR("Compute shadeFrentr linking error:\n{}", errorLog.data());
			glDeleteShader(cs);
			glDeleteProgram(program);
			return PipelineHandle(0);
		}

		glDeleteShader(cs);
		RENDERX_INFO("Compute shader pipeline created successfully | Program ID: {}", program);
		return PipelineHandle(program);
	}

	// Binding functions

	void RenderXGL::GLBindPipeline(const PipelineHandle handle) {
		PROFILE_FUNCTION();
		glUseProgram(handle.id);
	}

}