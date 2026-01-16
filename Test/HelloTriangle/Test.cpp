#include "RenderX/RenderX.h"
#include <iostream>

int main() {
	
	RenderX::Init();
	RenderX::SetBackend(RenderX::GraphicsAPI::Vulkan);

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f
	};

	RenderX::VertexAttribute posAttr(0, 3, RenderX::DataFormat::RGBA32F, 0);
	RenderX::VertexBinding binding(0, sizeof(float) * 3, false);

	auto vertexBuffer = RenderX::CreateBuffer(RenderX::BufferDesc(
		RenderX::BufferType::Vertex,
		sizeof(vertices),
		RenderX::BufferUsage::Static,
		vertices
	));

	std::string vertSrc = R"(
		#version 460 core
		layout(location = 0) in vec3 aPos;
        layout(location = 0) out vec3 fragPos;
		void main() {
		    fragPos = aPos;
			gl_Position = vec4(aPos, 1.0);
			}
			)";

	std::string fragSrc = R"(
				#version 460 core
				layout(location = 0) in vec3 fragPos;
				out vec4 FragColor;
				void main() {
					FragColor = vec4(fragPos, 1.0);
					}
				)";

	auto vertexShader = RenderX::CreateShader(RenderX::ShaderDesc(RenderX::ShaderType::Vertex, vertSrc));
	auto fragmentShader = RenderX::CreateShader(RenderX::ShaderDesc(RenderX::ShaderType::Fragment, fragSrc));
	RenderX::PipelineDesc pipelineDesc;
	pipelineDesc.shaders = { vertexShader, fragmentShader };
	pipelineDesc.vertexLayout.attributes.push_back(posAttr);
	pipelineDesc.vertexLayout.bindings.push_back(binding);
	RenderX::RasterizerState rasterzier;
	rasterzier.cullMode = RenderX::CullMode::Front;
	rasterzier.depthBias = 0.0f;
	rasterzier.fillMode = RenderX::FillMode::Solid;
    

	pipelineDesc.rasterizer = rasterzier;
	
	auto pipeline = RenderX::CreateGraphicsPipeline(pipelineDesc);

    RenderX::CommandList cmdList = RenderX::CreateCommandList();
    // cmdList.begin();
	// cmdList.setPipeline(pipeline);
	// cmdList.setVertexBuffer(vertexBuffer);
	// cmdList.draw(3 , 1 , 0 , 0);
	// cmdList.end();

	while (!RenderX::ShouldClose()) {
       RenderX::ExecuteCommandList(cmdList);  
	} 
    
	RenderX::ShutDown();
	return 0;
}