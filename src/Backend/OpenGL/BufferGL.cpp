// #include <GL/glew.h>

// #include "RenderXGL.h"
// #include "RenderX/Log.h"
// #include "ProLog/ProLog.h"



// namespace RenderX {

// 	static inline GLenum TOGLBufferUse(BufferUsage usages) {
// 		switch (usages) {
// 		case BufferUsage::Static:
// 			return GL_STATIC_DRAW;
// 		case BufferUsage::Dynamic:
// 			return GL_DYNAMIC_DRAW;
// 		case BufferUsage::Stream:
// 			return GL_STREAM_DRAW;
// 		default:
// 			RENDERX_ERROR("OpenGL : Unsupported Buffer Use");
// 			break;
// 		}
// 		return GL_STATIC_DRAW;
// 	};

// 	static inline GLenum TOGLBufferTarget(BufferType type) {
// 		switch (type) {
// 		case BufferType::Vertex:
// 			return GL_ARRAY_BUFFER;
// 		case BufferType::Index:
// 			return GL_ELEMENT_ARRAY_BUFFER;
// 		case BufferType::Uniform:
// 			return GL_UNIFORM_BUFFER;
// 		case BufferType::Storage:
// 			return GL_SHADER_STORAGE_BUFFER;
// 		case BufferType::Indirect:
// 			return GL_DRAW_INDIRECT_BUFFER;
// 		default:
// 			return GL_ARRAY_BUFFER;
// 		}
// 	}

// 	// namespace RenderXGL
// 	namespace RenderXGL {

// 		const BufferHandle GLCreateBuffer(size_t size,
// 			const void* data,
// 			BufferType type,
// 			BufferUsage use) {
// 			PROFILE_FUNCTION();
// 			BufferHandle handle;
// 			glGenBuffers(1, &handle.id);
// 			glBindBuffer(TOGLBufferTarget(type), handle.id);
// 			glBufferData(GL_ARRAY_BUFFER, size, data, TOGLBufferUse(use));
// 			RENDERX_INFO("OpenGL: Created Vertex Buffer | ID: {} | Size: {} bytes", handle.id, size);
// 			return handle;
// 		}
        
// 		void GLDestroyBuffer(BufferHandle handle){
// 			PROFILE_FUNCTION();
//             glDeleteBuffers(1 , const_cast<GLuint*>(&handle.id));
// 		}

// 		const BufferHandle GLCreateVertexBuffer(size_t size,
// 			const void* data,
// 			BufferUsage use) {
// 			PROFILE_FUNCTION();
// 			BufferHandle handle;
// 			glGenBuffers(1, &handle.id);
// 			glBindBuffer(GL_ARRAY_BUFFER, handle.id);
// 			glBufferData(GL_ARRAY_BUFFER, size, data, TOGLBufferUse(use));
// 			RENDERX_INFO("OpenGL: Created Vertex Buffer | ID: {} | Size: {} bytes", handle.id, size);
// 			return handle;
// 		}

// 	}
// }