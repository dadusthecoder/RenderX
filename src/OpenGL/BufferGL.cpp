#include "CommonGL.h"

namespace Rx {

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

	namespace RxGL {

		// Low-level helper used by the RHI entry point
		BufferHandle GLCreateBuffer(const BufferDesc& desc) {
			PROFILE_FUNCTION();
			BufferHandle handle;
			GLenum target = TOGLBufferTarget(desc.type);
			glGenBuffers(1, reinterpret_cast<GLuint*>(&handle.id));
			glBindBuffer(target, handle.id);
			glBufferData(target, static_cast<GLsizeiptr>(desc.size), desc.initialData, TOGLBufferUse(desc.usage));
			RENDERX_INFO("OpenGL: Created Buffer | ID: {} | Size: {} bytes", handle.id, desc.size);
			return handle;
		}

		void GLDestroyBuffer(BufferHandle handle) {
			PROFILE_FUNCTION();
			GLuint id = static_cast<GLuint>(handle.id);
			glDeleteBuffers(1, &id);
		}

		void GLCmdSetVertexBuffer(CommandList& cmdList, BufferHandle buffer, uint64_t offset) {
			PROFILE_FUNCTION();
			RENDERX_ASSERT_MSG(cmdList.isOpen, "CommandList must be open");

			glBindBuffer(GL_ARRAY_BUFFER, buffer.id);

			// NOTE:
			// Vertex attribute setup (glVertexAttribPointer)
			// should be handled by your pipeline/VAO system,
			// NOT here.
		}

		void GLCmdSetIndexBuffer(CommandList& cmdList, BufferHandle buffer, uint64_t offset) {
			PROFILE_FUNCTION();
			RENDERX_ASSERT_MSG(cmdList.isOpen, "CommandList must be open");

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.id);
		}

	} // namespace RxGL

} // namespace Rx
