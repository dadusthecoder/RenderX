#include "RenderX/RenderX.h"
#include "Files.h"
#include "RenderX/Log.h"
#include "Windows.h"
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

int main() {
	if (!glfwInit()) {
		// RENDERX_ERROR("Failed to initialize GLFW");
		return 0;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	auto* window = glfwCreateWindow(1280, 720, "RenderX", nullptr, nullptr);
	int fbWidth = 0;
	int fbHeight = 0;

	while (fbWidth == 0 || fbHeight == 0) {
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
		glfwPollEvents();
	}

	RenderX::Window Window;
	Window.api = RenderX::GraphicsAPI::Vulkan;
	Window.platform = RenderX::Platform::Windows;
	Window.instanceExtensions = glfwGetRequiredInstanceExtensions(&Window.extensionCount);
	Window.height = 720;
	Window.width = 1280;
	Window.nativeHandle = glfwGetWin32Window(window);
	Window.displayHandle = GetModuleHandle(nullptr);
	RenderX::Init(Window);

#if 1
	auto vertexShader = RenderX::CreateShader({ RenderX::ShaderType::Vertex,
		Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.vert.spv ") });

	auto fragmentShader = RenderX::CreateShader({ RenderX::ShaderType::Fragment,
		Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.frag.spv") });
#else

	auto vertexShader = RenderX::CreateShader({ RenderX::ShaderType::Vertex,
		Files::ReadTextFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.vert") });

	auto fragmentShader = RenderX::CreateShader({ RenderX::ShaderType::Fragment,
		Files::ReadTextFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.frag") });
#endif
	const float vertices[] = {
		-0.5f, -0.5f, 0.0f, // 0 bottom-left
		0.5f, -0.5f, 0.0f,	// 1 bottom-right
		0.5f, 0.5f, 0.0f,	// 2 top-right
		-0.5f, 0.5f, 0.0f	// 3 top-left
	};
	const uint32_t indices[] = {
		0, 1, 2, // first triangle
		2, 3, 0	 // second triangle
	};


	auto indexBuffer = RenderX::CreateBuffer({ RenderX::BufferType::Index,
		RenderX::BufferUsage::Dynamic,
		sizeof(indices),
		1,
		indices });
	auto vertexBuffer = RenderX::CreateBuffer({ RenderX::BufferType::Vertex,
		RenderX::BufferUsage::Dynamic,
		sizeof(vertices),
		1,
		vertices });

	RenderX::VertexInputState vertexInputState;
	vertexInputState.attributes.emplace_back(
		0, // location
		0, // binding
		RenderX::DataFormat::RGB32F,
		0);

	vertexInputState.bindings.emplace_back(0, sizeof(float) * 3, false);

	RenderX::PipelineDesc pipelineDesc{};
	pipelineDesc.shaders = { vertexShader, fragmentShader };
	pipelineDesc.vertexInputState = vertexInputState;
	pipelineDesc.renderPass = RenderX::GetDefaultRenderPass();
	pipelineDesc.rasterizer.cullMode = RenderX::CullMode::None;
	pipelineDesc.rasterizer.frontCounterClockwise = true;
	pipelineDesc.blend.enable = true;

	pipelineDesc.depthStencil = RenderX::DepthStencilState();

	auto pipeline = RenderX::CreateGraphicsPipeline(pipelineDesc);

	std::vector<RenderX::ClearValue> clearValues;
	RenderX::ClearValue clear{};
	clear.color = RenderX::ClearColor(0.10f, 0.0f, 0.30f, 1.0f);
	clearValues.push_back(clear);

	while (!glfwWindowShouldClose(window)) {
		RenderX::Begin();
		auto cmd = RenderX::CreateCommandList();
		cmd.open();
<<<<<<< HEAD
		cmd.beginRenderPass(renderPass, clearValues.data(), clearValues.size());
=======
		cmd.beginRenderPass(RenderX::GetDefaultRenderPass(), clearValues.data(),
			(uint32_t)clearValues.size());
>>>>>>> 9972efc528dafc56ed23303a4428e8e781ff3b97
		cmd.setPipeline(pipeline);
		cmd.setVertexBuffer(vertexBuffer);
		cmd.setIndexBuffer(indexBuffer);
		cmd.drawIndexed(6);
		cmd.endRenderPass();
		cmd.close();
		RenderX::ExecuteCommandList(cmd);
		RenderX::End();
		glfwPollEvents();
	}

	RenderX::ShutDown();
	return 0;
}
