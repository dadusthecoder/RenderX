#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#define RENDERX_DEBUG
#include "RenderX/RenderX.h"



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

	Rx::CommandAllocator* graphicsAlloc = graphics->CreateCommandAllocator();
	Rx::CommandAllocator* computeAlloc = compute->CreateCommandAllocator();

	Rx::CommandList* graphicslist = graphicsAlloc->Allocate();
	Rx::CommandList* computelist = computeAlloc->Allocate();

	while (!glfwWindowShouldClose(window)) {
		computelist->open();
		computelist->close();
		graphicslist->open();
		graphicslist->close();

		auto t0 = compute->Submit(computelist);
		Rx::SubmitInfo submitInfo{};
		submitInfo.commandList = graphicslist;
		submitInfo.commandListCount = 1;
		submitInfo.waitDependencies.push_back({ Rx::QueueType::COMPUTE, t0 });
		auto t1 = graphics->Submit(submitInfo);

		graphics->Wait(t1);
		//computeAlloc->Reset();
		//graphicsAlloc->Reset();
		glfwPollEvents();
	}

	graphics->DestroyCommandAllocator(graphicsAlloc);
	compute->DestroyCommandAllocator(computeAlloc);

	Rx::Shutdown();
	return 0;
}