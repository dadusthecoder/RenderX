#include "RenderX/RenderX.h"
#include "ProLog/ProLog.h"
#include <iostream>

int main() {
		ProLog::ProfilerConfig config;
		config.enableProfiling = true;
		config.enableLogging = true;
		config.bufferSize = 500;
		config.autoFlush = true;
		ProLog::SetConfig(config);
        PROFILE_START_SESSION("RenderX_Api_Loading" ,"RenderX_Api_Loading.json");
	RenderX::LoadAPI(RenderX::RenderXAPI::OpenGL);

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f
	};

	RenderX::VertexAttribute posAttr(0, 3, RenderX::DataType::Float, 0);
	RenderX::VertexBinding binding(0, sizeof(float) * 3, false);
	auto Layout = RenderX::VertexLayout();
	Layout.totalStride = sizeof(float) * 3;
	Layout.attributes.push_back(posAttr);
	Layout.bindings.push_back(binding);

	auto VAO = RenderX::CreateVertexArray();
	auto vertexBuffer = RenderX::CreateVertexBuffer(sizeof(vertices), (void*)vertices, RenderX::BufferUsage::Static);
	RenderX::CreateVertexLayout(Layout);

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

	auto pipeline = RenderX::CreatePipelineGFX(vertSrc, fragSrc);
	RenderX::PipelineDesc comp;
	// auto PipelineCOMP = RenderX::CreatePipelineCOMP(comp);

	while (!RenderX::ShouldClose()) {
		RenderX::BeginFrame();
		RenderX::BindPipeline(pipeline);
		RenderX::Draw(3, 1, 0, 0);
		RenderX::EndFrame();
	}

	PROFILE_PRINT_STATS();
    PROFILE_END_SESSION();
	return 0;
}