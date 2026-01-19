#include "CommonGL.h"

namespace RenderX {
namespace RenderXGL {
	 std::unordered_map<uint32_t, GLCommandListState> s_GLCommandLists;
	 uint32_t s_NextGLCmdId = 1;

	void GLBegin() {
		// Clear the screen
		PROFILE_FUNCTION();
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void GLEnd() {
		PROFILE_FUNCTION();

		// Swap buffers (assuming double buffering)
		glfwSwapBuffers(s_Window);
		glfwPollEvents();
	}

	bool GLShouldClose() {
		return s_Window ? glfwWindowShouldClose(s_Window) != 0 : true;
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
		(void)vertexOffset;
		(void)firstInstance; // Unused for now
		glDrawElementsInstanced(
			GL_TRIANGLES,
			static_cast<GLsizei>(indexCount),
			GL_UNSIGNED_INT,
			reinterpret_cast<void*>(static_cast<uintptr_t>(firstIndex) * sizeof(GLuint)),
			static_cast<GLsizei>(instanceCount));
	}

	void GLExecuteCommandList(CommandList& cmdList) {
		PROFILE_FUNCTION();
		if (!s_Window)
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
				// NOTE: Vertex attribute layout setup is backend-specific and
				// uses PipelineDesc::vertexLayout; you can wire that up next.
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
