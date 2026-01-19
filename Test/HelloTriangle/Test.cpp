#include "RenderX/RenderX.h"
#include "Files.h"

int main() {
	RenderX::Init();
#if 1
	RenderX::SetBackend(RenderX::GraphicsAPI::Vulkan);

	auto vertexShader = RenderX::CreateShader({ RenderX::ShaderType::Vertex,
		Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.vert.spv ") });

	auto fragmentShader = RenderX::CreateShader({ RenderX::ShaderType::Fragment,
		Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.frag.spv") });
#else
	RenderX::SetBackend(RenderX::GraphicsAPI::OpenGL);

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

	RenderX::RenderPassDesc renderPassDesc{};
	RenderX::AttachmentDesc colorAttach;
	colorAttach.format = RenderX::TextureFormat::BGRA8_SRGB;
	colorAttach.finalState = RenderX::ResourceState::Present;
	renderPassDesc.colorAttachments.push_back(colorAttach);
    
	auto renderPass = RenderX::CreateRenderPass(renderPassDesc);

	RenderX::PipelineDesc pipelineDesc{};
	pipelineDesc.shaders = { vertexShader, fragmentShader };
	pipelineDesc.vertexInputState = vertexInputState;
	pipelineDesc.renderPass = renderPass;
	// For now, disable back-face culling so both Vulkan and OpenGL show the quad
	// regardless of winding differences.
	pipelineDesc.rasterizer.cullMode = RenderX::CullMode::None;
	pipelineDesc.rasterizer.frontCounterClockwise = true;

	auto pipeline = RenderX::CreateGraphicsPipeline(pipelineDesc);

	std::vector<RenderX::ClearValue> clearValues;
	RenderX::ClearValue clear{};
	clear.color = RenderX::ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	clearValues.push_back(clear);

	while (!RenderX::ShouldClose()) {
		RenderX::Begin();

		auto cmd = RenderX::CreateCommandList();
		cmd.open();
		cmd.beginRenderPass(renderPass, clearValues);
		cmd.setPipeline(pipeline);
		cmd.setVertexBuffer(vertexBuffer);
		cmd.setIndexBuffer(indexBuffer);
		cmd.drawIndexed(6);
		cmd.endRenderPass();
		cmd.close();
		RenderX::ExecuteCommandList(cmd);

		RenderX::End();
	}

	RenderX::ShutDown();
	return 0;
}
