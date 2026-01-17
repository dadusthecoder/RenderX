#include "RenderX/RenderX.h"
#include "Files.h"
#include <iostream>

int main() {
	RenderX::Init();
	RenderX::SetBackend(RenderX::GraphicsAPI::Vulkan);

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f
	};

	auto vertexBuffer = RenderX::CreateBuffer(RenderX::BufferDesc(
		RenderX::BufferType::Vertex,
		sizeof(vertices),
		RenderX::BufferUsage::Dynamic,
		vertices));

	// Create shaders
	std::vector<uint8_t> vertexShaderSrc = Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.vert.spv");
	std::vector<uint8_t> fragmentShaderSrc = Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.frag.spv");

	auto vertexShader = RenderX::CreateShader(RenderX::ShaderDesc(
		RenderX::ShaderType::Vertex,
		vertexShaderSrc));

	auto fragmentShader = RenderX::CreateShader(RenderX::ShaderDesc(
		RenderX::ShaderType::Fragment,
		fragmentShaderSrc));

	// Create pipeline

	std::vector<RenderX::ShaderHandle> shaders = { vertexShader, fragmentShader };

	RenderX::PipelineDesc pipelineDesc;
	pipelineDesc.shaders = shaders;
	auto pipeline = RenderX::CreateGraphicsPipeline(pipelineDesc);

	RenderX::CommandList cmdList = RenderX::CreateCommandList();
	while (!RenderX::ShouldClose()) {
		cmdList.begin();
		cmdList.setPipeline(pipeline);
		cmdList.setVertexBuffer(vertexBuffer);
		cmdList.draw(3); // Draw 3 vertices (triangle)
		cmdList.end();
		RenderX::ExecuteCommandList(cmdList);
	}

	RenderX::DestroyCommandList(cmdList);
	RenderX::ShutDown();
	return 0;
}
