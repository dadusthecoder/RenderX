#include <GL/glew.h>
#include "RenderXGL.h"
#include "RenderX/Log.h"

#ifdef _WIN32
    #include <windows.h>
    #include <GL/wglew.h>
#else
    #include <X11/Xlib.h>
    #include <GL/glx.h>
#endif

namespace Lgt{
	#ifdef _WIN32
    static HWND s_hwnd = nullptr;
    static HDC  s_hdc  = nullptr;
    static HGLRC s_hglrc = nullptr;
#else
    static Display* s_display = nullptr;
    static ::Window s_window = 0;
    static GLXContext s_glxCtx = nullptr;
#endif

    inline bool CreateContext_OpenGL(int width = 1, int height = 1, bool visible = false)
    {
#ifdef _WIN32
        WNDCLASSA wc = {};
        wc.lpfnWndProc = DefWindowProcA;
        wc.hInstance = GetModuleHandleA(nullptr);
        wc.lpszClassName = "RenderX_Client_Window";
        if (!RegisterClassA(&wc)) {
            RENDERX_ERROR("Failed to register window class");
            return false;
        }

        DWORD style = visible ? (WS_OVERLAPPEDWINDOW | WS_VISIBLE)
                              : (WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

        HWND hwnd = CreateWindowA(wc.lpszClassName, "RenderX Client",
                                  style,
                                  CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                                  nullptr, nullptr, wc.hInstance, nullptr);

        if (!hwnd) {
            RENDERX_ERROR("Failed to create Win32 window");
            return false;
        }

        HDC hdc = GetDC(hwnd);

        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        int pf = ChoosePixelFormat(hdc, &pfd);
        SetPixelFormat(hdc, pf, &pfd);

        HGLRC ctx = wglCreateContext(hdc);
        if (!ctx) {
            RENDERX_ERROR("Failed to create WGL context");
            ReleaseDC(hwnd, hdc);
            DestroyWindow(hwnd);
            return false;
        }

        if (!wglMakeCurrent(hdc, ctx)) {
            RENDERX_ERROR("Failed to make WGL context current");
            wglDeleteContext(ctx);
            ReleaseDC(hwnd, hdc);
            DestroyWindow(hwnd);
            return false;
        }

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            RENDERX_ERROR("Failed to initialize GLEW");
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(ctx);
            ReleaseDC(hwnd, hdc);
            DestroyWindow(hwnd);
            return false;
        }

        RENDERX_INFO("OpenGL context created (Windows). Version:");

        s_hwnd = hwnd;
        s_hdc = hdc;
        s_hglrc = ctx;
        return true;

#else
        Display* display = XOpenDisplay(nullptr);
        if (!display) {
            RENDERX_ERROR("Failed to open X11 display");
            return false;
        }

        int screen = DefaultScreen(display);
        static int visualAttribs[] = {
            GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None
        };

        XVisualInfo* visual = glXChooseVisual(display, screen, visualAttribs);
        if (!visual) {
            RENDERX_ERROR("Failed to choose X11 visual");
            XCloseDisplay(display);
            return false;
        }

        Colormap cmap = XCreateColormap(display, RootWindow(display, visual->screen), visual->visual, AllocNone);
        XSetWindowAttributes swa;
        swa.colormap = cmap;
        swa.event_mask = StructureNotifyMask;

        Window win = XCreateWindow(display, RootWindow(display, visual->screen),
                                   0, 0, width, height, 0,
                                   visual->depth, InputOutput, visual->visual,
                                   CWColormap | CWEventMask, &swa);

        if (visible)
            XMapWindow(display, win);

        GLXContext ctx = glXCreateContext(display, visual, nullptr, GL_TRUE);
        glXMakeCurrent(display, win, ctx);

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            RENDERX_ERROR("Failed to initialize GLEW (Linux)");
            glXDestroyContext(display, ctx);
            XDestroyWindow(display, win);
            XCloseDisplay(display);
            return false;
        }

        RENDERX_INFO("OpenGL context created (Linux). Version: %s", glGetString(GL_VERSION));

        s_display = display;
        s_window = win;
        s_glxCtx = ctx;
        return true;
#endif
    }

	// Convert engine ShaderType to OpenGL shader enums
	const static inline GLuint TOGLShaderType(ShaderType type) {
		switch (type) {
		case ShaderType::Vertex: return GL_VERTEX_SHADER;
		case ShaderType::Fragment: return GL_FRAGMENT_SHADER;
		case ShaderType::Geometry: return GL_GEOMETRY_SHADER;
		case ShaderType::Compute: return GL_COMPUTE_SHADER;
		case ShaderType::TessControl: return GL_TESS_CONTROL_SHADER;
		case ShaderType::TessEvaluation: return GL_TESS_EVALUATION_SHADER;
		default:
			RENDERX_ERROR("Unknown ShaderType: {}", static_cast<int>(type));
			return 0;
		}
	}

	// Helper: Compile a single shader
	const static inline GLuint compileShader(GLenum type, const std::string& source) {
		GLuint id = glCreateShader(type);
		const char* src = source.c_str();
		glShaderSource(id, 1, &src, nullptr);
		glCompileShader(id);

		GLint result = 0;
		glGetShaderiv(id, GL_COMPILE_STATUS, &result);

		std::string shaderTypeStr;
		switch (type) {
		case GL_VERTEX_SHADER:
			shaderTypeStr = "Vertex";
			break;
		case GL_FRAGMENT_SHADER:
			shaderTypeStr = "Fragment";
			break;
		case GL_COMPUTE_SHADER:
			shaderTypeStr = "Compute";
			break;
		default:
			shaderTypeStr = "Unknown";
			break;
		}

		if (result == GL_FALSE) {
			GLint length = 0;
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> errorLog(length);
			glGetShaderInfoLog(id, length, &length, errorLog.data());

			RENDERX_ERROR("{} shader compilation failed:\n{}", shaderTypeStr, errorLog.data());
			glDeleteShader(id);
			return 0;
		}

		RENDERX_INFO("{} shader compiled successfully (ID: {}).", shaderTypeStr, id);
		return id;
	}

	const ShaderHandle RenderXGL::GLCreateShader(const ShaderDesc& desc) {
		GLuint shader = compileShader(TOGLShaderType(desc.type), desc.source);
		if (shader == 0) {
			RENDERX_ERROR("Failed to create shader of type {}", (int)desc.type);
			return 0;
		}
		RENDERX_INFO("Shader created successfully | ID: {}", shader);
		return shader;
	}

	const PipelineHandle RenderXGL::GLCreatePipelineGFX_Src(const std::string& vertsrc,
		const std::string& fragsrc) {
		GLuint program = glCreateProgram();
		GLuint vs = compileShader(GL_VERTEX_SHADER, vertsrc);
		GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragsrc);

		if (vs == 0 || fs == 0) {
			RENDERX_ERROR("Shader compilation failed program creation aborted.");
			if (vs)
				glDeleteShader(vs);
			if (fs)
				glDeleteShader(fs);
			glDeleteProgram(program);
			return 0;
		}

		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);

		GLint success = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			GLint length;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> errorLog(length);
			glGetProgramInfoLog(program, length, &length, errorLog.data());

			RENDERX_ERROR("Shader program linking error:\n{}", errorLog.data());

			glDeleteShader(vs);
			glDeleteShader(fs);
			glDeleteProgram(program);
			return 0;
		}

		// Validate program
		glValidateProgram(program);
		glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
		if (!success)
			RENDERX_WARN("Shader program validation failed | Program ID: {}", program);

		// Cleanup shaders after linking
		glDeleteShader(vs);
		glDeleteShader(fs);

		RENDERX_INFO("Shader program linked successfully | Program ID: {}", program);
		return program;
	}

	const PipelineHandle RenderXGL::GLCreatePipelineGFX_Desc(const PipelineDesc& desc) {
		RENDERX_WARN("GLCreatePipelineGFX_Desc not yet implemented.");
		return 1;
	}

	const PipelineHandle RenderXGL::GLCreatePipelineGFX_Shader(const ShaderDesc& vertdesc,
		const ShaderDesc& fragdesc) {
		RENDERX_WARN("GLCreatePipelineGFX_Shader not yet implemented.");
		return 0;
	}

	const PipelineHandle RenderXGL::GLCreatePipelineCOMP_Desc(const PipelineDesc& desc) {
		RENDERX_WARN("GLCreatePipelineCOMP_Desc not yet implemented.");
		return 0;
	}

	const PipelineHandle RenderXGL::GLCreatePipelineCOMP_Shader(const ShaderDesc& shaderdesc) {
		RENDERX_WARN("GLCreatePipelineCOMP_Shader not yet implemented.");
		return 0;
	}

	const PipelineHandle RenderXGL::GLCreatePipelineCOMP_Src(const std::string& compsrc) {
		GLuint cs = compileShader(GL_COMPUTE_SHADER, compsrc);
		if (!cs) {
			RENDERX_ERROR("Compute shader compilation failed � cannot create program.");
			return 0;
		}

		GLuint program = glCreateProgram();
		glAttachShader(program, cs);
		glLinkProgram(program);

		GLint success = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			GLint length;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> errorLog(length);
			glGetProgramInfoLog(program, length, &length, errorLog.data());

			RENDERX_ERROR("Compute shader linking error:\n{}", errorLog.data());
			glDeleteShader(cs);
			glDeleteProgram(program);
			return 0;
		}

		glDeleteShader(cs);
		RENDERX_INFO("Compute shader pipeline created successfully | Program ID: {}", program);
		return program;
	}

	void RenderXGL::GLInit() {

		CreateContext_OpenGL(800, 800 , true);
		
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
}

#include <GL/glew.h>

