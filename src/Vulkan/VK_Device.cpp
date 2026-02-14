#include "VK_RenderX.h"
#include "VK_Common.h"

#include <set>
#include <iostream>
#include <iomanip>

namespace Rx::RxVK {

	static bool HasFlag(VkQueueFlags flags, VkQueueFlags bit) {
		return (flags & bit) == bit;
	}

	DeviceInfo VulkanDevice::gatherDeviceInfo(VkPhysicalDevice device) const {
		DeviceInfo info;
		info.device = device;
		vkGetPhysicalDeviceProperties(device, &info.properties);
		vkGetPhysicalDeviceFeatures(device, &info.features);
		vkGetPhysicalDeviceMemoryProperties(device, &info.memoryProperties);
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		info.queueFamilies.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, info.queueFamilies.data());
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		info.extensions.resize(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, info.extensions.data());
		return info;
	}

	void VulkanDevice::logDeviceInfo(uint32_t index, const DeviceInfo& info) const {
		const auto& props = info.properties;
		const auto& features = info.features;
		const auto& mem = info.memoryProperties;

		RENDERX_INFO("------------------------------------------------------");
		RENDERX_INFO("Device [{}]: {}", index, props.deviceName);
		RENDERX_INFO("------------------------------------------------------");

		const char* deviceType = "Unknown";
		switch (props.deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: deviceType = "Discrete GPU"; break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: deviceType = "Integrated GPU"; break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: deviceType = "Virtual GPU"; break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU: deviceType = "CPU"; break;
		default: break;
		}
		RENDERX_INFO("  Type: {}", deviceType);

		const char* vendor = "Unknown";
		switch (props.vendorID) {
		case 0x1002: vendor = "AMD"; break;
		case 0x10DE: vendor = "NVIDIA"; break;
		case 0x8086: vendor = "Intel"; break;
		case 0x13B5: vendor = "ARM"; break;
		default: vendor = fmt::format("0x{:04X}", props.vendorID).c_str(); break;
		}
		RENDERX_INFO("  Vendor: {}", vendor);

		uint32_t major = VK_API_VERSION_MAJOR(props.apiVersion);
		uint32_t minor = VK_API_VERSION_MINOR(props.apiVersion);
		uint32_t patch = VK_API_VERSION_PATCH(props.apiVersion);
		RENDERX_INFO("  API Version: {}.{}.{}", major, minor, patch);

		RENDERX_INFO("  Driver Version: {}", props.driverVersion);

		uint64_t totalMemory = 0;
		for (uint32_t i = 0; i < mem.memoryHeapCount; i++) {
			if (mem.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				totalMemory += mem.memoryHeaps[i].size;
			}
		}
		RENDERX_INFO("  VRAM: {} MB", totalMemory / (1024 * 1024));

		RENDERX_INFO("  Queue Families:");
		for (uint32_t i = 0; i < info.queueFamilies.size(); i++) {
			const auto& family = info.queueFamilies[i];
			std::string capabilities;

			if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) capabilities += "Graphics ";
			if (family.queueFlags & VK_QUEUE_COMPUTE_BIT) capabilities += "Compute ";
			if (family.queueFlags & VK_QUEUE_TRANSFER_BIT) capabilities += "Transfer ";
			if (family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) capabilities += "SparseBinding ";

			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(info.device, i, m_Surface, &presentSupport);
			if (presentSupport) capabilities += "Present ";

			RENDERX_INFO("    [{}] Count: {}, Flags: {}", i, family.queueCount, capabilities);
		}

		RENDERX_INFO("  Features:");
		RENDERX_INFO("    Geometry Shader: {}", features.geometryShader ? "Yes" : "No");
		RENDERX_INFO("    Tessellation Shader: {}", features.tessellationShader ? "Yes" : "No");
		RENDERX_INFO("    Multi-Viewport: {}", features.multiViewport ? "Yes" : "No");
		RENDERX_INFO("    Sampler Anisotropy: {}", features.samplerAnisotropy ? "Yes" : "No");
		RENDERX_INFO("    Texture Compression BC: {}", features.textureCompressionBC ? "Yes" : "No");
		RENDERX_INFO("    Texture Compression ETC2: {}", features.textureCompressionETC2 ? "Yes" : "No");
		RENDERX_INFO("    Texture Compression ASTC: {}", features.textureCompressionASTC_LDR ? "Yes" : "No");
		RENDERX_INFO("    Fill Mode Non-Solid: {}", features.fillModeNonSolid ? "Yes" : "No");
		RENDERX_INFO("    Wide Lines: {}", features.wideLines ? "Yes" : "No");
		RENDERX_INFO("    Large Points: {}", features.largePoints ? "Yes" : "No");

		// Limits
		RENDERX_INFO("  Limits:");
		RENDERX_INFO("    Max Image Dimension 2D: {}", props.limits.maxImageDimension2D);
		RENDERX_INFO("    Max Framebuffer Width: {}", props.limits.maxFramebufferWidth);
		RENDERX_INFO("    Max Framebuffer Height: {}", props.limits.maxFramebufferHeight);
		RENDERX_INFO("    Max Viewport Dimensions: {}x{}",
			props.limits.maxViewportDimensions[0],
			props.limits.maxViewportDimensions[1]);
		RENDERX_INFO("    Max Bound Descriptor Sets: {}", props.limits.maxBoundDescriptorSets);
		RENDERX_INFO("    Max Per-Stage Descriptor Samplers: {}", props.limits.maxPerStageDescriptorSamplers);
		RENDERX_INFO("    Max Per-Stage Descriptor Uniform Buffers: {}", props.limits.maxPerStageDescriptorUniformBuffers);
		RENDERX_INFO("    Max Per-Stage Descriptor Storage Buffers: {}", props.limits.maxPerStageDescriptorStorageBuffers);
		RENDERX_INFO("    Max Per-Stage Descriptor Sampled Images: {}", props.limits.maxPerStageDescriptorSampledImages);
		RENDERX_INFO("    Max Compute Shared Memory Size: {} KB", props.limits.maxComputeSharedMemorySize / 1024);
		RENDERX_INFO("    Max Compute Work Group Invocations: {}", props.limits.maxComputeWorkGroupInvocations);
		RENDERX_INFO("    Timestamp Compute and Graphics: {}", props.limits.timestampComputeAndGraphics ? "Yes" : "No");

		// Extensions (show first 10)
		RENDERX_INFO("  Extensions ({} total):", info.extensions.size());
		size_t extensionShowCount = std::min<size_t>(10, info.extensions.size());
		for (size_t i = 0; i < extensionShowCount; i++) {
			RENDERX_INFO("    - {}", info.extensions[i].extensionName);
		}
		if (info.extensions.size() > 10) {
			RENDERX_INFO("    ... and {} more", info.extensions.size() - 10);
		}

		// Suitability
		// RENDERX_INFO("  Suitable: {}", info.suitable ? "Yes" : "No");
		RENDERX_INFO("  Score: {}\n", info.score);
	}

	uint32_t VulkanDevice::scoreDevice(const DeviceInfo& info) const {
		uint32_t score = 0;

		if (info.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 10000;
		}
		else if (info.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
			score += 5000;
		}

		score += info.properties.limits.maxImageDimension2D;

		uint64_t vram = 0;
		for (uint32_t i = 0; i < info.memoryProperties.memoryHeapCount; i++) {
			if (info.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				vram += info.memoryProperties.memoryHeaps[i].size;
			}
		}
		score += static_cast<uint32_t>(vram / (1024 * 1024));
		if (info.features.geometryShader) score += 100;
		if (info.features.tessellationShader) score += 100;
		if (info.features.samplerAnisotropy) score += 100;
		bool hasDedicatedCompute = false;
		bool hasDedicatedTransfer = false;
		for (const auto& family : info.queueFamilies) {
			if (HasFlag(family.queueFlags, VK_QUEUE_COMPUTE_BIT) &&
				!HasFlag(family.queueFlags, VK_QUEUE_GRAPHICS_BIT)) {
				hasDedicatedCompute = true;
			}

			if (HasFlag(family.queueFlags, VK_QUEUE_TRANSFER_BIT) &&
				!HasFlag(family.queueFlags, VK_QUEUE_GRAPHICS_BIT) &&
				!HasFlag(family.queueFlags, VK_QUEUE_COMPUTE_BIT)) {
				hasDedicatedTransfer = true;
			}
		}
		if (hasDedicatedCompute) score += 500;
		if (hasDedicatedTransfer) score += 250;
		return score;
	}


	int VulkanDevice::selectDevice(const std::vector<DeviceInfo>& devices) const {
#ifndef RX_DEBUG_BUILD
		int bestIndex = devices[0].index;
		uint32_t bestScore = devices[bestIndex].score;

		for (auto device : devices) {
			if (device.score > bestScore) {
				bestScore = device.score;
				bestIndex = device.index;
			}
		}

		RENDERX_INFO("Auto-selecting device [{}] (highest score)", bestIndex);
		return bestIndex;
#else
		RENDERX_INFO("Suitable devices:");
		for (auto idx : devices) {
			RENDERX_INFO("  {} {} - Score: {}",
				idx.index,
				idx.properties.deviceName,
				idx.score);
		}

		int i = 0, recommendedinfoIndex = 0;
		int recommendedIndex = devices[0].index;
		uint32_t bestScore = devices[0].score;

		for (auto idx : devices) {
			if (idx.score > bestScore) {
				bestScore = idx.score;
				recommendedIndex = idx.index;
				recommendedinfoIndex = i;
			}
			i++;
		}

		RENDERX_INFO("Recommended: {} (highest score)", devices[recommendedinfoIndex].properties.deviceName);
		std::string input;
		std::getline(std::cin, input);

		if (input.empty()) {
			return recommendedIndex;
		}

		try {
			int selectedIndex = std::stoi(input);
			int selectedInfoIndex = 0;
			bool valid = false;
			i = 0;
			for (auto idx : devices) {
				if (idx.index == selectedIndex) {
					selectedInfoIndex = i;
					valid = true;
					break;
				}
				i++;
			}

			if (valid) {
				RENDERX_INFO("Using selected device [{}]", devices[selectedInfoIndex].properties.deviceName);
				return selectedIndex;
			}
			else {
				RENDERX_WARN("Invalid device selection. Falling back to recommended device [{}]",
					devices[recommendedinfoIndex].properties.deviceName);
				return recommendedIndex;
			}
		}

		catch (const std::exception& e) {
			RENDERX_WARN("Exception  {} : Falling back to recommended device {}", devices[recommendedinfoIndex].properties.deviceName, e.what());
			return recommendedIndex;
		}

		return -1;
#endif
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

		// Find queue families
		m_GraphicsFamily = UINT32_MAX;
		m_ComputeFamily = UINT32_MAX;
		m_TransferFamily = UINT32_MAX;

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

		RENDERX_INFO("Graphics Queue: Family {}", m_GraphicsFamily);
		RENDERX_INFO("Compute Queue:  Family {}", m_ComputeFamily);
		RENDERX_INFO("Transfer Queue: Family {}", m_TransferFamily);

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

		// add here
		VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
		dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		dynamicRendering.dynamicRendering = VK_TRUE;

		VkPhysicalDeviceSynchronization2Features sync2{};
		sync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		sync2.synchronization2 = VK_TRUE;
		sync2.pNext = &dynamicRendering;

		VkPhysicalDeviceBufferDeviceAddressFeatures bufferAddressFeatures{};
		bufferAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		bufferAddressFeatures.bufferDeviceAddress = VK_TRUE;
		bufferAddressFeatures.pNext = &sync2;

		VkPhysicalDeviceTimelineSemaphoreFeatures timelineFeatures{};
		timelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
		timelineFeatures.pNext = &bufferAddressFeatures;
		timelineFeatures.timelineSemaphore = VK_TRUE;

		VkPhysicalDeviceDescriptorIndexingFeatures indexing{};
		indexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		indexing.pNext = &timelineFeatures;
		indexing.descriptorBindingPartiallyBound = VK_TRUE;
		indexing.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		indexing.runtimeDescriptorArray = VK_TRUE;
		indexing.descriptorBindingVariableDescriptorCount = VK_TRUE;


		VkDeviceCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		info.queueCreateInfoCount = uint32_t(queues.size());
		info.pQueueCreateInfos = queues.data();
		info.pEnabledFeatures = nullptr;
		info.pNext = &indexing;
		info.enabledExtensionCount = uint32_t(requiredExtensions.size());
		info.ppEnabledExtensionNames = requiredExtensions.data();
		info.enabledLayerCount = uint32_t(requiredLayers.size());
		info.ppEnabledLayerNames = requiredLayers.data();

		VK_CHECK(vkCreateDevice(m_PhysicalDevice, &info, nullptr, &m_Device));

		vkGetDeviceQueue(m_Device, m_GraphicsFamily, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, m_ComputeFamily, 0, &m_ComputeQueue);
		vkGetDeviceQueue(m_Device, m_TransferFamily, 0, &m_TransferQueue);

		RENDERX_INFO("Logical device created successfully");
		RENDERX_INFO("Queues retrieved successfully\n");
	}

	void VulkanDevice::queryLimits() {
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);
		m_Limits = props.limits;
	}

	VulkanDevice::VulkanDevice(
		VkInstance instance,
		VkSurfaceKHR surface,
		const std::vector<const char*>& requiredExtensions,
		const std::vector<const char*>& requiredLayers)
		: m_Instance(instance), m_Surface(surface) {
		std::vector<VkPhysicalDevice> devices;
		std::vector<DeviceInfo> infos;

		uint32_t count;
		vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);
		devices.resize(count);
		vkEnumeratePhysicalDevices(m_Instance, &count, devices.data());

		for (uint32_t i = 0; i < count; i++) {
			auto info = gatherDeviceInfo(devices[i]);
			info.score = scoreDevice(info);
			// logDeviceInfo(i, info);
			info.index = i;
			infos.push_back(info);
		}
		m_PhysicalDevice = devices[selectDevice(infos)];

		createLogicalDevice(requiredExtensions, requiredLayers);
		queryLimits();
	}

	VulkanDevice::~VulkanDevice() {
		if (m_Device) {
			vkDeviceWaitIdle(m_Device);
			vkDestroyDevice(m_Device, nullptr);
		}
	}
} // namespace Rx::RxVK
