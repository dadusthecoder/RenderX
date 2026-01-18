#include <GL/glew.h>

#include "RenderXGL.h"
#include "RenderX/Log.h"
#include "ProLog/ProLog.h"

namespace RenderX {

	static inline GLenum TOGLBufferUse(BufferUsage usage) {
		switch (usage) {
		case BufferUsage::Static:
			return GL_STATIC_DRAW;
		case BufferUsage::Dynamic:
			return GL_DYNAMIC_DRAW;
		case BufferUsage::Stream:
			return GL_STREAM_DRAW;
		default:
			RENDERX_ERROR("OpenGL: Unsupported BufferUsage");
			break;
		}
		return GL_STATIC_DRAW;
	};

	static inline GLenum TOGLBufferTarget(BufferType type) {
		switch (type) {
		case BufferType::Vertex:
			return GL_ARRAY_BUFFER;
		case BufferType::Index:
			return GL_ELEMENT_ARRAY_BUFFER;
		case BufferType::Uniform:
			return GL_UNIFORM_BUFFER;
		case BufferType::Storage:
			return GL_SHADER_STORAGE_BUFFER;
		case BufferType::Indirect:
			return GL_DRAW_INDIRECT_BUFFER;
		default:
			return GL_ARRAY_BUFFER;
		}
	}

	namespace RenderXGL {

		// Low-level helper used by the RHI entry point
		const BufferHandle GLCreateBuffer(size_t size,
			const void* data,
			BufferType type,
			BufferUsage use) {
			PROFILE_FUNCTION();
			BufferHandle handle;
			GLenum target = TOGLBufferTarget(type);
			glGenBuffers(1, &handle.id);
			glBindBuffer(target, handle.id);
			glBufferData(target, static_cast<GLsizeiptr>(size), data, TOGLBufferUse(use));
			RENDERX_INFO("OpenGL: Created Buffer | ID: {} | Size: {} bytes", handle.id, size);
			return handle;
		}

		void GLDestroyBuffer(BufferHandle handle) {
			PROFILE_FUNCTION();
			GLuint id = static_cast<GLuint>(handle.id);
			glDeleteBuffers(1, &id);
		}

		const BufferHandle GLCreateVertexBuffer(size_t size,
			const void* data,
			BufferUsage use) {
			PROFILE_FUNCTION();
			return GLCreateBuffer(size, data, BufferType::Vertex, use);
		}

		// RHI entry point required by RenderXAPI.def
		BufferHandle GLCreateBuffer(const BufferDesc& desc) {
			return GLCreateBuffer(desc.size, desc.initialData, desc.type, desc.usage);
		}

	} // namespace RenderXGL

} // namespace RenderX
