#include <GL/glew.h>

#include "RenderXGL.h"
#include "RenderX/Log.h"
#include "ProLog/ProLog.h"



namespace RenderX {

	static inline GLenum TOGLBufferUse(BufferUsage usages) {
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

	static inline GLenum TOGLDataType(DataType format) {
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
		case BufferType::CopySrc:
			return GL_COPY_READ_BUFFER;
		case BufferType::CopyDst:
			return GL_COPY_WRITE_BUFFER;
		default:
			return GL_ARRAY_BUFFER;
		}
	}

	// namespace RenderXGL
	namespace RenderXGL {

		const BufferHandle GLCreateBuffer(size_t size,
			const void* data,
			BufferType type,
			BufferUsage use) {
			PROFILE_FUNCTION();
			BufferHandle handle;
			glGenBuffers(1, &handle.id);
			glBindBuffer(TOGLBufferTarget(type), handle.id);
			glBufferData(GL_ARRAY_BUFFER, size, data, TOGLBufferUse(use));
			RENDERX_INFO("OpenGL: Created Vertex Buffer | ID: {} | Size: {} bytes", handle.id, size);
			return handle;
		}
        
		void GLDestroyBuffer(BufferHandle handle){
			PROFILE_FUNCTION();
            glDeleteBuffers(1 , const_cast<GLuint*>(&handle.id));
		}

		const BufferHandle GLCreateVertexBuffer(size_t size,
			const void* data,
			BufferUsage use) {
			PROFILE_FUNCTION();
			BufferHandle handle;
			glGenBuffers(1, &handle.id);
			glBindBuffer(GL_ARRAY_BUFFER, handle.id);
			glBufferData(GL_ARRAY_BUFFER, size, data, TOGLBufferUse(use));
			RENDERX_INFO("OpenGL: Created Vertex Buffer | ID: {} | Size: {} bytes", handle.id, size);
			return handle;
		}

		void GLBindIndexBuffer(const BufferHandle handle) {
			PROFILE_FUNCTION();
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle.id);
		}

		void GLBindVertexBuffer(const BufferHandle handle) {
			PROFILE_FUNCTION();
			glBindBuffer(GL_ARRAY_BUFFER, handle.id);
		}

		const VertexArrayHandle GLCreateVertexArray() {
			PROFILE_FUNCTION();
			VertexArrayHandle vao;
			glGenVertexArrays(1, &vao.id);
			glBindVertexArray(vao.id);
			RENDERX_INFO("OpenGL: Created Vertex Array Object | ID: {}", vao.id);
			return vao;
		}

		void GLBindVertexArray(const VertexArrayHandle handle) {
			PROFILE_FUNCTION();
			glBindVertexArray(handle.id);
		}

		void GLCreateVertexLayout(const VertexLayout& layout) {
			PROFILE_FUNCTION();
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




	}
}