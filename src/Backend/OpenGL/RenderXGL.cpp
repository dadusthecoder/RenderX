#include "CommonGL.h"

namespace RenderX {

namespace RenderXGL {

	GLFWwindow* s_Window = nullptr;

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

	void GLInit(const Window& window) {
		PROFILE_FUNCTION();
		if(!window.nativeHandle)
		  CreateContext_OpenGL(800, 800, false);
		else 
		  s_Window = reinterpret_cast<GLFWwindow*>(window.nativeHandle);  

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

} // namespace RenderXGL

} // namespace RenderX
