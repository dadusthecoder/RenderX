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

	
	Frame frames[3];
	for (auto& frame : frames) {
		frame.graphicsAlloc = graphics->CreateCommandAllocator();
		frame.computeAlloc = compute->CreateCommandAllocator();
		frame.graphicslist = frame.graphicsAlloc->Allocate();
		frame.computelist = frame.computeAlloc->Allocate();
	}
	uint32_t currentFrame = 0;

	while (!glfwWindowShouldClose(window)) {

		//look out for missing reference operator !!!
		auto& frame = frames[currentFrame];
		graphics->Wait(frame.T);

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
		submitInfo.waitDependencies.push_back({ Rx::QueueType::COMPUTE, t0 });
		frame.T = graphics->Submit(submitInfo);

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