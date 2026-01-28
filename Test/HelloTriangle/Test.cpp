#include "RenderX/RenderX.h"
#include "Files.h"
#include "RenderX/Log.h"
#include "Windows.h"
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

int main() {
	using namespace Rx;

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

	Window Window;
	Window.api = GraphicsAPI::Vulkan;
	Window.platform = Platform::Windows;
	Window.instanceExtensions = glfwGetRequiredInstanceExtensions(&Window.extensionCount);
	Window.height = 720;
	Window.width = 1280;
	Window.nativeHandle = glfwGetWin32Window(window);
	Window.displayHandle = GetModuleHandle(nullptr);
	Init(Window);

#if 1
	auto vertexShader = CreateShader({ ShaderStage::Vertex,
		Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.vert.spv") });

	auto fragmentShader = CreateShader({ ShaderStage::Fragment,
		Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.frag.spv") });
#else

	auto vertexShader = CreateShader({ ShaderStage::Vertex,
		Files::ReadTextFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.vert") });

	auto fragmentShader = CreateShader({ ShaderStage::Fragment,
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


	auto indexBuffer = CreateBuffer({ BufferType::Index,
		BufferUsage::Dynamic,
		sizeof(indices),
		1,
		indices });
	auto vertexBuffer = CreateBuffer({ BufferType::Vertex,
		BufferUsage::Dynamic,
		sizeof(vertices),
		1,
		vertices });

	VertexInputState vertexInputState;
	vertexInputState.attributes.push_back({ 0, // location
		0,									   // binding
		DataFormat::RGB32F,
		0 });

	vertexInputState.bindings.push_back({ 0, sizeof(float) * 3, false });

	ResourceGroupLayoutDesc srgLayout{};
	srgLayout.debugName = "Pipeline Layout";
	srgLayout.Resources = {
		ResourceGroupLayoutItem::ConstantBuffer(0, ShaderStage::Vertex)
	};

	ResourceGroupDesc srgDesc{};
	srgDesc.debugName = "Sahder Resource Group : 1";

	PipelineDesc pipelineDesc{};
	pipelineDesc.shaders = { vertexShader, fragmentShader };
	pipelineDesc.vertexInputState = vertexInputState;
	pipelineDesc.renderPass = GetDefaultRenderPass();
	pipelineDesc.rasterizer.cullMode = CullMode::None;
	pipelineDesc.rasterizer.frontCounterClockwise = false;
	pipelineDesc.blend.enable = false;

	pipelineDesc.depthStencil = DepthStencilState();

	auto pipeline = CreateGraphicsPipeline(pipelineDesc);
    
	std::vector<ClearValue> clearValues;
	ClearValue clear{};
	clear.color = ClearColor(0.10f, 0.0f, 0.30f, 1.0f);
	clearValues.push_back(clear);
	int i = 0;
	while (!glfwWindowShouldClose(window)) {
		auto frame = Begin();
		auto cmd = CreateCommandList();
		cmd.open();

		cmd.beginRenderPass(GetDefaultRenderPass(), clearValues.data(),
			(uint32_t)clearValues.size());

		cmd.setPipeline(pipeline);
		cmd.setVertexBuffer(vertexBuffer);
		cmd.setIndexBuffer(indexBuffer);
		cmd.drawIndexed(6);
		cmd.endRenderPass();
		cmd.close();
		ExecuteCommandList(cmd);
		End(frame);
		glfwPollEvents();
		i++;
	}

	ShutDown();
	return 0;
}
