#include "GL_Common.h"
#include <windows.h>

namespace Rx {

namespace RxGL {

	HWND s_WindowHandle = nullptr;
	HDC s_DeviceContext = nullptr;
	HGLRC s_RenderContext = nullptr;
	int s_WindowWidth = 0;
	int s_WindowHeight = 0;

	// WGL extension function pointer for vsync control
	typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);
	static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;

	void GLInit(const Window& window) {
		PROFILE_FUNCTION();

		if (!window.nativeHandle) {
			RENDERX_CRITICAL("GLInit: Native window handle is required for OpenGL backend (no GLFW fallback)");
			return;
		}

		s_WindowHandle = static_cast<HWND>(window.nativeHandle);
		s_WindowWidth = window.width;
		s_WindowHeight = window.height;
		if (!IsWindow(s_WindowHandle)) {
			RENDERX_CRITICAL("GLInit: Invalid window handle");
			return;
		}

		// Get device context from the window
		s_DeviceContext = GetDC(s_WindowHandle);
		if (!s_DeviceContext) {
			RENDERX_CRITICAL("GLInit: Failed to get device context");
			return;
		}

		// Set pixel format (can only be done once per HWND lifetime).
		// If a pixel format is already set for this window, reuse it and skip SetPixelFormat,
		// which would otherwise fail when switching backends (e.g., Vulkan -> OpenGL -> Vulkan).
		PIXELFORMATDESCRIPTOR pfd{};
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;

		int existingFormat = GetPixelFormat(s_DeviceContext);
		if (existingFormat == 0) {
			int pixelFormat = ChoosePixelFormat(s_DeviceContext, &pfd);
			if (!pixelFormat) {
				RENDERX_CRITICAL("GLInit: Failed to choose pixel format");
				ReleaseDC(s_WindowHandle, s_DeviceContext);
				s_DeviceContext = nullptr;
				return;
			}

			if (!SetPixelFormat(s_DeviceContext, pixelFormat, &pfd)) {
				RENDERX_CRITICAL("GLInit: Failed to set pixel format");
				ReleaseDC(s_WindowHandle, s_DeviceContext);
				s_DeviceContext = nullptr;
				return;
			}
		} else {
			// Reuse existing pixel format when reinitializing GL on the same window.
		}

		// Try to create a modern core profile context using WGL_ARB_create_context
		// Create a temporary context first so we can load wglCreateContextAttribsARB
		HGLRC tempContext = wglCreateContext(s_DeviceContext);
		if (!tempContext) {
			RENDERX_CRITICAL("GLInit: Failed to create temp OpenGL context");
			ReleaseDC(s_WindowHandle, s_DeviceContext);
			s_DeviceContext = nullptr;
			return;
		}

		if (!wglMakeCurrent(s_DeviceContext, tempContext)) {
			RENDERX_CRITICAL("GLInit: Failed to make temp OpenGL context current");
			wglDeleteContext(tempContext);
			ReleaseDC(s_WindowHandle, s_DeviceContext);
			s_DeviceContext = nullptr;
			return;
		}

		// Initialize GLEW so we can fetch WGL extension entrypoints
		glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK) {
			RENDERX_CRITICAL("GLInit: Failed to initialize GLEW (temp)");
			wglMakeCurrent(nullptr, nullptr);
			wglDeleteContext(tempContext);
			ReleaseDC(s_WindowHandle, s_DeviceContext);
			s_DeviceContext = nullptr;
			return;
		}

		// Try to get wglCreateContextAttribsARB
		typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
		PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
			(PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

		if (wglCreateContextAttribsARB) {
			int attribs[] = {
				0x2091 /* WGL_CONTEXT_MAJOR_VERSION_ARB */, 4,
				0x2092 /* WGL_CONTEXT_MINOR_VERSION_ARB */, 6,
				0x9126 /* WGL_CONTEXT_PROFILE_MASK_ARB */, 0x00000001 /* WGL_CONTEXT_CORE_PROFILE_BIT_ARB */,
				0
			};
			HGLRC newCtx = wglCreateContextAttribsARB(s_DeviceContext, 0, attribs);
			if (newCtx) {
				// Replace temp context with new one
				wglMakeCurrent(nullptr, nullptr);
				wglDeleteContext(tempContext);
				s_RenderContext = newCtx;
				if (!wglMakeCurrent(s_DeviceContext, s_RenderContext)) {
					RENDERX_CRITICAL("GLInit: Failed to make modern OpenGL context current");
					wglDeleteContext(s_RenderContext);
					ReleaseDC(s_WindowHandle, s_DeviceContext);
					s_RenderContext = nullptr;
					s_DeviceContext = nullptr;
					s_WindowHandle = nullptr;
					return;
				}
			}
			else {
				// fallback to temp
				s_RenderContext = tempContext;
			}
		}
		else {
			// No wglCreateContextAttribsARB available, use temp context
			s_RenderContext = tempContext;
		}

		// Ensure GLEW is initialized for the final context
		glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK) {
			RENDERX_CRITICAL("GLInit: Failed to initialize GLEW (final)");
			wglMakeCurrent(nullptr, nullptr);
			if (s_RenderContext) wglDeleteContext(s_RenderContext);
			ReleaseDC(s_WindowHandle, s_DeviceContext);
			s_RenderContext = nullptr;
			s_DeviceContext = nullptr;
			s_WindowHandle = nullptr;
			return;
		}

		// Diagnostic info: log GL strings and default framebuffer binding
		const GLubyte* vendor = glGetString(GL_VENDOR);
		const GLubyte* renderer = glGetString(GL_RENDERER);
		const GLubyte* version = glGetString(GL_VERSION);
		const GLubyte* glsl = glGetString(GL_SHADING_LANGUAGE_VERSION);
		// diagnostic info removed to reduce startup logs

		// Load WGL extensions
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
		if (wglSwapIntervalEXT) {
			wglSwapIntervalEXT(1);  // Enable vsync
		}

		// Set OpenGL state
		glDisable(GL_CULL_FACE);  // Disable culling to ensure all triangles are visible
		glDisable(GL_BLEND);  // Disable blend for now to ensure visibility
		glDisable(GL_DEPTH_TEST);  // Disable depth test to ensure geometry shows

		// Clear any existing VAO state
		glBindVertexArray(0);

		// OpenGL initialized
	}

	void GLShutdown() {
		PROFILE_FUNCTION();

		// Make the context current first to delete resources
		if (s_RenderContext && s_DeviceContext) {
			wglMakeCurrent(s_DeviceContext, s_RenderContext);
		}

		// Clear VAO cache (must happen before context is destroyed)
		GLClearVAOCache();

		// Clear pipeline cache
		GLClearPipelineCache();

		if (s_RenderContext) {
			wglMakeCurrent(nullptr, nullptr);
			wglDeleteContext(s_RenderContext);
			s_RenderContext = nullptr;
		}

		if (s_DeviceContext && s_WindowHandle) {
			ReleaseDC(s_WindowHandle, s_DeviceContext);
			s_DeviceContext = nullptr;
		}

		s_WindowHandle = nullptr;
		// OpenGL shutdown complete
	}

} // namespace RxGL

} // namespace Rx
