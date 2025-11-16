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

	static inline GLuint TOGLDataType(DataType format) {
		switch (format) {
		case DataType::Float:
			return GL_FLOAT;
		case DataType::Int:
			return GL_INT;
		case DataType::UInt:
			return GL_UNSIGNED_INT;
		case DataType::Short:
			return GL_SHORT;
		case DataType::UShort:
			return GL_UNSIGNED_SHORT;
		case DataType::Byte:
			return GL_BYTE;
		case DataType::UByte:
			return GL_UNSIGNED_BYTE;
		default:
			return GL_FLOAT;
		}
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

	const VertexArrayHandle GLCreateVertexArray() {
		VertexArrayHandle vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		RENDERX_INFO("OpenGL: Created Vertex Array Object | ID: {}", vao);
		return vao;
	}

	void GLBindVertexArray(const VertexArrayHandle handle) {
		glBindVertexArray(handle);
	}

	void GLCreateVertexLayout(const VertexLayout& layout) {
		for (const auto& element : layout.attributes) {
			glVertexAttribPointer(
				element.location,
				element.count,
				GL_FLOAT,
				TOGLDataType(element.datatype),
				layout.totalStride,
				(void*)(element.offset));
			glEnableVertexAttribArray(element.location);
		}
		}

	void GLBindVertexBuffer(const BufferHandle handle) {
		glBindBuffer(GL_ARRAY_BUFFER, handle);
	}

	void GLBindIndexBuffer(const BufferHandle handle) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
	}

}
}