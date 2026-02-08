#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
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
	Rx::Shutdown();
	return 0;
}