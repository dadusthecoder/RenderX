#include "CommonGL.h"

namespace Rx {
namespace RxGL {
	 std::unordered_map<uint32_t, GLCommandListState> s_GLCommandLists;
	 uint32_t s_NextGLCmdId = 1;
    static std::unordered_map<uint64_t, GLuint> s_VAOCache;

	uint32_t GLBegin() {
		// Clear the screen
		PROFILE_FUNCTION();
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return 0;
	}

	void GLEnd(uint32_t frame) {
		PROFILE_FUNCTION();

		// Swap buffers (assuming double buffering)
		if (s_DeviceContext) {
			SwapBuffers(s_DeviceContext);
		}
		// Note: Input polling is now the responsibility of the application
	}

	bool GLShouldClose() {
		return s_WindowHandle ? false : true;
	}

	CommandList GLCreateCommandList() {
		PROFILE_FUNCTION();
		CommandList cmd;
		cmd.id = s_NextGLCmdId++;
		s_GLCommandLists[cmd.id] = GLCommandListState{};
		return cmd;
	}

	void GLDestroyCommandList(CommandList& cmdList) {
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
			RENDERX_ASSERT(false && "Base-vertex draw call not available (requires OpenGL >= 3.2 or ARB extension)");
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
		if (!s_WindowHandle)
			return;



		auto it = s_GLCommandLists.find(cmdList.id);
		if (it != s_GLCommandLists.end()) {
			const auto& st = it->second;

			// Bind pipeline and fixed-function state
			if (st.pipeline.IsValid()) {
				GLBindPipeline(st.pipeline);
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
							for (const auto& b : pd->vertexInputState.bindings) {
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
						}
					}
					// Store in cache
					s_VAOCache.emplace(key, vao);
				}
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
		}
	}
} // namespace  RenderXGL


} // namespace  RenderX
