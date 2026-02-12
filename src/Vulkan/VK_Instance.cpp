#include "VK_RenderX.h"
#include "VK_Common.h"

#include <Windows.h>
#include <vector>
#include <cstring>
#include <vulkan/vulkan_win32.h>

namespace Rx::RxVK {

	static std::vector<const char*> GetValidationLayers() {
#ifdef RX_DEBUG_BUILD
		return { "VK_LAYER_KHRONOS_validation" };
#else
		return {};
#endif
	}

	VulkanInstance::VulkanInstance(const InitDesc& window) {
		createInstance(window);
		createSurface(window);
	}

	VulkanInstance::~VulkanInstance() {
		if (m_Surface)
			vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		if (m_Instance)
			vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanInstance::createInstance(const InitDesc& window) {
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

	void VulkanInstance::createSurface(const InitDesc& window) {
#if defined(RX_PLATFORM_WINDOWS)

		VkWin32SurfaceCreateInfoKHR ci{};
		ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		ci.hwnd = static_cast<HWND>(window.nativeWindowHandle);
		ci.hinstance = static_cast<HINSTANCE>(window.displayHandle);
		VK_CHECK(vkCreateWin32SurfaceKHR(m_Instance, &ci, nullptr, &m_Surface));

#elif defined(RX_PLATFORM_LINUX_X11)

		VkXlibSurfaceCreateInfoKHR ci{};
		ci.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
		ci.dpy = static_cast<Display*>(window.displayHandle);
		ci.window = reinterpret_cast<::Window>(window.nativeHandle);

		VK_CHECK(vkCreateXlibSurfaceKHR(m_Instance, &ci, nullptr, &m_Surface));

#elif defined(RX_PLATFORM_LINUX_WAYLAND)

		VkWaylandSurfaceCreateInfoKHR ci{};
		ci.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
		ci.display = static_cast<wl_display*>(window.displayHandle);
		ci.surface = static_cast<wl_surface*>(window.nativeHandle);

		VK_CHECK(vkCreateWaylandSurfaceKHR(m_Instance, &ci, nullptr, &m_Surface));

#elif defined(RX_PLATFORM_MACOS)

		// MoltenVK path
		VkMetalSurfaceCreateInfoEXT ci{};
		ci.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
		ci.pLayer = static_cast<CAMetalLayer*>(window.nativeHandle);

		VK_CHECK(vkCreateMetalSurfaceEXT(m_Instance, &ci, nullptr, &m_Surface));

#elif defined(RX_PLATFORM_HEADLESS)

		// Optional: no surface (compute / offscreen)
		m_Surface = VK_NULL_HANDLE;

#else
#error "Vulkan surface creation not supported on this platform"
#endif
	}


}
