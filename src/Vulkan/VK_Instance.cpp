#include "VK_RenderX.h"
#include "VK_Common.h"

#include <Windows.h>
#include <vector>
#include <cstring>
#include <vulkan/vulkan_win32.h>

namespace Rx::RxVK {

	static std::vector<const char*> GetValidationLayers() {
#ifdef RENDERX_DEBUG
		return { "VK_LAYER_KHRONOS_validation" };
#else
		return {};
#endif
	}

	VulkanInstance::VulkanInstance(const Window& window) {
		createInstance(window);
		createSurface(window);
	}

	VulkanInstance::~VulkanInstance() {
		if (m_Surface)
			vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		if (m_Instance)
			vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanInstance::createInstance(const Window& window) {
		VkApplicationInfo app{};
		app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app.pApplicationName = "RenderX";
		app.apiVersion = VK_API_VERSION_1_3;

		std::vector<const char*> extensions(
			window.instanceExtensions,
			window.instanceExtensions + window.extensionCount);

#ifdef RENDERX_DEBUG
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		auto layers = GetValidationLayers();

		VkInstanceCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		ci.pApplicationInfo = &app;
		ci.enabledExtensionCount = uint32_t(extensions.size());
		ci.ppEnabledExtensionNames = extensions.data();
		ci.enabledLayerCount = uint32_t(layers.size());
		ci.ppEnabledLayerNames = layers.data();

		VK_CHECK(vkCreateInstance(&ci, nullptr, &m_Instance));
	}

	void VulkanInstance::createSurface(const Window& window) {
#if defined(RENDERX_PLATFORM_WINDOWS)
		VkWin32SurfaceCreateInfoKHR ci{};
		ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		ci.hwnd = (HWND)window.nativeHandle;
		ci.hinstance = (HINSTANCE)window.displayHandle;
		VK_CHECK(vkCreateWin32SurfaceKHR(m_Instance, &ci, nullptr, &m_Surface));
#else
#error Platform not supported
#endif
	}

}
