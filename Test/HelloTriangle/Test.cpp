#include "RenderX/RenderX.h"
#include "Files.h"
#include "RenderX/Log.h"
#include "Windows.h"
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

using namespace Rx;

#define MAX_FRAMES_IN_FLIGHT 3

static GraphicsAPI currentAPI = GraphicsAPI::Vulkan;

enum class RenderMode {
	Rectangle,
	Cube
};

static RenderMode g_RenderMode = RenderMode::Rectangle;

struct UBO {
	glm::mat4 mvp;
};

struct RectangleResources {
	ShaderHandle vs;
	ShaderHandle fs;
	BufferHandle vbo;
	BufferHandle ibo;
	std::vector<BufferHandle> ubo;
	std::vector<BufferViewHandle> uboView;
	PipelineLayoutHandle layout;
	PipelineHandle pipeline;
};

struct CubeResources {
	ShaderHandle vs;
	ShaderHandle fs;
	BufferHandle vbo;
	BufferHandle ibo;
	std::vector<BufferHandle> ubo;
	std::vector<BufferViewHandle> uboView;
	PipelineLayoutHandle layout;
	PipelineHandle pipeline;
};

RectangleResources CreateRectangleResources() {
	RectangleResources r{};

	if (currentAPI == GraphicsAPI::Vulkan) {
		r.vs = CreateShader({ ShaderStage::Vertex, Files::ReadBinaryFile("Shaders/bsc.vert.spv") });
		r.fs = CreateShader({ ShaderStage::Fragment, Files::ReadBinaryFile("Shaders/bsc.frag.spv") });
	}
	else {
		r.vs = CreateShader({ ShaderStage::Vertex, Files::ReadTextFile("Shaders/bsc.vert") });
		r.fs = CreateShader({ ShaderStage::Fragment, Files::ReadTextFile("Shaders/bsc.frag") });
	}

	const float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f
	};

	const uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };

	r.vbo = CreateBuffer({ BufferFlags::Vertex | BufferFlags::Static | BufferFlags::TransferDst,
		MemoryType::GpuOnly, sizeof(vertices), 1, vertices });

	r.ibo = CreateBuffer({ BufferFlags::Index | BufferFlags::Static | BufferFlags::TransferDst,
		MemoryType::GpuOnly, sizeof(indices), 1, indices });

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		r.ubo.push_back(CreateBuffer({ BufferFlags::Uniform | BufferFlags::Static | BufferFlags::TransferDst,
			MemoryType::GpuOnly, sizeof(UBO) }));
		r.uboView.push_back(CreateBufferView({ r.ubo[i], 0, sizeof(UBO) }));
	}

	ResourceGroupLayout set{};
	set.setIndex = 0;
	set.resourcebindings = { ResourceGroupLayoutItem::ConstantBuffer(0, ShaderStage::Vertex) };
	r.layout = CreatePipelineLayout(&set, 1);

	VertexInputState vi{};
	vi.vertexBindings.push_back({ 0, sizeof(float) * 3, false });
	vi.attributes.push_back({ 0, 0, DataFormat::RGB32F, 0 });

	PipelineDesc pso{};
	pso.layout = r.layout;
	pso.renderPass = GetDefaultRenderPass();
	pso.vertexInputState = vi;
	pso.shaders = { r.vs, r.fs };
	pso.rasterizer.cullMode = CullMode::None;
	pso.rasterizer.fillMode = FillMode::Solid;
	pso.depthStencil = DepthStencilState();

	r.pipeline = CreateGraphicsPipeline(pso);

	return r;
}

CubeResources CreateCubeResources() {
	CubeResources r{};

	if (currentAPI == GraphicsAPI::Vulkan) {
		r.vs = CreateShader({ ShaderStage::Vertex, Files::ReadBinaryFile("Shaders/bsc.vert.spv") });
		r.fs = CreateShader({ ShaderStage::Fragment, Files::ReadBinaryFile("Shaders/bsc.frag.spv") });
	}
	else {
		r.vs = CreateShader({ ShaderStage::Vertex, Files::ReadTextFile("Shaders/bsc.vert") });
		r.fs = CreateShader({ ShaderStage::Fragment, Files::ReadTextFile("Shaders/bsc.frag") });
	}

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 col;
	};

	const Vertex vertices[] = {
		{ { -0.5f, -0.5f, -0.5f }, { 1, 0, 0 } }, { { 0.5f, -0.5f, -0.5f }, { 0, 1, 0 } },
		{ { 0.5f, 0.5f, -0.5f }, { 0, 0, 1 } }, { { -0.5f, 0.5f, -0.5f }, { 1, 1, 0 } },
		{ { -0.5f, -0.5f, 0.5f }, { 1, 0, 1 } }, { { 0.5f, -0.5f, 0.5f }, { 0, 1, 1 } },
		{ { 0.5f, 0.5f, 0.5f }, { 1, 1, 1 } }, { { -0.5f, 0.5f, 0.5f }, { 0, 0, 0 } }
	};

	const uint32_t indices[] = {
		0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4,
		0, 4, 7, 7, 3, 0, 1, 5, 6, 6, 2, 1,
		3, 2, 6, 6, 7, 3, 0, 1, 5, 5, 4, 0
	};

	r.vbo = CreateBuffer({ BufferFlags::Vertex | BufferFlags::Static | BufferFlags::TransferDst,
		MemoryType::GpuOnly, sizeof(vertices), 1, vertices });

	r.ibo = CreateBuffer({ BufferFlags::Index | BufferFlags::Static | BufferFlags::TransferDst,
		MemoryType::GpuOnly, sizeof(indices), 1, indices });

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		r.ubo.push_back(CreateBuffer({ BufferFlags::Uniform | BufferFlags::Static | BufferFlags::TransferDst,
			MemoryType::GpuOnly, sizeof(UBO) }));
		r.uboView.push_back(CreateBufferView({ r.ubo[i], 0, sizeof(UBO) }));
	}

	ResourceGroupLayout set{};
	set.setIndex = 0;
	set.resourcebindings = { ResourceGroupLayoutItem::ConstantBuffer(0, ShaderStage::Vertex) };
	r.layout = CreatePipelineLayout(&set, 1);

	VertexInputState vi{};
	vi.vertexBindings.push_back({ 0, sizeof(Vertex), false });
	vi.attributes = {
		{ 0, 0, DataFormat::RGB32F, offsetof(Vertex, pos) },
		{ 1, 0, DataFormat::RGB32F, offsetof(Vertex, col) }
	};

	PipelineDesc pso{};
	pso.layout = r.layout;
	pso.renderPass = GetDefaultRenderPass();
	pso.vertexInputState = vi;
	pso.shaders = { r.vs, r.fs };
	pso.rasterizer.cullMode = CullMode::None;
	pso.rasterizer.fillMode = FillMode::Solid;
	pso.depthStencil = DepthStencilState();

	r.pipeline = CreateGraphicsPipeline(pso);

	return r;
}

GraphicsAPI SwitchAPI(GraphicsAPI api) {
	return api == GraphicsAPI::Vulkan ? GraphicsAPI::OpenGL : GraphicsAPI::Vulkan;
}

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "RenderX", nullptr, nullptr);

	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwPollEvents();
	}

	Window rxWindow{};
	rxWindow.api = currentAPI;
	rxWindow.platform = Platform::Windows;
	rxWindow.width = width;
	rxWindow.height = height;
	rxWindow.instanceExtensions = glfwGetRequiredInstanceExtensions(&rxWindow.extensionCount);
	rxWindow.nativeHandle = glfwGetWin32Window(window);
	rxWindow.displayHandle = GetModuleHandle(nullptr);

	Init(rxWindow);

	RectangleResources rect = CreateRectangleResources();
	CubeResources cube = CreateCubeResources();

	std::vector<CommandList> cmds;
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		cmds.push_back(CreateCommandList(i));

	std::vector<ClearValue> clears(1);
	clears[0].color = ClearColor(0.02f, 0.02f, 0.02f, 1.0f);

	uint32_t frame = 0;
	float angle = 0.0f;

	while (!glfwWindowShouldClose(window)) {
		if (GetAsyncKeyState('R') & 1)
			g_RenderMode = g_RenderMode == RenderMode::Rectangle ? RenderMode::Cube : RenderMode::Rectangle;

		if (GetAsyncKeyState('S') & 1) {
			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
				DestroyCommandList(cmds[i], i);
			cmds.clear();
			Shutdown();
			glfwDestroyWindow(window);

			currentAPI = SwitchAPI(currentAPI);

			window = glfwCreateWindow(1280, 720, "RenderX", nullptr, nullptr);
			while (width == 0 || height == 0) {
				glfwGetFramebufferSize(window, &width, &height);
				glfwPollEvents();
			}

			rxWindow.api = currentAPI;
			rxWindow.nativeHandle = glfwGetWin32Window(window);
			Init(rxWindow);

			rect = CreateRectangleResources();
			cube = CreateCubeResources();

			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
				cmds.push_back(CreateCommandList(i));

			frame = 0;
			continue;
		}

		Begin(frame);

		angle += 0.01f;

		glm::mat4 model = g_RenderMode == RenderMode::Cube
							  ? glm::rotate(glm::mat4(1.0f), angle, { 0, 1, 0 })
							  : glm::mat4(1.0f);

		glm::mat4 view = glm::lookAt(
			glm::vec3(2.0f, 2.0f, 2.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj = glm::perspective(glm::radians(60.0f),
			float(width) / float(height), 0.1f, 100.0f);
		if (currentAPI == GraphicsAPI::Vulkan)
			proj[1][1] *= -1.0f;

		UBO ubo{ proj * view * model };

		CommandList cmd = cmds[frame];
		cmd.open();

		if (g_RenderMode == RenderMode::Rectangle) {
			cmd.writeBuffer(rect.ubo[frame], &ubo, 0, sizeof(UBO));
			auto rg = CreateResourceGroup({ 0, ResourceGroupLifetime::PerFrame, rect.layout,
				{ ResourceGroupItem::ConstantBuffer(0, rect.uboView[frame]) } });
			cmd.beginRenderPass(GetDefaultRenderPass(), clears.data(), 1);
			cmd.setPipeline(rect.pipeline);
			cmd.setVertexBuffer(rect.vbo);
			cmd.setIndexBuffer(rect.ibo);
			cmd.setResourceGroup(rg);
			cmd.drawIndexed(6);
		}
		else {
			cmd.writeBuffer(cube.ubo[frame], &ubo, 0, sizeof(UBO));
			auto rg = CreateResourceGroup({ 0, ResourceGroupLifetime::PerFrame, cube.layout,
				{ ResourceGroupItem::ConstantBuffer(0, cube.uboView[frame]) } });
			cmd.beginRenderPass(GetDefaultRenderPass(), clears.data(), 1);
			cmd.setPipeline(cube.pipeline);
			cmd.setVertexBuffer(cube.vbo);
			cmd.setIndexBuffer(cube.ibo);
			cmd.setResourceGroup(rg);
			cmd.drawIndexed(36);
		}

		cmd.endRenderPass();
		cmd.close();
		ExecuteCommandList(cmd);
		End(frame);

		frame = (frame + 1) % MAX_FRAMES_IN_FLIGHT;
		glfwPollEvents();
	}

	Shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
