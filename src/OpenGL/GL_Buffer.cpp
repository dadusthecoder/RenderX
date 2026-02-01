#include "CommonGL.h"

namespace Rx {
namespace RxGL {
	// Helper to get OpenGL buffer target from BufferFlags
	GLenum ToGLBufferTarget(BufferFlags flags) {
		// OpenGL requires you to bind a buffer to a specific target
		// Priority order: more specific targets first

		if (Has(flags, BufferFlags::Uniform))
			return GL_UNIFORM_BUFFER;

		if (Has(flags, BufferFlags::Storage))
			return GL_SHADER_STORAGE_BUFFER;

		if (Has(flags, BufferFlags::Indirect))
			return GL_DRAW_INDIRECT_BUFFER;

		if (Has(flags, BufferFlags::Index))
			return GL_ELEMENT_ARRAY_BUFFER;

		if (Has(flags, BufferFlags::Vertex))
			return GL_ARRAY_BUFFER;

		// Transfer-only buffers
		if (Has(flags, BufferFlags::TransferSrc))
			return GL_COPY_READ_BUFFER;

		if (Has(flags, BufferFlags::TransferDst))
			return GL_COPY_WRITE_BUFFER;

		// Fallback
		return GL_ARRAY_BUFFER;
	}

	// Convert to glBufferData usage hint (for mutable buffers)
	GLenum ToGLBufferUsage(BufferFlags flags) {
		// OpenGL usage hints: <frequency>_<nature>
		// Frequency: STREAM (modified once, used few times)
		//           STATIC (modified once, used many times)
		//           DYNAMIC (modified repeatedly, used many times)
		// Nature: DRAW (modified by app, used by GL for drawing)
		//        READ (modified by GL, read by app)
		//        COPY (modified by GL, used by GL)

		bool isDynamic = Has(flags, BufferFlags::Dynamic);
		bool isStatic = Has(flags, BufferFlags::Static);
		bool isTransferSrc = Has(flags, BufferFlags::TransferSrc);
		bool isTransferDst = Has(flags, BufferFlags::TransferDst);

		// Determine nature
		if (isTransferSrc && !isTransferDst) {
			// Buffer is read by GL for transfers
			return isDynamic ? GL_DYNAMIC_READ : GL_STATIC_READ;
		}
		else if (isTransferDst && !isTransferSrc) {
			// Buffer is written by GL (less common)
			return isDynamic ? GL_DYNAMIC_COPY : GL_STATIC_COPY;
		}
		else if (isDynamic) {
			// Frequently updated by CPU
			return GL_DYNAMIC_DRAW;
		}
		else {
			// Default: static draw (most common)
			return GL_STATIC_DRAW;
		}
	}

	// Convert to glBufferStorage flags (for immutable buffers - preferred in modern GL)
	GLbitfield ToGLStorageFlags(BufferFlags flags) {
		// glBufferStorage creates immutable storage (GL 4.4+, more efficient)
		// Unlike glBufferData, you specify access patterns up front

		GLbitfield glFlags = 0;

		// Map access flags
		if (Has(flags, BufferFlags::Dynamic)) {
			// Dynamic = CPU will write to this buffer
			glFlags |= GL_MAP_WRITE_BIT;
			glFlags |= GL_DYNAMIC_STORAGE_BIT; // Allow updates

			// Optional: persistent mapping for high-frequency updates
			// glFlags |= GL_MAP_PERSISTENT_BIT;
			// glFlags |= GL_MAP_COHERENT_BIT;
		}

		if (Has(flags, BufferFlags::TransferSrc)) {
			// Will be read back by CPU
			glFlags |= GL_MAP_READ_BIT;
		}

		if (Has(flags, BufferFlags::TransferDst)) {
			// Will receive data from CPU or GPU
			glFlags |= GL_DYNAMIC_STORAGE_BIT;
		}

		// Client storage hint (rarely used)
		// glFlags |= GL_CLIENT_STORAGE_BIT;

		return glFlags;
	}

	// MemoryType doesn't map directly to OpenGL (it's more implicit)
	// But we can provide hints for allocation strategy
	struct GLBufferHints {
		bool preferPersistentMapping; // Use GL_MAP_PERSISTENT_BIT
		bool preferCoherentMapping;	  // Use GL_MAP_COHERENT_BIT
		bool useBufferStorage;		  // Use glBufferStorage vs glBufferData
	};

	GLBufferHints ToGLBufferHints(MemoryType type, BufferFlags flags) {
		GLBufferHints hints = {};

		bool isDynamic = Has(flags, BufferFlags::Dynamic);

		switch (type) {
		case MemoryType::GpuOnly:
			// Immutable storage, no CPU access
			hints.preferPersistentMapping = false;
			hints.preferCoherentMapping = false;
			hints.useBufferStorage = true; // Immutable is best for GPU-only
			break;

		case MemoryType::CpuToGpu:
			// Frequent CPU writes
			hints.preferPersistentMapping = isDynamic; // Keep mapped if dynamic
			hints.preferCoherentMapping = true;		   // Auto-sync writes
			hints.useBufferStorage = isDynamic;		   // Persistent mapping needs storage
			break;

		case MemoryType::GpuToCpu:
			// GPU writes, CPU reads
			hints.preferPersistentMapping = false; // Usually map on-demand
			hints.preferCoherentMapping = true;
			hints.useBufferStorage = true;
			break;

		case MemoryType::CpuOnly:
			// Rare: pure staging buffer
			hints.preferPersistentMapping = false;
			hints.preferCoherentMapping = true;
			hints.useBufferStorage = false; // glBufferData is fine
			break;

		case MemoryType::Auto:
			// Let driver decide
			hints.preferPersistentMapping = isDynamic;
			hints.preferCoherentMapping = isDynamic;
			hints.useBufferStorage = true; // Modern path
			break;
		}

		return hints;
	}

	struct GLBufferInfo {
		GLenum target;
		GLenum usage;			 // For glBufferData path
		GLbitfield storageFlags; // For glBufferStorage path
		GLBufferHints hints;
	};

	GLBufferInfo ToGLBufferInfo(BufferFlags flags, MemoryType memory) {
		GLBufferInfo info = {};
		info.target = ToGLBufferTarget(flags);
		info.usage = ToGLBufferUsage(flags);
		info.storageFlags = ToGLStorageFlags(flags);
		info.hints = ToGLBufferHints(memory, flags);
		return info;
	}

	// Low-level helper used by the RHI entry point
	BufferHandle GLCreateBuffer(const BufferDesc& desc) {
		PROFILE_FUNCTION();
		BufferHandle handle;
		GLuint buffer;
		glCreateBuffers(1, &buffer); // DSA (GL 4.5+)
		// or: glGenBuffers(1, &buffer);

		GLBufferInfo info = ToGLBufferInfo(desc.flags, desc.memoryType);

		if (info.hints.useBufferStorage) {
			// Modern path: immutable storage (GL 4.4+)
			GLbitfield storageFlags = info.storageFlags;

			if (info.hints.preferPersistentMapping && Has(desc.flags, BufferFlags::Dynamic)) {
				storageFlags |= GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
			}

			glNamedBufferStorage(buffer, desc.size, desc.initialData, storageFlags);
			// or: glBindBuffer + glBufferStorage for non-DSA
		}
		else {
			// Legacy path: mutable storage
			glNamedBufferData(buffer, desc.size, desc.initialData, info.usage);
			// or: glBindBuffer + glBufferData for non-DSA
		}

		glGenBuffers(1, reinterpret_cast<GLuint*>(&handle.id));
		glBindBuffer(info.target, handle.id);
		glBufferData(info.target, static_cast<GLsizeiptr>(desc.size), desc.initialData, info.usage);
		RENDERX_INFO("OpenGL: Created Buffer | ID: {} | Size: {} bytes", handle.id, desc.size);
		return handle;
	}

	BufferViewHandle GLCreateBufferView(const BufferViewDesc& desc) {
		PROFILE_FUNCTION();
		BufferViewHandle handle;
		// In OpenGL we don't have a distinct buffer-view object in this simple RHI.
		// Map the BufferView handle to the underlying buffer handle id so
		// higher-level code can use the view as a reference to the buffer.
		handle.id = desc.buffer.id;
		return handle;
	}

	void GLDestroyBufferView(BufferViewHandle& handle) {
		PROFILE_FUNCTION();
		// No-op for OpenGL backend; buffer-view is not a distinct GPU object here.
		// Accept by-reference to match the public API signature.
		handle.id = 0;
	}

	void GLDestroyBuffer(BufferHandle handle) {
		PROFILE_FUNCTION();
		GLuint id = static_cast<GLuint>(handle.id);
		glDeleteBuffers(1, &id);
	}

	void GLCmdSetVertexBuffer(CommandList& cmdList, BufferHandle buffer, uint64_t offset) {
		PROFILE_FUNCTION();
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "CommandList must be open");

		// Store buffer in command list state for execution phase
		auto it = s_GLCommandLists.find(cmdList.id);
		if (it != s_GLCommandLists.end()) {
			it->second.vertexBuffer = buffer;
			it->second.vertexOffset = offset;
		}
	}

	void GLCmdSetIndexBuffer(CommandList& cmdList, BufferHandle buffer, uint64_t offset) {
		PROFILE_FUNCTION();
		RENDERX_ASSERT_MSG(cmdList.isOpen, "CommandList must be open");

		// Store buffer in command list state for execution phase
		auto it = s_GLCommandLists.find(cmdList.id);
		if (it != s_GLCommandLists.end()) {
			it->second.indexBuffer = buffer;
			it->second.indexOffset = offset;
		}
	}

	void GLCmdWriteBuffer(const CommandList& cmdList, BufferHandle handle, void* data, uint32_t offset, uint32_t size) {
		return;
	}

} // namespace RxGL

} // namespace Rx
