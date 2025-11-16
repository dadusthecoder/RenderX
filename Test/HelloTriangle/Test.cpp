#include "RenderX/RenderX.h"

#include <iostream>

int main() {
	Lgt::LoadAPI(Lgt::RenderXAPI::OpenGL);

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f
	};

	Lgt::VertexAttribute posAttr(0, 3,Lgt::DataType::Float, 0);
	Lgt::VertexBinding binding(0, sizeof(float) * 3, false);
	auto Layout = Lgt::VertexLayout();
	Layout.totalStride = sizeof(float) * 3;
	Layout.attributes.push_back(posAttr);
	Layout.bindings.push_back(binding);

	auto VAO = Lgt::CreateVertexArray();
	auto vertexBuffer = Lgt::CreateVertexBuffer(sizeof(vertices), (void*)vertices, Lgt::BufferUsage::Static);
	Lgt::CreateVertexLayout(Layout);

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

	auto pipeline = Lgt::CreatePipelineGFX(vertSrc, fragSrc);

	while (true) {
		Lgt::Begin();
		Lgt::BindPipeline(pipeline);		
		Lgt::Draw(3, 1, 0, 0);
		Lgt::End();
	}
	return 0;
}