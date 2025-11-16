#include <GL/glew.h>
#include "RenderXGL.h"
#include "RenderX/Log.h"
#include <glfw/glfw3.h>

namespace Lgt {
    
    static GLFWwindow* window = nullptr;

    void CreateContext_OpenGL(int width, int height, bool fullscreen) {
        
        if (!glfwInit()) {
            RENDERX_CRITICAL("Failed to initialize GLFW!");
            return;     
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);  
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        window = glfwCreateWindow(width, height, "RenderX",nullptr, nullptr);
        if (!window) {
            RENDERX_CRITICAL("Failed to create GLFW window!");
            glfwTerminate();
            return;
        }
        glfwMakeContextCurrent(window);
    }

	void RenderXGL::GLInit() {
		CreateContext_OpenGL(800, 800, false);

		if (glewInit() != GLEW_OK) {
			RENDERX_CRITICAL("Failed to initialize GLEW!");
			return;
		}

 		//glEnable(GL_CULL_FACE);
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glEnable(GL_DEPTH_TEST);
		RENDERX_INFO("OpenGL initialized successfully (GLEW + GL states configured).");
	}

	const void RenderXGL::GLBegin() {
		// Clear the screen
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	const void RenderXGL::GLEnd() {
        // Swap buffers (assuming double buffering)
        glfwSwapBuffers(window);
        glfwPollEvents();
        if(glfwWindowShouldClose(window)) {
            glfwTerminate();
            RENDERX_INFO("GLFW window closed, terminating application.");
            exit(0);
        }
	}

    const void RenderXGL::GLDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
        glDrawArraysInstanced(GL_TRIANGLES,firstVertex,vertexCount,instanceCount);
    }   

    const void RenderXGL::GLDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
        glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT,(void*)(firstIndex * sizeof(GLuint)), instanceCount);
    }

}
