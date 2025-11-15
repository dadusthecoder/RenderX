#include "RenderX/RenderX.h"

#include <iostream>

int main() {
	Lgt::LoadAPI(Lgt::RenderXAPI::OpenGL);

	int Traingle[] = {
		0, 1, 0,
		-1, -1, 0,
		1, -1, 0
	};

	Lgt::VertexAttribute posAttr(0, 3, Lgt::TextureFormat::RGB8, 0);
	Lgt::VertexBinding binding(0, sizeof(float) * 3, false);
	auto Layout = Lgt::VertexLayout();
	Layout.attributes.push_back(posAttr);
	Layout.bindings.push_back(binding);
	auto layoutHandle = Lgt::CreateVertexLayout(Layout);
	Lgt::BindLayout(layoutHandle);
	auto vertexBuffer = Lgt::CreateVertexBuffer(sizeof(Traingle), Traingle, Lgt::BufferUsage::Static);

	std::string vertSrc = R"(
		#version 330 core
		layout(location = 0) in vec3 aPos;
		void main() {
			//gl_Position = vec4(aPos, 1.0);
		}
	)";
	std::string fragSrc = R"(
		#version 330 core
		out vec4 FragColor;
		void main() {
			FragColor = vec4(1.0, 0.5, 0.2, 1.0);
		}
	)";
	auto pipeline = Lgt::CreatePipelineGFX(vertSrc, fragSrc);

	while (true) {
		Lgt::Begin();
		Lgt::BindPipeline(pipeline);
		Lgt::BindVertexBuffer(vertexBuffer);
		Lgt::Draw(3, 1, 0, 0);
		Lgt::End();
	}
	return 0;
}