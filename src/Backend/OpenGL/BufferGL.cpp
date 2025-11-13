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

	// namespace RenderXGL
	namespace RenderXGL {

		const BufferHandle GLCreateVertexBuffer(size_t size,
			const void* data,
			BufferUsage use) {
			BufferHandle id;
			glGenBuffers(1, &id);
			glBindBuffer(GL_ARRAY_BUFFER, id);
			glBufferData(GL_ARRAY_BUFFER, size, data, TOGLBufferUse(use));
			return id;
		}
	}

}