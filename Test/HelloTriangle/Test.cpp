#include "RenderX/RenderX.h"
#include "Files.h"

int main() {
	RenderX::Init();
	RenderX::SetBackend(RenderX::GraphicsAPI::Vulkan);

	// ---------------------------------------------------------------------
	// Shaders
	// ---------------------------------------------------------------------
	auto vertexShader = RenderX::CreateShader({ RenderX::ShaderType::Vertex,
		Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.vert.spv ") });

	auto fragmentShader = RenderX::CreateShader({ RenderX::ShaderType::Fragment,
		Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.frag.spv") });

	// ---------------------------------------------------------------------
	// Vertex Data
	// ---------------------------------------------------------------------
	const float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f
	};

	auto vertexBuffer = RenderX::CreateBuffer({ RenderX::BufferType::Vertex,
		RenderX::BufferUsage::Dynamic,
		sizeof(vertices),
		1,
		vertices });

	// ---------------------------------------------------------------------
	// Vertex Layout
	// ---------------------------------------------------------------------
	RenderX::VertexLayout layout;

	layout.attributes.emplace_back(
		0, // location
		0, // binding
		RenderX::DataFormat::RGB32F,
		0);

	layout.bindings.emplace_back(
		0,
		sizeof(float) * 3,
		false);

	// ---------------------------------------------------------------------
	// Render Pass
	// ---------------------------------------------------------------------
	RenderX::RenderPassDesc renderPassDesc{};
	RenderX::AttachmentDesc colorAttach;
	colorAttach.format = RenderX::TextureFormat::BGRA8;
	colorAttach.finalState = RenderX::ResourceState::Present;
	renderPassDesc.colorAttachments.push_back(colorAttach);

	auto renderPass = RenderX::CreateRenderPass(renderPassDesc);

	// ---------------------------------------------------------------------
	// Pipeline
	// ---------------------------------------------------------------------
	RenderX::PipelineDesc pipelineDesc{};
	pipelineDesc.shaders = { vertexShader, fragmentShader };
	pipelineDesc.vertexLayout = layout;
	pipelineDesc.renderPass = renderPass;

	auto pipeline = RenderX::CreateGraphicsPipeline(pipelineDesc);

	// ---------------------------------------------------------------------
	// Clear Values
	// ---------------------------------------------------------------------
	std::vector<RenderX::ClearValue> clearValues;
	RenderX::ClearValue clear{};
	clear.color = RenderX::ClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	clearValues.push_back(clear);

	// ---------------------------------------------------------------------
	// Main Loop
	// ---------------------------------------------------------------------
	while (!RenderX::ShouldClose()) {
		RenderX::Begin();

		auto cmd = RenderX::CreateCommandList();
		cmd.open();
		cmd.beginRenderPass(renderPass, clearValues);
		cmd.setPipeline(pipeline);
		cmd.setVertexBuffer(vertexBuffer);
		cmd.draw(3);
		cmd.endRenderPass();

		cmd.close();
		RenderX::ExecuteCommandList(cmd);

		RenderX::End();
	}

	RenderX::ShutDown();
	return 0;
}
