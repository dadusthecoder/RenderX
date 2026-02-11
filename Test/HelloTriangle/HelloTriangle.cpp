#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#define RENDERX_DEBUG
#include "RenderX/RenderX.h"

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

	Rx::Window rxWindow{};
	rxWindow.api = Rx::GraphicsAPI::VULKAN;
	rxWindow.platform = Rx::Platform::WINDOWS;
	rxWindow.width = width;
	rxWindow.height = height;
	rxWindow.instanceExtensions = glfwGetRequiredInstanceExtensions(&rxWindow.extensionCount);
	rxWindow.nativeHandle = glfwGetWin32Window(window);
	rxWindow.displayHandle = GetModuleHandle(nullptr);
	Rx::Init(rxWindow);



	Rx::CommandQueue* graphics = Rx::GetGpuQueue(Rx::QueueType::GRAPHICS);
	Rx::CommandQueue* compute = Rx::GetGpuQueue(Rx::QueueType::COMPUTE);

	Rx::BufferDesc vertexbufferinfo{};
	vertexbufferinfo.usage = Rx::BufferUsage::VERTEX | Rx::BufferUsage::TRANSFER_DST;
	vertexbufferinfo.initialData = cubeIndices;
	vertexbufferinfo.memoryType = Rx::MemoryType::GPU_ONLY;
	vertexbufferinfo.size = sizeof(cubeVertices);


	auto vertexbuffer = Rx::CreateBuffer(vertexbufferinfo);

	Rx::SwapchainDesc swapchianinfo{};
	glfwGetWindowSize(window, (int*)&swapchianinfo.width, (int*)&swapchianinfo.height);
	swapchianinfo.preferredFromat = Rx::Format::RGBA8_SRGB;
	auto* swapchain = Rx::CreateSwapchain(swapchianinfo);


	Frame frames[3];
	for (auto& frame : frames) {
		frame.graphicsAlloc = graphics->CreateCommandAllocator();
		frame.computeAlloc = compute->CreateCommandAllocator();
		frame.graphicslist = frame.graphicsAlloc->Allocate();
		frame.computelist = frame.computeAlloc->Allocate();
	}

	uint32_t currentFrame = 0;
	uint32_t currentImageIndex = 0;
	int currentWidth = swapchianinfo.width;
	int currentHeight = swapchianinfo.height;

	while (!glfwWindowShouldClose(window)) {
		// look out for missing reference operator !!!
		auto& frame = frames[currentFrame];
		glfwGetWindowSize(window, &currentWidth, &currentHeight);
		if (swapchianinfo.width != currentWidth || swapchianinfo.height != currentHeight) {
			swapchianinfo.width = currentWidth;
			swapchianinfo.height = currentHeight;
			swapchain->Resize(currentWidth, currentHeight);
		}
		graphics->Wait(frame.T);

		currentImageIndex = swapchain->AcquireNextImage();

		frame.computelist->open();
		// Compute work
		frame.computelist->close();

		frame.graphicslist->open();
		// Graphics Work
		frame.graphicslist->close();


		auto t0 = compute->Submit(frame.computelist);
		Rx::SubmitInfo submitInfo{};
		submitInfo.commandList = frame.graphicslist;

		submitInfo.commandListCount = 1;
		submitInfo.writesToSwapchain = true;
		submitInfo.waitDependencies.push_back({ Rx::QueueType::COMPUTE, t0 });
		frame.T = graphics->Submit(submitInfo);

		swapchain->Present(currentImageIndex);
		currentFrame = (currentFrame + 1) % 3;
		glfwPollEvents();
	}

	graphics->WaitIdle();
	compute->WaitIdle();

	for (auto& frame : frames) {
		frame.computeAlloc->Free(frame.computelist);
		frame.graphicsAlloc->Free(frame.graphicslist);
		graphics->DestroyCommandAllocator(frame.graphicsAlloc);
		compute->DestroyCommandAllocator(frame.computeAlloc);
	}
	Rx::Shutdown();
	return 0;
}