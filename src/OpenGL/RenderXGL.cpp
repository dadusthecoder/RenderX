#include "CommonGL.h"
#include <windows.h>

namespace Rx {

namespace RxGL {

	HWND s_WindowHandle = nullptr;
	HDC s_DeviceContext = nullptr;
	HGLRC s_RenderContext = nullptr;

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

		// Set pixel format
		PIXELFORMATDESCRIPTOR pfd{};
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;

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

		// Create OpenGL context
		s_RenderContext = wglCreateContext(s_DeviceContext);
		if (!s_RenderContext) {
			RENDERX_CRITICAL("GLInit: Failed to create OpenGL context");
			ReleaseDC(s_WindowHandle, s_DeviceContext);
			s_DeviceContext = nullptr;
			return;
		}

		// Make context current
		if (!wglMakeCurrent(s_DeviceContext, s_RenderContext)) {
			RENDERX_CRITICAL("GLInit: Failed to make OpenGL context current");
			wglDeleteContext(s_RenderContext);
			ReleaseDC(s_WindowHandle, s_DeviceContext);
			s_RenderContext = nullptr;
			s_DeviceContext = nullptr;
			s_WindowHandle = nullptr;
			return;
		}

		// Initialize GLEW after context is current
		glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK) {
			RENDERX_CRITICAL("GLInit: Failed to initialize GLEW");
			wglMakeCurrent(nullptr, nullptr);
			wglDeleteContext(s_RenderContext);
			ReleaseDC(s_WindowHandle, s_DeviceContext);
			s_RenderContext = nullptr;
			s_DeviceContext = nullptr;
			s_WindowHandle = nullptr;
			return;
		}

		// Load WGL extensions
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
		if (wglSwapIntervalEXT) {
			wglSwapIntervalEXT(1);  // Enable vsync
		}

		// Set OpenGL state
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		RENDERX_INFO("OpenGL initialized successfully (native Win32 context, GLEW configured)");
	}

	void GLShutdown() {
		PROFILE_FUNCTION();

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
		RENDERX_INFO("OpenGL shutdown complete");
	}

} // namespace RxGL

} // namespace Rx
