#include <GL/glew.h>

#include "RenderXGL.h"
#include "RenderX/Log.h"

// namespace Lng
namespace Lgt {

	static inline GLuint TOGLBufferUse(BufferUsage usages) {
		switch (usages) {
		case BufferUsage::Static:
			return GL_STATIC_DRAW;
		case BufferUsage::Dynamic:
			return GL_DYNAMIC_DRAW;
		case BufferUsage::Stream:
			return GL_STREAM_DRAW;
		default:
			RENDERX_ERROR("OpenGL : Unsupported Buffer Use");
			break;
		}
		return GL_STATIC_DRAW;
	};

	static inline GLuint TOGLFormat(TextureFormat format) {
		switch (format) {
		case TextureFormat::R8:
			return GL_RED;
		case TextureFormat::RG8:
			return GL_RG;
		case TextureFormat::RGB8:
			return GL_RGB;
		case TextureFormat::RGBA8:
			return GL_RGBA;
		case TextureFormat::Depth24Stencil8:
			return GL_DEPTH_STENCIL;
		default:
			RENDERX_ERROR("OpenGL : Unsupported Texture Format");
			break;
		}
		return GL_RGBA;
	}; 
    
	// namespace RenderXGL
	namespace RenderXGL {

		const BufferHandle GLCreateVertexBuffer(size_t size,
			const void* data,
			BufferUsage use) {
			BufferHandle id;
			glGenBuffers(1, &id);
			glBindBuffer(GL_ARRAY_BUFFER, id);
			glBufferData(GL_ARRAY_BUFFER, size, data, TOGLBufferUse(use));
			RENDERX_INFO("OpenGL: Created Vertex Buffer | ID: {} | Size: {} bytes", id, size);
			return id;
		}

		const VertexLayoutHandle GLCreateVertexLayout(const VertexLayout& layout) {
			GLuint vao;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			for (const auto& element : layout.attributes) {
				glEnableVertexAttribArray(element.location);
				glVertexAttribPointer(
					element.location,
					element.count,
					TOGLFormat(element.format),	
					GL_FALSE,
					layout.totalStride, 
					(void*)(element.offset));
			}

			RENDERX_INFO("OpenGL: Created Vertex Layout | VAO ID: {}", vao);
			return vao;
		}

		const void GLBindVertexBuffer(const BufferHandle handle) {
			glBindBuffer(GL_ARRAY_BUFFER, handle);
		}

		const void GLBindLayout(const VertexLayoutHandle handle) {
			glBindVertexArray(handle);
		}

		const void GLBindIndexBuffer(const BufferHandle handle) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
		}

	}


}