#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "RenderX/RenderX.h"
#include "Files.h"

struct Frame {
	Rx::CommandAllocator* graphicsAlloc;
	Rx::CommandAllocator* computeAlloc;
	Rx::CommandList* graphicslist;
	Rx::CommandList* computelist;
	Rx::Timeline T = Rx::Timeline(0);
};

float cubeVertices[] = {
	// positions          // colors
	-0.5f, -0.5f, -0.5f, 1, 0, 0,
	0.5f, -0.5f, -0.5f, 0, 1, 0,
	0.5f, 0.5f, -0.5f, 0, 0, 1,
	-0.5f, 0.5f, -0.5f, 1, 1, 0,

	-0.5f, -0.5f, 0.5f, 1, 0, 1,
	0.5f, -0.5f, 0.5f, 0, 1, 1,
	0.5f, 0.5f, 0.5f, 1, 1, 1,
	-0.5f, 0.5f, 0.5f, 0, 0, 0
};

unsigned int cubeIndices[] = {
	// back
	0, 1, 2, 2, 3, 0,
	// front
	4, 5, 6, 6, 7, 4,
	// left
	4, 7, 3, 3, 0, 4,
	// right
	1, 5, 6, 6, 2, 1,
	// bottom
	4, 5, 1, 1, 0, 4,
	// top
	3, 2, 6, 6, 7, 3
};


int main() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "RenderX", nullptr, nullptr);

	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwPollEvents();
	}

	Rx::InitDesc rxWindow{};
	rxWindow.api = Rx::GraphicsAPI::VULKAN;
	rxWindow.platform = Rx::Platform::WINDOWS;
	rxWindow.windowWidth = width;
	rxWindow.windowHeight = height;
	rxWindow.instanceExtensions = glfwGetRequiredInstanceExtensions(&rxWindow.extensionCount);
	rxWindow.nativeWindowHandle = glfwGetWin32Window(window);
	rxWindow.displayHandle = GetModuleHandle(nullptr);
	Rx::Init(rxWindow);

	// Get GPU queues
	Rx::CommandQueue* graphics = Rx::GetGpuQueue(Rx::QueueType::GRAPHICS);
	Rx::CommandQueue* compute = Rx::GetGpuQueue(Rx::QueueType::COMPUTE);

	// Create vertex buffer 
	auto vertexbuffer = Rx::CreateBuffer(
		Rx::BufferDesc::VertexBuffer(
			sizeof(cubeVertices),
			Rx::MemoryType::GPU_ONLY,
			cubeVertices));

	// Create vertex input state
	auto vertexInputState = Rx::VertexInputState()
								.AddBinding(Rx::VertexBinding::PerVertex(0, sizeof(float) * 6))
								.AddAttribute(Rx::VertexAttribute::Vec3(0, 0, 0))				   // position
								.AddAttribute(Rx::VertexAttribute::Vec3(1, 0, sizeof(float) * 3)); // color

	// Load shaders
	auto vertexshader = Rx::CreateShader(
		Rx::ShaderDesc::FromBytecode(
			Rx::ShaderStage::VERTEX,
			Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.vert.spv")));

	auto fragmentshader = Rx::CreateShader(
		Rx::ShaderDesc::FromBytecode(
			Rx::ShaderStage::FRAGMENT,
			Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.frag.spv")));

	// Create swapchain
	int swapWidth, swapHeight;
	glfwGetWindowSize(window, &swapWidth, &swapHeight);

	auto* swapchain = Rx::CreateSwapchain(
		Rx::SwapchainDesc::Default(swapWidth, swapHeight)
			.setFormat(Rx::Format::BGRA8_SRGB));

	// Create resource group
	auto rglayout = Rx::CreateResourceGroupLayout(
		Rx::ResourceGroupLayoutDesc()
			.AddBinding(Rx::ResourceGroupLayoutItem::ConstantBuffer(0, Rx::ShaderStage::VERTEX))
			.setDebugName("MainResourceGroupLayout"));

	// Create pipeline layout
	auto pipelineLayout = Rx::CreatePipelineLayout(&rglayout, 1);

	// Create graphics pipeline
	auto pipeline = Rx::CreateGraphicsPipeline(
		Rx::PipelineDesc()
			.setLayout(pipelineLayout)
			.AddColorFormat(Rx::Format::BGRA8_SRGB)
			.AddShader(vertexshader)
			.AddShader(fragmentshader)
			.setVertexInput(vertexInputState)
			.setTopology(Rx::Topology::TRIANGLES)
			.setRasterizer(Rx::RasterizerState::Default())
			.setDepthStencil(Rx::DepthStencilState::NoDepth())
			.setBlend(Rx::BlendState::Disabled()));

	// Setup per-frame resources
	Frame frames[3];
	for (auto& frame : frames) {
		frame.graphicsAlloc = graphics->CreateCommandAllocator();
		frame.computeAlloc = compute->CreateCommandAllocator();
		frame.graphicslist = frame.graphicsAlloc->Allocate();
		frame.computelist = frame.computeAlloc->Allocate();
	}

	uint32_t currentFrame = 0;
	uint32_t currentImageIndex = 0;
	int currentWidth = swapWidth;
	int currentHeight = swapHeight;

	while (!glfwWindowShouldClose(window)) {
		auto& frame = frames[currentFrame];

		// Handle window resize
		glfwGetWindowSize(window, &currentWidth, &currentHeight);
		if (swapchain->GetWidth() != currentWidth || swapchain->GetHeight() != currentHeight) {
			swapchain->Resize(currentWidth, currentHeight);
		}

		// Wait for this frame to complete its prevois submition
		graphics->Wait(frame.T);

		// Acquire next swapchain image
		currentImageIndex = swapchain->AcquireNextImage();

		// Record compute commands
		frame.computelist->open();
		// Compute work...
		frame.computelist->close();

		// Record graphics commands
		frame.graphicslist->open();
		// Graphics work...
		frame.graphicslist->close();

		// Submit compute work
		auto t0 = compute->Submit(frame.computelist);

		// Submit graphics work set dependency on compute and swapchain write
		frame.T = graphics->Submit(
			Rx::SubmitInfo::Single(frame.graphicslist)
				.setSwapchainWrite()
				.AddDependency(Rx::QueueDependency(Rx::QueueType::COMPUTE, t0)));

		// Present
		swapchain->Present(currentImageIndex);

		// Advance to next frame
		currentFrame = (currentFrame + 1) % 3;
		glfwPollEvents();
	}

	// Cleanup
	graphics->WaitIdle();
	compute->WaitIdle();

	for (auto& frame : frames) {
		frame.computeAlloc->Free(frame.computelist);
		frame.graphicsAlloc->Free(frame.graphicslist);
		graphics->DestroyCommandAllocator(frame.graphicsAlloc);
		compute->DestroyCommandAllocator(frame.computeAlloc);
	}

	Rx::Shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}