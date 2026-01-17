#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "RenderXGL.h"
#include "RenderX/Log.h"
#include "ProLog/ProLog.h"
#include <unordered_map>

namespace RenderX {

namespace RenderXGL {

	struct GLCommandListState {
		PipelineHandle pipeline;
		BufferHandle vertexBuffer;
		BufferHandle indexBuffer;
		uint64_t vertexOffset = 0;
		uint64_t indexOffset = 0;

		uint32_t vertexCount = 0;
		uint32_t instanceCount = 1;
		uint32_t firstVertex = 0;
		uint32_t firstInstance = 0;

		uint32_t indexCount = 0;
		int32_t vertexOffsetIdx = 0;
		uint32_t firstIndex = 0;
	};

	static GLFWwindow* s_Window = nullptr;
	static std::unordered_map<uint32_t, GLCommandListState> s_GLCommandLists;
	static uint32_t s_NextGLCmdId = 1;

	static void CreateContext_OpenGL(int width, int height, bool fullscreen) {
		PROFILE_FUNCTION();
		(void)fullscreen; // fullscreen handling can be added later

		if (!glfwInit()) {
			RENDERX_CRITICAL("Failed to initialize GLFW!");
			return;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		s_Window = glfwCreateWindow(width, height, "RenderX", nullptr, nullptr);
		if (!s_Window) {
			RENDERX_CRITICAL("Failed to create GLFW window!");
			glfwTerminate();
			return;
		}

		glfwMakeContextCurrent(s_Window);
	}

	void GLInit() {
		PROFILE_FUNCTION();
		CreateContext_OpenGL(800, 800, false);

		if (!s_Window)
			return;

		// GLEW must be initialized after an OpenGL context is current
		if (glewInit() != GLEW_OK) {
			RENDERX_CRITICAL("Failed to initialize GLEW!");
			return;
		}

		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
		RENDERX_INFO("OpenGL initialized successfully (GLEW + GL states configured).");
	}

	void GLBeginFrame() {
		// Clear the screen
		PROFILE_FUNCTION();
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void GLEndFrame() {
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

	void GLCmdBegin(CommandList& cmd) {
		PROFILE_FUNCTION();
		auto it = s_GLCommandLists.find(cmd.id);
		if (it == s_GLCommandLists.end()) return;
		it->second.vertexCount = 0;
		it->second.indexCount = 0;
		it->second.instanceCount = 1;
	}

	void GLCmdEnd(CommandList& /*cmd*/) {
		PROFILE_FUNCTION();
		// No-op for now; execution happens in GLExecuteCommandList
	}

	void GLCmdSetPipeline(CommandList& cmd, const PipelineHandle pipeline) {
		auto it = s_GLCommandLists.find(cmd.id);
		if (it == s_GLCommandLists.end()) return;
		it->second.pipeline = pipeline;
	}

	void GLCmdSetVertexBuffer(CommandList& cmd, const BufferHandle buffer, uint64_t offset) {
		auto it = s_GLCommandLists.find(cmd.id);
		if (it == s_GLCommandLists.end()) return;
		it->second.vertexBuffer = buffer;
		it->second.vertexOffset = offset;
	}

	void GLCmdSetIndexBuffer(CommandList& cmd, const BufferHandle buffer, uint64_t offset) {
		auto it = s_GLCommandLists.find(cmd.id);
		if (it == s_GLCommandLists.end()) return;
		it->second.indexBuffer = buffer;
		it->second.indexOffset = offset;
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

		GLBeginFrame();

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

		GLEndFrame();
	}


} // namespace RenderXGL

} // namespace RenderX
