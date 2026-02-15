#include "GL_Common.h"

namespace Rx {
namespace RxGL {
	 std::unordered_map<uint32_t, GLCommandListState> s_GLCommandLists;
	 uint32_t s_NextGLCmdId = 1;
    static std::unordered_map<uint64_t, GLuint> s_VAOCache;
	void GLClearVAOCache() {
		PROFILE_FUNCTION();
		// Delete all cached VAOs and clear the map
		for (const auto& pair : s_VAOCache) {
			glDeleteVertexArrays(1, &pair.second);
		}
		s_VAOCache.clear();
	}
	void GLBegin(uint32_t frameIndex ) {
		PROFILE_FUNCTION();
		
		// Set viewport and scissor to match window dimensions
		if (s_WindowWidth > 0 && s_WindowHeight > 0) {
			glViewport(0, 0, s_WindowWidth, s_WindowHeight);
			glScissor(0, 0, s_WindowWidth, s_WindowHeight);
		}
		
		// minimal runtime: do not log every frame begin
	}

	void GLEnd(uint32_t frame) {
		PROFILE_FUNCTION();

		// Swap buffers (assuming double buffering)
		if (s_DeviceContext) {
			SwapBuffers(s_DeviceContext);
		} else {
			RENDERX_ERROR("GLEnd: No device context!");
		}
		// Note: Input polling is now the responsibility of the application
	}

	 CommandList GLCreateCommandList(uint32_t frameIndex ) {
		PROFILE_FUNCTION();
		CommandList cmd;
		cmd.id = s_NextGLCmdId++;
		s_GLCommandLists[cmd.id] = GLCommandListState{};
		return cmd;
	}

	void GLDestroyCommandList(CommandList& cmdList , uint32_t frameIndex) {
		PROFILE_FUNCTION();
		s_GLCommandLists.erase(cmdList.id);
	}

	// Recording operations --------------------------------------------------

	void GLCmdOpen(CommandList& cmd) {
		PROFILE_FUNCTION();
		auto it = s_GLCommandLists.find(cmd.id);
		if (it == s_GLCommandLists.end()) return;
		it->second.vertexCount = 0;
		it->second.indexCount = 0;
		it->second.instanceCount = 1;
		cmd.isOpen = true;
	}

	void GLCmdClose(CommandList& /*cmd*/) {
		PROFILE_FUNCTION();
		// No-op for now; execution happens in GLExecuteCommandList
	}

	void GLCmdSetPipeline(CommandList& cmd, const PipelineHandle pipeline) {
		auto it = s_GLCommandLists.find(cmd.id);
		if (it == s_GLCommandLists.end()) return;
		it->second.pipeline = pipeline;
	}

	void GLCmdDraw(CommandList& cmd,
		uint32_t vertexCount, uint32_t instanceCount,
		uint32_t firstVertex, uint32_t firstInstance) {
		auto it = s_GLCommandLists.find(cmd.id);
		if (it == s_GLCommandLists.end()) return;
		auto& st = it->second;
		st.vertexCount = vertexCount;
		st.instanceCount = instanceCount;
		st.firstVertex = firstVertex;
		st.firstInstance = firstInstance;
		st.indexCount = 0; // non-indexed draw
	}

	void GLCmdDrawIndexed(CommandList& cmd,
		uint32_t indexCount, int32_t vertexOffset,
		uint32_t instanceCount, uint32_t firstIndex, uint32_t firstInstance) {
		auto it = s_GLCommandLists.find(cmd.id);
		if (it == s_GLCommandLists.end()) return;
		auto& st = it->second;
		st.indexCount = indexCount;
		st.vertexOffsetIdx = vertexOffset;
		st.instanceCount = instanceCount;
		st.firstIndex = firstIndex;
		st.firstInstance = firstInstance;
	}


	void GLDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
		PROFILE_FUNCTION();
		(void)firstInstance; // Unused for now
		glDrawArraysInstanced(GL_TRIANGLES, static_cast<GLint>(firstVertex), static_cast<GLsizei>(vertexCount), static_cast<GLsizei>(instanceCount));
	}

	void GLDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
		PROFILE_FUNCTION();
		(void)firstInstance; // Unused for now



		// Prefer the base-vertex-aware entry if available so vertexOffset is respected
		if (glDrawElementsInstancedBaseVertex) {
			glDrawElementsInstancedBaseVertex(
				GL_TRIANGLES,
				static_cast<GLsizei>(indexCount),
				GL_UNSIGNED_INT,
				reinterpret_cast<void*>(static_cast<uintptr_t>(firstIndex) * sizeof(GLuint)),
				static_cast<GLsizei>(instanceCount),
				vertexOffset);
		}
		else {
			// Fallback to non-base-vertex call (may render incorrectly if vertexOffset != 0)
			glDrawElementsInstanced(
				GL_TRIANGLES,
				static_cast<GLsizei>(indexCount),
				GL_UNSIGNED_INT,
				reinterpret_cast<void*>(static_cast<uintptr_t>(firstIndex) * sizeof(GLuint)),
				static_cast<GLsizei>(instanceCount));
		}
	}

	void GLExecuteCommandList(CommandList& cmdList) {
		PROFILE_FUNCTION();
		if (!s_WindowHandle) {
			RENDERX_WARN("GLExecuteCommandList: No window handle");
			return;
		}

		auto it = s_GLCommandLists.find(cmdList.id);
		if (it == s_GLCommandLists.end()) {
			RENDERX_WARN("GLExecuteCommandList: Command list {} not found", cmdList.id);
			return;
		}

		const auto& st = it->second;


		// Bind pipeline and fixed-function state
		if (st.pipeline.IsValid()) {
			GLBindPipeline(st.pipeline);
		}
		else {
			RENDERX_WARN("GLExecuteCommandList: Invalid pipeline");
			return;
		}

		// Bind vertex buffer
		if (st.vertexBuffer.IsValid()) {
			GLuint vbo = static_cast<GLuint>(st.vertexBuffer.id);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			// Setup or bind a cached VAO for the current pipeline+VBO combination
			uint64_t key = (static_cast<uint64_t>(st.pipeline.id) << 32) ^ static_cast<uint64_t>(st.vertexBuffer.id);
			auto vaoIt = s_VAOCache.find(key);
			if (vaoIt != s_VAOCache.end()) {
				glBindVertexArray(vaoIt->second);
			}
			else {
				// Create and configure a VAO based on the pipeline's vertex input state
				GLuint vao = 0;
				glGenVertexArrays(1, &vao);
				glBindVertexArray(vao);
				const PipelineDesc* pd = GLGetPipelineDesc(st.pipeline);
				if (pd) {
					// For each attribute described in the pipeline
					for (const auto& attr : pd->vertexInputState.attributes) {
						GLint components = 0;
						GLenum type = GL_FLOAT;
						GLboolean normalized = GL_FALSE;
						GLGetVertexFormat(attr.datatype, components, type, normalized);
						// Find corresponding binding to get stride and instance divisor
						GLuint stride = 0;
						bool instance = false;
						for (const auto& b : pd->vertexInputState.vertexBindings) {
							if (b.binding == attr.binding) {
								stride = b.stride;
								instance = b.instanceData;
								break;
							}
						}
						glEnableVertexAttribArray(attr.location);
						if (type == GL_FLOAT || type == GL_HALF_FLOAT) {
							glVertexAttribPointer(attr.location, components, type, normalized, stride, reinterpret_cast<void*>(static_cast<uintptr_t>(attr.offset)));
						}
						else {
							// Integer formats
							glVertexAttribIPointer(attr.location, components, type, stride, reinterpret_cast<void*>(static_cast<uintptr_t>(attr.offset)));
						}
						if (instance) {
							glVertexAttribDivisor(attr.location, 1);
						}
						// attribute setup complete
						// Diagnostic: verify attribute state recorded in GL
						// optional attribute verification removed to reduce log noise
					}

						// Bind index buffer while VAO is bound so the VAO records the element-array binding
						if (st.indexBuffer.IsValid()) {
							GLuint ibo_bind = static_cast<GLuint>(st.indexBuffer.id);
							glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_bind);
						}
				} else {
					RENDERX_ERROR("GLExecuteCommandList: Pipeline descriptor not found!");
				}
				// Store in cache
				s_VAOCache.emplace(key, vao);
			}
		}
		else {
			RENDERX_WARN("GLExecuteCommandList: Invalid vertex buffer");
			return;
		}

		// Issue draw
		if (st.indexCount > 0 && st.indexBuffer.IsValid()) {
			GLuint ibo = static_cast<GLuint>(st.indexBuffer.id);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			GLDrawIndexed(st.indexCount,
				st.instanceCount,
				st.firstIndex,
				st.vertexOffsetIdx,
				st.firstInstance);
		}
		else if (st.vertexCount > 0) {
			GLDraw(st.vertexCount,
				st.instanceCount,
				st.firstVertex,
				st.firstInstance);
		}
		else {
			RENDERX_WARN("GLExecuteCommandList: No draw data (indexCount={}, vertexCount={})", st.indexCount, st.vertexCount);
		}
	}
} // namespace  RenderXGL


} // namespace  RenderX
