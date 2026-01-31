// #include "RenderX/RenderX.h"
// #include "Files.h"
// #include "RenderX/Log.h"
// #include "Windows.h"
// #include <GLFW/glfw3.h>

// #define GLFW_EXPOSE_NATIVE_WIN32
// #include <GLFW/glfw3native.h>

// using namespace Rx;
// struct UBO {
// 	Mat4 viewProj;
// };

// #define MAX_FRMES_IN_FLIGHT 3
// static GraphicsAPI currentAPI = GraphicsAPI::Vulkan;

// struct RenderResources {
// 	ShaderHandle vertexShader;
// 	ShaderHandle fragmentShader;
// 	BufferHandle vertexBuffer;
// 	BufferHandle indexBuffer;
// 	std::vector<BufferHandle> uboBuffer;
// 	std::vector<BufferViewHandle> uboBufferView;
// 	PipelineLayoutHandle pipelineLayout;
// 	PipelineHandle pipeline;
// 	ResourceGroupHandle perFrameRG;
// };

// RenderResources CreateResources() {
// 	RenderResources res{};

// 	if (currentAPI == GraphicsAPI::Vulkan) {
// 		res.vertexShader = CreateShader({ ShaderStage::Vertex,
// 			Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.vert.spv") });

// 		res.fragmentShader = CreateShader({ ShaderStage::Fragment,
// 			Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.frag.spv") });
// 	}
// 	else {
// 		res.vertexShader = CreateShader({ ShaderStage::Vertex,
// 			Files::ReadTextFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.vert") });

// 		res.fragmentShader = CreateShader({ ShaderStage::Fragment,
// 			Files::ReadTextFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.frag") });
// 	}

// 	const float vertices[] = {
// 		-0.5f, -0.5f, 0.0f,
// 		0.5f, -0.5f, 0.0f,
// 		0.5f, 0.5f, 0.0f,
// 		-0.5f, 0.5f, 0.0f
// 	};
// 	const uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };

// 	res.vertexBuffer = CreateBuffer({ BufferFlags::Vertex | BufferFlags::Static | BufferFlags::TransferDst, MemoryType::GpuOnly,
// 		sizeof(vertices), 1, vertices });

// 	res.indexBuffer = CreateBuffer({ BufferFlags::Index | BufferFlags::Static | BufferFlags::TransferDst, MemoryType::GpuOnly,
// 		sizeof(indices), 1, indices });

// 	ResourceGroupLayout setLayout{};
// 	setLayout.setIndex = 0;
// 	setLayout.resourcebindings = {
// 		ResourceGroupLayoutItem::ConstantBuffer(0, ShaderStage::Vertex)
// 	};

// 	res.pipelineLayout = CreatePipelineLayout(&setLayout, 1);

// 	BufferDesc uboDesc{};
// 	// if Static bit is set then user ,ust also set the TransferDst bit to allow drivers to copy data to the buffer
// 	//
// 	uboDesc.flags = BufferFlags::Uniform | BufferFlags::Static | BufferFlags::TransferDst;
// 	uboDesc.momory = MemoryType::GpuOnly;
// 	uboDesc.size = sizeof(UBO);
// 	for (uint32_t i = 0; i < MAX_FRMES_IN_FLIGHT; ++i)
// 		res.uboBuffer.push_back(CreateBuffer(uboDesc));

// 	for (uint32_t i = 0; i < MAX_FRMES_IN_FLIGHT; ++i) {
// 		BufferViewDesc viewDesc{};
// 		viewDesc.buffer = res.uboBuffer[i];
// 		viewDesc.range = sizeof(UBO);
// 		res.uboBufferView.push_back(CreateBufferView(viewDesc));
// 	}

// 	VertexInputState vi{};
// 	vi.attributes.push_back({ 0, 0, DataFormat::RGB32F, 0 });
// 	vi.vertexBindings.push_back({ 0, sizeof(float) * 3, false });

// 	PipelineDesc pso{};
// 	pso.shaders = { res.vertexShader, res.fragmentShader };
// 	pso.vertexInputState = vi;
// 	pso.renderPass = GetDefaultRenderPass();
// 	pso.layout = res.pipelineLayout;
// 	pso.rasterizer.cullMode = CullMode::None;
// 	pso.rasterizer.fillMode = FillMode::Wireframe;

// 	res.pipeline = CreateGraphicsPipeline(pso);

// 	return res;
// }

// void DestroyResources(RenderResources& res) {
// 	res = {};
// }

// GraphicsAPI SwitchAPI(GraphicsAPI api) {
// 	return api == GraphicsAPI::Vulkan
// 			   ? GraphicsAPI::OpenGL
// 			   : GraphicsAPI::Vulkan;
// }


// int main() {
// 	using namespace Rx;

// 	// === Application responsibility: Create window ===
// 	if (!glfwInit()) {
// 		return 0;
// 	}
// 	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
// 	auto* window = glfwCreateWindow(1280, 720,
// 		currentAPI == GraphicsAPI::OpenGL ? "RenderX - OpenGL Backend" : "RenderX - Vulkan Backend",
// 		nullptr, nullptr);
// 	int fbWidth = 0;
// 	int fbHeight = 0;

// 	while (fbWidth == 0 || fbHeight == 0) {
// 		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
// 		glfwPollEvents();
// 	}

// 	// === Pass window handles to RenderX ===
// 	Window rxWindow;
// 	rxWindow.api = currentAPI;
// 	rxWindow.platform = Platform::Windows;
// 	rxWindow.height = fbHeight;
// 	rxWindow.width = fbWidth;
// 	rxWindow.instanceExtensions = glfwGetRequiredInstanceExtensions(&rxWindow.extensionCount);
// 	rxWindow.nativeHandle = glfwGetWin32Window(window);
// 	rxWindow.displayHandle = GetModuleHandle(nullptr);
// 	Init(rxWindow);
// 	auto resources = CreateResources();

// 	std::vector<CommandList> frameCommandLists;
// 	for (uint32_t i = 0; i < MAX_FRMES_IN_FLIGHT; ++i)
// 		frameCommandLists.push_back(CreateCommandList(i));

// 	std::vector<ClearValue> clearValues;
// 	ClearValue clear{};
// 	clearValues.emplace_back();
// 	clearValues[0].color = ClearColor(0.01f, 0.01f, 0.01f, 1.0f);

// 	uint32_t currentFrame = 0;

// 	UBO ubo{};
// 	ubo.viewProj = glm::perspective(glm::radians(45.0f), static_cast<float>(fbHeight) / static_cast<float>(fbWidth), 0.1f, 100.0f);

// 	while (!glfwWindowShouldClose(window)) {
// 		// --- Backend switch on 'S' ---
// 		if (GetAsyncKeyState('S') & 1) // press S to switch
// 		{
// 			// Destroy per-frame command lists for current backend
// 			for (uint32_t i = 0; i < MAX_FRMES_IN_FLIGHT; ++i)
// 				DestroyCommandList(frameCommandLists[i], i);
// 			frameCommandLists.clear();

// 			// Shut down current RHI backend
// 			Shutdown();

// 			// Destroy current window and create a fresh one for the new backend
// 			glfwDestroyWindow(window);

// 			currentAPI = SwitchAPI(currentAPI);

// 			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
// 			window = glfwCreateWindow(1280, 720,
// 				currentAPI == GraphicsAPI::OpenGL ? "RenderX - OpenGL Backend" : "RenderX - Vulkan Backend",
// 				nullptr, nullptr);

// 			// Query framebuffer size for the new window
// 			fbWidth = fbHeight = 0;
// 			while (fbWidth == 0 || fbHeight == 0) {
// 				glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
// 				glfwPollEvents();
// 			}

// 			// Rebuild Window description and re-initialize RHI
// 			rxWindow.api = currentAPI;
// 			rxWindow.platform = Platform::Windows;
// 			rxWindow.height = fbHeight;
// 			rxWindow.width = fbWidth;
// 			rxWindow.instanceExtensions = glfwGetRequiredInstanceExtensions(&rxWindow.extensionCount);
// 			rxWindow.nativeHandle = glfwGetWin32Window(window);
// 			rxWindow.displayHandle = GetModuleHandle(nullptr);

// 			Init(rxWindow);
// 			resources = CreateResources();

// 			for (uint32_t i = 0; i < MAX_FRMES_IN_FLIGHT; ++i)
// 				frameCommandLists.push_back(CreateCommandList(i));

// 			// Start next iteration with frame index 0 for simplicity
// 			currentFrame = 0;
// 			continue; // Skip rendering this frame after switch
// 		}

// 		Begin(currentFrame);

// 		ResourceGroupDesc rg{};
// 		rg.setIndex = 0;
// 		rg.flags = ResourceGroupLifetime::PerFrame;
// 		rg.layout = resources.pipelineLayout;
// 		rg.Resources = {
// 			ResourceGroupItem::ConstantBuffer(0, resources.uboBufferView[currentFrame])
// 		};
// 		resources.perFrameRG = CreateResourceGroup(rg);

// 		auto cmd = frameCommandLists[currentFrame];
// 		cmd.open();
// 		cmd.writeBuffer(resources.uboBuffer[currentFrame], &ubo, 0, sizeof(UBO));

// 		cmd.beginRenderPass(GetDefaultRenderPass(),
// 			clearValues.data(),
// 			(uint32_t)clearValues.size());
// 		cmd.setPipeline(resources.pipeline);
// 		cmd.setVertexBuffer(resources.vertexBuffer);
// 		cmd.setIndexBuffer(resources.indexBuffer);
// 		cmd.setResourceGroup(resources.perFrameRG);

// 		cmd.drawIndexed(6);

// 		cmd.endRenderPass();
// 		cmd.close();
// 		ExecuteCommandList(cmd);

// 		End(currentFrame);

// 		currentFrame = (currentFrame + 1) % MAX_FRMES_IN_FLIGHT;
// 		glfwPollEvents();
// 	}

// 	// Cleanup
// 	Shutdown();
// 	glfwDestroyWindow(window);
// 	glfwTerminate();
// 	return 0;
// }


#include "RenderX/RenderX.h"
#include "RenderX/Log.h"
#include "Files.h"

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace Rx;

// Config
#define MAX_FRAMES_IN_FLIGHT 3

static GraphicsAPI g_CurrentAPI = GraphicsAPI::Vulkan;

// Data
struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
};

struct UBO {
	glm::mat4 mvp;
};

// Cube geometry
static const Vertex kCubeVertices[] = {
	{ { -0.5f, -0.5f, -0.5f }, { 1, 0, 0 } },
	{ { 0.5f, -0.5f, -0.5f }, { 0, 1, 0 } },
	{ { 0.5f, 0.5f, -0.5f }, { 0, 0, 1 } },
	{ { -0.5f, 0.5f, -0.5f }, { 1, 1, 0 } },
	{ { -0.5f, -0.5f, 0.5f }, { 1, 0, 1 } },
	{ { 0.5f, -0.5f, 0.5f }, { 0, 1, 1 } },
	{ { 0.5f, 0.5f, 0.5f }, { 1, 1, 1 } },
	{ { -0.5f, 0.5f, 0.5f }, { 0, 0, 0 } },
};

static const uint32_t kCubeIndices[] = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4,
	0, 4, 7, 7, 3, 0,
	1, 5, 6, 6, 2, 1,
	3, 2, 6, 6, 7, 3,
	0, 1, 5, 5, 4, 0
};

// Render resources
struct CubeResources {
	ShaderHandle vs;
	ShaderHandle fs;

	BufferHandle vbo;
	BufferHandle ibo;

	BufferHandle ubo[MAX_FRAMES_IN_FLIGHT];
	BufferViewHandle uboView[MAX_FRAMES_IN_FLIGHT];

	PipelineLayoutHandle layout;
	PipelineHandle pipeline;
};

// Resource creation
CubeResources CreateCubeResources() {
	CubeResources r{};

	// Shaders
	r.vs = CreateShader({ ShaderStage::Vertex,
		Files::ReadBinaryFile("Shaders/bsc.vert.spv") });

	r.fs = CreateShader({ ShaderStage::Fragment,
		Files::ReadBinaryFile("Shaders/bsc.frag.spv") });

	// Buffers
	r.vbo = CreateBuffer({ BufferFlags::Vertex | BufferFlags::Static | BufferFlags::TransferDst,
		MemoryType::GpuOnly,
		sizeof(kCubeVertices),
		1,
		kCubeVertices });

	r.ibo = CreateBuffer({ BufferFlags::Index | BufferFlags::Static | BufferFlags::TransferDst,
		MemoryType::GpuOnly,
		sizeof(kCubeIndices),
		1,
		kCubeIndices });

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		r.ubo[i] = CreateBuffer({ BufferFlags::Uniform | BufferFlags::Static | BufferFlags::TransferDst,
			MemoryType::GpuOnly,
			sizeof(UBO) });

		r.uboView[i] = CreateBufferView({ r.ubo[i],
			0,
			sizeof(UBO) });
	}

	// Pipeline layout
	ResourceGroupLayout set{};
	set.setIndex = 0;
	set.resourcebindings = {
		ResourceGroupLayoutItem::ConstantBuffer(0, ShaderStage::Vertex)
	};

	r.layout = CreatePipelineLayout(&set, 1);

	// Vertex input
	VertexInputState vi{};
	vi.vertexBindings.push_back({ 0, sizeof(Vertex), false });
	vi.attributes = {
		{ 0, 0, DataFormat::RGB32F, offsetof(Vertex, position) },
		{ 1, 0, DataFormat::RGB32F, offsetof(Vertex, color) }
	};

	// Pipeline
	PipelineDesc pso{};
	pso.layout = r.layout;
	pso.renderPass = GetDefaultRenderPass();
	pso.vertexInputState = vi;
	pso.shaders = { r.vs, r.fs };
	pso.rasterizer.cullMode = CullMode::Back;
	pso.rasterizer.fillMode = FillMode::Solid;

	r.pipeline = CreateGraphicsPipeline(pso);

	return r;
}



// Main
int main() {
	// --- Window ---
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(
		1280, 720,
		"RenderX – Rotating Cube",
		nullptr, nullptr);

	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwPollEvents();
	}

	// --- Init RenderX ---
	Window rxWindow{};
	rxWindow.api = g_CurrentAPI;
	rxWindow.platform = Platform::Windows;
	rxWindow.width = width;
	rxWindow.height = height;
	rxWindow.instanceExtensions = glfwGetRequiredInstanceExtensions(&rxWindow.extensionCount);
	rxWindow.nativeHandle = glfwGetWin32Window(window);
	rxWindow.displayHandle = GetModuleHandle(nullptr);

	Init(rxWindow);

	CubeResources cube = CreateCubeResources();

	std::vector<ClearValue> clearValues;
	ClearValue clear{};
	clearValues.emplace_back();
	clearValues[0].color = ClearColor(0.01f, 0.01f, 0.01f, 1.0f);

	std::vector<CommandList> commandLists;
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		commandLists.push_back(CreateCommandList(i));

	float angle = 0.0f;
	uint32_t currentframe = 0;

	while (!glfwWindowShouldClose(window)) {
		Begin(currentframe);

		angle += 0.01f;

		glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, { 0, 1, 0 });
		glm::mat4 view = glm::lookAt(
			glm::vec3(2, 2, 2),
			glm::vec3(0, 0, 0),
			glm::vec3(0, 1, 0));

		glm::mat4 proj = glm::perspective(
			glm::radians(60.0f),
			float(width) / float(height),
			0.1f, 100.0f);

		// Vulkan NDC fix
		proj[1][1] *= -1.0f;

		UBO ubo{};
		ubo.mvp = proj * view * model;

		CommandList cmd = commandLists[currentframe];
		cmd.open();

		cmd.writeBuffer(cube.ubo[currentframe], &ubo, 0, sizeof(UBO));

		ResourceGroupDesc rg{};
		rg.flags = ResourceGroupLifetime::PerFrame;
		rg.layout = cube.layout;
		rg.setIndex = 0;
		rg.Resources = {
			ResourceGroupItem::ConstantBuffer(0, cube.uboView[currentframe])
		};

		auto group = CreateResourceGroup(rg);

		cmd.beginRenderPass(GetDefaultRenderPass(), clearValues.data(),
			(uint32_t)clearValues.size());

		cmd.setPipeline(cube.pipeline);
		cmd.setVertexBuffer(cube.vbo);
		cmd.setIndexBuffer(cube.ibo);
		cmd.setResourceGroup(group);
		cmd.drawIndexed(36);
		cmd.endRenderPass();

		cmd.close();
		ExecuteCommandList(cmd);

		End(currentframe);
		glfwPollEvents();
		currentframe = (currentframe + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	Shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
