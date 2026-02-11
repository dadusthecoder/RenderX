#include "VK_Common.h"
#include "VK_RenderX.h"

#include <algorithm>
#include <cassert>

namespace Rx::RxVK {

	static VkSurfaceCapabilitiesKHR QueryCaps(VkPhysicalDevice phys, VkSurfaceKHR surface) {
		VkSurfaceCapabilitiesKHR caps{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps);
		return caps;
	}

	static std::vector<VkSurfaceFormatKHR> QueryFormats(VkPhysicalDevice phys, VkSurfaceKHR surface) {
		uint32_t count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &count, nullptr);
		std::vector<VkSurfaceFormatKHR> formats(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &count, formats.data());
		return formats;
	}

	static std::vector<VkPresentModeKHR> QueryPresentModes(VkPhysicalDevice phys, VkSurfaceKHR surface) {
		uint32_t count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &count, nullptr);
		std::vector<VkPresentModeKHR> modes(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &count, modes.data());
		return modes;
	}

	VulkanSwapchain::VulkanSwapchain() {
	}

	VulkanSwapchain::~VulkanSwapchain() {
		destroy();
	}

	bool VulkanSwapchain::create(const SwapchainCreateInfo& info) {
		m_Info = info;
		auto& ctx = GetVulkanContext();

		createSwapchain(info.width, info.height);

		m_SignalSemaphores.resize(info.maxFramesInFlight);
		m_WaitSemaphores.resize(m_ImageCount);

		for (uint32_t i = 0; i < info.maxFramesInFlight; i++) {
			VkSemaphoreCreateInfo info1{};
			info1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VK_CHECK(vkCreateSemaphore(ctx.device->logical(), &info1, nullptr, &m_SignalSemaphores[i]));
		}

		for (uint32_t i = 0; i < m_ImageCount; i++) {
			VkSemaphoreCreateInfo info2{};
			info2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VK_CHECK(vkCreateSemaphore(ctx.device->logical(), &info2, nullptr, &m_WaitSemaphores[i]));
		}

		return true;
	}

	void VulkanSwapchain::destroy() {
		auto& ctx = GetVulkanContext();
		for (size_t i = 0; i < m_SignalSemaphores.size(); i++) {
			if (m_SignalSemaphores[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(ctx.device->logical(), m_SignalSemaphores[i], nullptr);
		}
		for (size_t i = 0; i < m_WaitSemaphores.size(); i++) {
			if (m_WaitSemaphores[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(ctx.device->logical(), m_WaitSemaphores[i], nullptr);
		}
		m_SignalSemaphores.clear();
		m_WaitSemaphores.clear();
		destroySwapchain();
	}

	void VulkanSwapchain::recreate(uint32_t width, uint32_t height) {
		auto& ctx = GetVulkanContext();
		RENDERX_INFO("Recreating Swapchain with size [ {} x {} ]", width, height);
		vkDeviceWaitIdle(ctx.device->logical());
		destroySwapchain();
		createSwapchain(width, height);
	}

	Format VulkanSwapchain::GetFormat() const { return VkFormatToFormat(m_Format); }


	uint32_t VulkanSwapchain::AcquireNextImage() {
		auto& ctx = GetVulkanContext();
		VkResult result = vkAcquireNextImageKHR(ctx.device->logical(), m_Swapchain, UINT64_MAX,
			m_SignalSemaphores[m_currentSemaphoreIndex], VK_NULL_HANDLE, &m_CurrentImageIndex);
		VK_CHECK(result);
		return m_CurrentImageIndex;
	}
	void VulkanSwapchain::Present(uint32_t imageIndex) {
		auto& ctx = GetVulkanContext();
		VkPresentInfoKHR info{};
		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &m_WaitSemaphores[imageIndex];
		info.swapchainCount = 1;
		info.pSwapchains = &m_Swapchain;
		info.pImageIndices = &imageIndex;
		VK_CHECK(vkQueuePresentKHR(ctx.graphicsQueue->Queue(), &info));
		m_currentSemaphoreIndex = (m_currentSemaphoreIndex + 1) % m_Info.maxFramesInFlight;
	}
	void VulkanSwapchain::Resize(uint32_t width, uint32_t height) {
		recreate(width, height);
	}

	void VulkanSwapchain::createSwapchain(uint32_t width, uint32_t height) {
		auto& ctx = GetVulkanContext();
		auto caps = QueryCaps(ctx.device->physical(), ctx.instance->getSurface());
		auto formats = QueryFormats(ctx.device->physical(), ctx.instance->getSurface());
		auto presentModes = QueryPresentModes(ctx.device->physical(), ctx.instance->getSurface());

		VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat();
		VkPresentModeKHR presentMode = choosePresentMode();
		VkExtent2D extent = chooseExtent(width, height);
		uint32_t desired = m_Info.imageCount;

		if (desired == 0)
			desired = caps.minImageCount + 1;

		uint32_t imageCount = std::max(desired, caps.minImageCount);

		if (caps.maxImageCount > 0)
			imageCount = std::min(imageCount, caps.maxImageCount);

		VkSwapchainCreateInfoKHR ci{};
		ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		ci.surface = ctx.instance->getSurface();
		ci.minImageCount = imageCount;
		ci.imageFormat = surfaceFormat.format;
		ci.imageColorSpace = surfaceFormat.colorSpace;
		ci.imageExtent = extent;
		ci.imageArrayLayers = 1;
		ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ci.preTransform = caps.currentTransform;
		ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		ci.presentMode = presentMode;
		ci.clipped = VK_TRUE;

		VK_CHECK(vkCreateSwapchainKHR(ctx.device->logical(), &ci, nullptr, &m_Swapchain));

		VK_CHECK(vkGetSwapchainImagesKHR(ctx.device->logical(), m_Swapchain, &imageCount, nullptr));
		m_Images.resize(imageCount);
		VK_CHECK(vkGetSwapchainImagesKHR(ctx.device->logical(), m_Swapchain, &imageCount, m_Images.data()));

		m_ImageViews.resize(imageCount);
		for (uint32_t i = 0; i < imageCount; ++i) {
			VkImageViewCreateInfo iv{};
			iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			iv.image = m_Images[i];
			iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
			iv.format = surfaceFormat.format;
			iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			iv.subresourceRange.levelCount = 1;
			iv.subresourceRange.layerCount = 1;
			vkCreateImageView(ctx.device->logical(), &iv, nullptr, &m_ImageViews[i]);
		}

		m_Format = surfaceFormat.format;
		m_Extent = extent;
		m_ImageCount = imageCount;
	}

	void VulkanSwapchain::destroySwapchain() {
		auto& ctx = GetVulkanContext();
		for (auto view : m_ImageViews)
			vkDestroyImageView(ctx.device->logical(), view, nullptr);

		m_ImageViews.clear();
		m_Images.clear();

		if (m_Swapchain) {
			vkDestroySwapchainKHR(ctx.device->logical(), m_Swapchain, nullptr);
			m_Swapchain = VK_NULL_HANDLE;
		}
	}

	VkSurfaceFormatKHR VulkanSwapchain::chooseSurfaceFormat() const {
		auto& ctx = GetVulkanContext();
		auto formats = QueryFormats(ctx.device->physical(), ctx.instance->getSurface());
		for (auto& f : formats) {
			if (f.format == m_Info.preferredFormat &&
				f.colorSpace == m_Info.preferredColorSpace) {
				RENDERX_INFO("Swapchain format: {} | ColorSpace: {}",
					FormatToString(VkFormatToFormat(m_Info.preferredFormat)), VkColorSpaceToString(f.colorSpace));
				return f;
			}
		}

		RENDERX_WARN("The surface doesn't support preferred format: {}", FormatToString(VkFormatToFormat(m_Info.preferredFormat)));
		assert(!formats.empty() && "No surface formats available");
		RENDERX_INFO("Swapchain format: {} | ColorSpace={}",
			FormatToString(VkFormatToFormat(formats[0].format)), VkColorSpaceToString(formats[0].colorSpace));
		return formats[0];
	}
	VkPresentModeKHR VulkanSwapchain::choosePresentMode() const {
		auto& ctx = GetVulkanContext();
		auto modes = QueryPresentModes(ctx.device->physical(), ctx.instance->getSurface());
		for (auto m : modes)
			if (m == m_Info.preferredPresentMode)
				return m;
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VulkanSwapchain::chooseExtent(uint32_t width, uint32_t height) const {
		auto& ctx = GetVulkanContext();
		auto caps = QueryCaps(ctx.device->physical(), ctx.instance->getSurface());
		if (caps.currentExtent.width != UINT32_MAX)
			return caps.currentExtent;

		VkExtent2D e{};
		e.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
		e.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
		return e;
	}

	// renderx API implemention
	Swapchain* VKCreateSwapchain(const SwapchainDesc& desc) {
		auto& ctx = GetVulkanContext();
		SwapchainCreateInfo info{};
		info.height = desc.height;
		info.width = desc.width;
		info.imageCount = desc.preferredImageCount;
		info.maxFramesInFlight = desc.maxFramesInFlight;
		info.preferredFormat = ToVulkanFormat(desc.preferredFromat);
		info.preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // Or add to SwapchainDesc
		info.preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;	  // Or add to SwapchainDesc
		ctx.swapchain->create(info);
		return ctx.swapchain.get();
	}

	void VKDestroySwapchain(Swapchain* swapchain) {
		VulkanSwapchain* vulkanspchn = reinterpret_cast<VulkanSwapchain*>(swapchain);
		vulkanspchn->destroy();
	}

} // namespace Rx::RxVK
