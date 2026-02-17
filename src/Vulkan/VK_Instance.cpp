#include "VK_Common.h"
#include "VK_RenderX.h"


#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <vulkan/vulkan_win32.h>
#include <windows.h>

#elif defined(__linux__)
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>

#endif

#include <cstring>
#include <vector>


namespace Rx::RxVK {

static std::vector<const char*> GetValidationLayers() {
#ifdef RX_DEBUG_BUILD
    return {"VK_LAYER_KHRONOS_validation"};
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
    app.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "RenderX";
    app.apiVersion       = VK_API_VERSION_1_3;

    std::vector<const char*> extensions(window.instanceExtensions, window.instanceExtensions + window.extensionCount);

#ifdef RX_DEBUG_BUILD
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    auto layers = GetValidationLayers();

    VkInstanceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo        = &app;
    ci.enabledExtensionCount   = uint32_t(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();
    ci.enabledLayerCount       = uint32_t(layers.size());
    ci.ppEnabledLayerNames     = layers.data();

    VK_CHECK(vkCreateInstance(&ci, nullptr, &m_Instance));
}

void VulkanInstance::createSurface(const InitDesc& window) {
#if defined(RX_PLATFORM_WINDOWS)
    VkWin32SurfaceCreateInfoKHR ci{};
    ci.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    ci.hwnd      = static_cast<HWND>(window.nativeWindowHandle);
    ci.hinstance = static_cast<HINSTANCE>(window.displayHandle);
    VK_CHECK(vkCreateWin32SurfaceKHR(m_Instance, &ci, nullptr, &m_Surface));
#elif defined(RX_PLATFORM_LINUX)
    VkXlibSurfaceCreateInfoKHR ci{};
    ci.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    ci.dpy    = static_cast<Display*>(window.displayHandle);
    ci.window = reinterpret_cast<::Window>(window.nativeWindowHandle);
    VK_CHECK(vkCreateXlibSurfaceKHR(m_Instance, &ci, nullptr, &m_Surface));
#elif defined(RX_PLATFORM_MACOS)
    VkMetalSurfaceCreateInfoEXT ci{};
    ci.sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    ci.pLayer = static_cast<CAMetalLayer*>(window.nativeHandle);
    VK_CHECK(vkCreateMetalSurfaceEXT(m_Instance, &ci, nullptr, &m_Surface));
#elif defined(RX_PLATFORM_HEADLESS)
    m_Surface = VK_NULL_HANDLE;
#else
#error "Vulkan surface creation not supported on this platform"
#endif
}

} // namespace Rx::RxVK
