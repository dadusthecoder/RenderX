#pragma once
#include "RenderX/Log.h"
#include "ProLog/ProLog.h"
#include "GL_RenderX.h"

#include <GL/glew.h>
#include <windows.h>

#include <vector>
#include <array>
#include <optional>
#include <unordered_map>
#include <limits>
#include <cstdint>
#include <cstring>
#include <string>

// Small helpers specific to the OpenGL backend
namespace Rx {
namespace RxGL {


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

	extern HWND s_WindowHandle;
	extern int s_WindowWidth;
	extern int s_WindowHeight;
	extern HDC s_DeviceContext;
	extern HGLRC s_RenderContext;
	extern std::unordered_map<uint32_t, GLCommandListState> s_GLCommandLists;
	extern uint32_t s_NextGLCmdId;

	// Translate RenderX::DataFormat into OpenGL vertex attribute parameters.
	inline void GLGetVertexFormat(DataFormat fmt,
		GLint& components,
		GLenum& type,
		GLboolean& normalized) {
		normalized = GL_FALSE;
		switch (fmt) {
		case DataFormat::R8:
			components = 1;
			type = GL_UNSIGNED_BYTE;
			normalized = GL_TRUE;
			break;
		case DataFormat::RG8:
			components = 2;
			type = GL_UNSIGNED_BYTE;
			normalized = GL_TRUE;
			break;
		case DataFormat::RGB8:
			components = 3;
			type = GL_UNSIGNED_BYTE;
			normalized = GL_TRUE;
			break;
		case DataFormat::RGBA8:
			components = 4;
			type = GL_UNSIGNED_BYTE;
			normalized = GL_TRUE;
			break;

		case DataFormat::R16F:
			components = 1;
			type = GL_HALF_FLOAT;
			break;
		case DataFormat::RG16F:
			components = 2;
			type = GL_HALF_FLOAT;
			break;
		case DataFormat::RGB16F:
			components = 3;
			type = GL_HALF_FLOAT;
			break;
		case DataFormat::RGBA16F:
			components = 4;
			type = GL_HALF_FLOAT;
			break;

		case DataFormat::R32F:
			components = 1;
			type = GL_FLOAT;
			break;
		case DataFormat::RG32F:
			components = 2;
			type = GL_FLOAT;
			break;
		case DataFormat::RGB32F:
			components = 3;
			type = GL_FLOAT;
			break;
		case DataFormat::RGBA32F:
			components = 4;
			type = GL_FLOAT;
			break;

		default:
			components = 4;
			type = GL_FLOAT;
			break;
		}
	}

	// Forward declaration for render pass clear value support
	const std::vector<ClearValue>& GLGetPendingClearValues();

} // namespace RxGL
} // namespace Rx
