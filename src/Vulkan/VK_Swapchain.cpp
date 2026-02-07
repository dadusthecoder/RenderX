#include "VK_Common.h"
#include "VK_RenderX.h"

#include <algorithm>
#include <cassert>

namespace Rx::RxVK {

static VkSurfaceCapabilitiesKHR QueryCaps(VkPhysicalDevice phys, VkSurfaceKHR surface)
{
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps);
    return caps;
}

static std::vector<VkSurfaceFormatKHR> QueryFormats(VkPhysicalDevice phys, VkSurfaceKHR surface)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &count, formats.data());
    return formats;
}

static std::vector<VkPresentModeKHR> QueryPresentModes(VkPhysicalDevice phys, VkSurfaceKHR surface)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &count, nullptr);
    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &count, modes.data());
    return modes;
}

VulkanSwapchain::~VulkanSwapchain()
{
    destroy();
}

bool VulkanSwapchain::create(const SwapchainCreateInfo& info)
{
    m_Info = info;
    m_PhysicalDevice = info.physicalDevice;
    m_Device = info.device;
    m_Surface = info.surface;

    createSwapchain(info.width, info.height);
    return true;
}

void VulkanSwapchain::destroy()
{
    destroySwapchain();
}

void VulkanSwapchain::recreate(uint32_t width, uint32_t height)
{
    vkDeviceWaitIdle(m_Device);
    destroySwapchain();
    createSwapchain(width, height);
}

bool VulkanSwapchain::acquireNextImage(VkSemaphore signalSemaphore, uint32_t* outImageIndex)
{
    VkResult res = vkAcquireNextImageKHR(
        m_Device,
        m_Swapchain,
        UINT64_MAX,
        signalSemaphore,
        VK_NULL_HANDLE,
        outImageIndex
    );

    return res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR;
}

bool VulkanSwapchain::present(VkQueue presentQueue, VkSemaphore waitSemaphore, uint32_t imageIndex)
{
    VkPresentInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &waitSemaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &m_Swapchain;
    info.pImageIndices = &imageIndex;

    VkResult res = vkQueuePresentKHR(presentQueue, &info);
    return res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR;
}

void VulkanSwapchain::createSwapchain(uint32_t width, uint32_t height)
{
    auto caps = QueryCaps(m_PhysicalDevice, m_Surface);
    auto formats = QueryFormats(m_PhysicalDevice, m_Surface);
    auto presentModes = QueryPresentModes(m_PhysicalDevice, m_Surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat();
    VkPresentModeKHR presentMode = choosePresentMode();
    VkExtent2D extent = chooseExtent(width, height);

    uint32_t imageCount = std::max(m_Info.imageCount, caps.minImageCount);
    if (caps.maxImageCount > 0)
        imageCount = std::min(imageCount, caps.maxImageCount);

    VkSwapchainCreateInfoKHR ci{};
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface = m_Surface;
    ci.minImageCount = imageCount;
    ci.imageFormat = surfaceFormat.format;
    ci.imageColorSpace = surfaceFormat.colorSpace;
    ci.imageExtent = extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform = caps.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = presentMode;
    ci.clipped = VK_TRUE;

    vkCreateSwapchainKHR(m_Device, &ci, nullptr, &m_Swapchain);

    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
    m_Images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_Images.data());

    m_ImageViews.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        VkImageViewCreateInfo iv{};
        iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv.image = m_Images[i];
        iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv.format = surfaceFormat.format;
        iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.levelCount = 1;
        iv.subresourceRange.layerCount = 1;
        vkCreateImageView(m_Device, &iv, nullptr, &m_ImageViews[i]);
    }

    m_Format = surfaceFormat.format;
    m_Extent = extent;
    m_ImageCount = imageCount;
}

void VulkanSwapchain::destroySwapchain()
{
    for (auto view : m_ImageViews)
        vkDestroyImageView(m_Device, view, nullptr);

    m_ImageViews.clear();
    m_Images.clear();

    if (m_Swapchain)
    {
        vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
        m_Swapchain = VK_NULL_HANDLE;
    }
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSurfaceFormat() const
{
    auto formats = QueryFormats(m_PhysicalDevice, m_Surface);
    for (auto& f : formats)
    {
        if (f.format == m_Info.preferredFormat &&
            f.colorSpace == m_Info.preferredColorSpace)
            return f;
    }
    return formats[0];
}

VkPresentModeKHR VulkanSwapchain::choosePresentMode() const
{
    auto modes = QueryPresentModes(m_PhysicalDevice, m_Surface);
    for (auto m : modes)
        if (m == m_Info.preferredPresentMode)
            return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseExtent(uint32_t width, uint32_t height) const
{
    auto caps = QueryCaps(m_PhysicalDevice, m_Surface);
    if (caps.currentExtent.width != UINT32_MAX)
        return caps.currentExtent;

    VkExtent2D e{};
    e.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
    e.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return e;
}

} // namespace Rx::RxVK
