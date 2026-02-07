#include "VK_RenderX.h"
#include "VK_Common.h"

#include <set>

namespace Rx::RxVK {

	static bool HasFlag(VkQueueFlags flags, VkQueueFlags bit) {
		return (flags & bit) == bit;
	}

	VulkanDevice::VulkanDevice(
		VkInstance instance,
		VkSurfaceKHR surface,
		const std::vector<const char*>& requiredExtensions,
		const std::vector<const char*>& requiredLayers)
		: m_Instance(instance), m_Surface(surface) {
		pickPhysicalDevice();
		createLogicalDevice(requiredExtensions, requiredLayers);
		queryLimits();
	}

	VulkanDevice::~VulkanDevice() {
		if (m_Device) {
			vkDeviceWaitIdle(m_Device);
			vkDestroyDevice(m_Device, nullptr);
		}
	}

	void VulkanDevice::pickPhysicalDevice() {
		uint32_t count = 0;
		vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);
		assert(count > 0);

		std::vector<VkPhysicalDevice> devices(count);
		vkEnumeratePhysicalDevices(m_Instance, &count, devices.data());

		for (auto dev : devices) {
			if (isDeviceSuitable(dev)) {
				m_PhysicalDevice = dev;
				break;
			}
		}

		assert(m_PhysicalDevice != VK_NULL_HANDLE);
	}

	bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device) const {
		uint32_t count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

		std::vector<VkQueueFamilyProperties> families(count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

		bool hasGraphics = false;
		bool hasPresent = false;

		for (uint32_t i = 0; i < count; i++) {
			if (HasFlag(families[i].queueFlags, VK_QUEUE_GRAPHICS_BIT))
				hasGraphics = true;

			VkBool32 present = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &present);
			if (present)
				hasPresent = true;
		}

		return hasGraphics && hasPresent;
	}

	void VulkanDevice::createLogicalDevice(
		const std::vector<const char*>& requiredExtensions,
		const std::vector<const char*>& requiredLayers) {
		uint32_t count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(
			m_PhysicalDevice, &count, nullptr);

		std::vector<VkQueueFamilyProperties> families(count);
		vkGetPhysicalDeviceQueueFamilyProperties(
			m_PhysicalDevice, &count, families.data());

		for (uint32_t i = 0; i < count; i++) {
			const auto& f = families[i];

			if (HasFlag(f.queueFlags, VK_QUEUE_GRAPHICS_BIT))
				m_GraphicsFamily = i;

			if (HasFlag(f.queueFlags, VK_QUEUE_COMPUTE_BIT) &&
				!HasFlag(f.queueFlags, VK_QUEUE_GRAPHICS_BIT))
				m_ComputeFamily = i;

			if (HasFlag(f.queueFlags, VK_QUEUE_TRANSFER_BIT) &&
				!HasFlag(f.queueFlags, VK_QUEUE_GRAPHICS_BIT) &&
				!HasFlag(f.queueFlags, VK_QUEUE_COMPUTE_BIT))
				m_TransferFamily = i;
		}

		if (m_ComputeFamily == UINT32_MAX)
			m_ComputeFamily = m_GraphicsFamily;

		if (m_TransferFamily == UINT32_MAX)
			m_TransferFamily = m_ComputeFamily;

		std::set<uint32_t> uniqueFamilies = {
			m_GraphicsFamily,
			m_ComputeFamily,
			m_TransferFamily
		};

		float priority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queues;

		for (uint32_t family : uniqueFamilies) {
			VkDeviceQueueCreateInfo q{};
			q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			q.queueFamilyIndex = family;
			q.queueCount = 1;
			q.pQueuePriorities = &priority;
			queues.push_back(q);
		}

		VkPhysicalDeviceFeatures features{};

		VkPhysicalDeviceDescriptorIndexingFeatures indexing{};
		indexing.descriptorBindingPartiallyBound = VK_TRUE;
		indexing.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		indexing.runtimeDescriptorArray = VK_TRUE;
		indexing.descriptorBindingVariableDescriptorCount = VK_TRUE;


		// Enable timeline semaphore feature if available
		VkPhysicalDeviceTimelineSemaphoreFeatures timelineFeatures{};
		timelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
		timelineFeatures.timelineSemaphore = VK_TRUE;

		VkDeviceCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		info.queueCreateInfoCount = uint32_t(queues.size());
		info.pQueueCreateInfos = queues.data();
		info.pEnabledFeatures = &features;
		// Chain timeline feature into device create info so timeline semaphores are enabled
		info.pNext = &timelineFeatures;
		info.enabledExtensionCount = uint32_t(requiredExtensions.size());
		info.ppEnabledExtensionNames = requiredExtensions.data();
		info.enabledLayerCount = uint32_t(requiredLayers.size());
		info.ppEnabledLayerNames = requiredLayers.data();

		vkCreateDevice(m_PhysicalDevice, &info, nullptr, &m_Device);

		vkGetDeviceQueue(m_Device, m_GraphicsFamily, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, m_ComputeFamily, 0, &m_ComputeQueue);
		vkGetDeviceQueue(m_Device, m_TransferFamily, 0, &m_TransferQueue);
	}

	void VulkanDevice::queryLimits() {
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);
		m_Limits = props.limits;
	}

} // namespace Rx::RxVK
