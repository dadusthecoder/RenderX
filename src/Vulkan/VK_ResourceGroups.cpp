#include "VK_Common.h"
#include "VK_RenderX.h"
namespace Rx {
namespace RxVK {

	struct VulkanSampler {
		VkSampler sampler = VK_NULL_HANDLE;
		bool isValid = false;
	};

	// cache
	std::unordered_map<Hash64, ResourceGroupHandle> g_ResourceGroupCache;

	// resource pools
	ResourcePool<VulkanResourceGroup, ResourceGroupHandle> g_ResourceGroupPool;
	ResourcePool<VulkanResourceGroupLayout, ResourceGroupLayoutHandle> g_ResourceGroupLayoutPool;

	inline Hash64 ComputeResourceGroupHash(const ResourceGroupDesc& desc) {
		Hash64 hash = 0xcbf29ce484222325;

		for (const auto& binding : desc.Resources) {
			hash ^= binding.binding;
			hash *= 0x100000001b3;

			hash ^= static_cast<uint64_t>(binding.type);
			hash *= 0x100000001b3;

			switch (binding.type) {
			case ResourceType::CONSTANT_BUFFER:
			case ResourceType::STORAGE_BUFFER:
			case ResourceType::RW_STORAGE_BUFFER:
				hash ^= binding.bufferView.id;
				break;

			case ResourceType::TEXTURE_SRV:
			case ResourceType::TEXTURE_UAV:
				hash ^= binding.textureView.id;
				break;

			case ResourceType::SAMPLER:
				hash ^= binding.sampler.id;
				break;
			}

			hash *= 0x100000001b3;
		}

		return hash;
	}

	VulkanDescriptorPoolManager::VulkanDescriptorPoolManager(VulkanContext& ctx)
		: m_Ctx(ctx) {
		// Persistent pool for things that never get deleted
		m_Persistent = createPool(8192, true, false);

		// Bindless pool with UpdateAfterBind flags
		m_Bindless = createPool(16384, false, true);

		// Gets Recycled every frame
		m_CurrentTransient.pool = createPool(1000, true, false);
	}

	VulkanDescriptorPoolManager::~VulkanDescriptorPoolManager() {
		for (auto& pool : m_usedPools) {
			if (pool.pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(m_Ctx.device->logical(), pool.pool, nullptr);
		}
		for (auto& pool : m_freePools) {
			if (pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(m_Ctx.device->logical(), pool, nullptr);
		}
		if (m_Bindless != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(m_Ctx.device->logical(), m_Bindless, nullptr);
		if (m_Persistent != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(m_Ctx.device->logical(), m_Persistent, nullptr);
		if (m_CurrentTransient.pool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(m_Ctx.device->logical(), m_CurrentTransient.pool, nullptr);
	}

	VkDescriptorPool VulkanDescriptorPoolManager::createPool(
		uint32_t maxSets,
		bool allowFree,
		bool updateAfterBind) {
		std::array<VkDescriptorPoolSize, 6> sizes = { {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxSets },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, maxSets },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxSets },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxSets },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxSets },
			{ VK_DESCRIPTOR_TYPE_SAMPLER, maxSets },
		} };

		VkDescriptorPoolCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		ci.maxSets = maxSets;
		ci.poolSizeCount = static_cast<uint32_t>(sizes.size());
		ci.pPoolSizes = sizes.data();
		ci.flags = (allowFree ? VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT : 0) |
				   (updateAfterBind ? VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT : 0);

		VkDescriptorPool pool;
		VK_CHECK(vkCreateDescriptorPool(m_Ctx.device->logical(), &ci, nullptr, &pool));
		return pool;
	}

	VkDescriptorPool VulkanDescriptorPoolManager::acquire(bool allowFree, bool updateAfterBind, uint64_t timelineValue) {
		if (updateAfterBind) return m_Bindless;
		if (!allowFree) return m_Persistent;

		// If we have an active transient pool, use it
		if (m_CurrentTransient.pool != VK_NULL_HANDLE) {
			return m_CurrentTransient.pool;
		}

		if (!m_freePools.empty()) {
			m_CurrentTransient.pool = m_freePools.back();
			m_freePools.pop_back();
			return m_CurrentTransient.pool;
		}

		// Nothing to reuse? Create a fresh one
		m_CurrentTransient.pool = createPool(1000, true, false);
		return m_CurrentTransient.pool;
	}

	// Call this when you submit the command buffer to the queue
	void VulkanDescriptorPoolManager::submit(uint64_t timelineValue) {
		if (m_CurrentTransient.pool == VK_NULL_HANDLE)
			return;

		m_CurrentTransient.submissionID = timelineValue;
		m_usedPools.push_back(m_CurrentTransient);
		m_CurrentTransient = {};
	}

	void VulkanDescriptorPoolManager::retire(uint64_t timelineValue) {
		for (auto it = m_usedPools.begin(); it != m_usedPools.end();) {
			if (timelineValue >= it->submissionID) {
				// this pool is done on a Queue now you can reset it
				// you cannot reset a pool if it is being used by the gpu queue
				// you cannot use this befor all the queues that are using this pool are Done
				// Hence Right now this can only scale to the one queue

				// TODO -- PROBLEM : This cannot be used by the multiple queues
				// SOLUTION -- HACK: Wait on all the queues that are using this pool before reseting

				vkResetDescriptorPool(m_Ctx.device->logical(), it->pool, 0);
				m_freePools.push_back(it->pool);
				it = m_usedPools.erase(it); // SAFE erase
			}
			else {
				++it;
			}
		}
	}

	void VulkanDescriptorPoolManager::resetPersistent() {
		vkResetDescriptorPool(m_Ctx.device->logical(), m_Persistent, 0);
	}

	void VulkanDescriptorPoolManager::resetBindless() {
		vkResetDescriptorPool(m_Ctx.device->logical(), m_Bindless, 0);
	}

	static DescriptorBindingModel ChooseModel(ResourceGroupFlags flags) {
		if (Has(flags, ResourceGroupFlags::BINDLESS))
			return DescriptorBindingModel::Bindless;
		if (Has(flags, ResourceGroupFlags::BUFFER))
			return DescriptorBindingModel::DescriptorBuffer;
		if (Has(flags, ResourceGroupFlags::DYNAMIC_UNIFORM))
			return DescriptorBindingModel::DynamicUniform;
		if (Has(flags, ResourceGroupFlags::DYNAMIC))
			return DescriptorBindingModel::Dynamic;
		return DescriptorBindingModel::Static;
	}

	VulkanDescriptorManager::VulkanDescriptorManager(VulkanContext& ctx)
		: m_Ctx(ctx) {}

	VulkanDescriptorManager::~VulkanDescriptorManager() {}

	VulkanResourceGroupLayout VulkanDescriptorManager::createLayout(const ResourceGroupLayoutDesc& layout) {
		VulkanResourceGroupLayout out{};
		out.model = ChooseModel(layout.flags);

		std::vector<VkDescriptorSetLayoutBinding> bindings;
		std::vector<VkDescriptorBindingFlags> flags;

		for (auto& b : layout.resourcebindings) {
			VkDescriptorSetLayoutBinding vk{};
			vk.binding = b.binding;
			vk.descriptorCount = b.count;
			vk.descriptorType = ToVulkanDescriptorType(b.type);
			vk.stageFlags = ToVulkanShaderStageFlags(b.stages);
			bindings.push_back(vk);

			VkDescriptorBindingFlags f = 0;
			if (out.model == DescriptorBindingModel::Bindless) {
				f |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
				f |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
			}
			flags.push_back(f);
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO
		};
		flagsInfo.bindingCount = (uint32_t)flags.size();
		flagsInfo.pBindingFlags = flags.data();

		VkDescriptorSetLayoutCreateInfo ci{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
		};
		ci.bindingCount = (uint32_t)bindings.size();
		ci.pBindings = bindings.data();

		if (out.model == DescriptorBindingModel::Bindless) {
			ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
			ci.pNext = &flagsInfo;
		}

		VK_CHECK(vkCreateDescriptorSetLayout(
			m_Ctx.device->logical(), &ci, nullptr, &out.layout));

		return out;
	}

	VulkanResourceGroup VulkanDescriptorManager::createGroup(
		const ResourceGroupLayoutHandle& handle,
		const ResourceGroupDesc& desc,
		uint64_t timelineValue) {
		VulkanResourceGroup group{};
		auto* layout = g_ResourceGroupLayoutPool.get(handle);
		group.model = layout->model;

		// Collect all descriptor infos upfront to keep pointers valid
		std::vector<VkDescriptorImageInfo> imageInfos;
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkWriteDescriptorSet> writes;

		imageInfos.reserve(desc.Resources.size());
		bufferInfos.reserve(desc.Resources.size());
		writes.reserve(desc.Resources.size());

		if (group.model == DescriptorBindingModel::Dynamic) {
			VkDescriptorSetAllocateInfo ai{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO
			};
			ai.descriptorPool = m_Ctx.descriptorPoolManager->acquire(true, false, timelineValue);
			ai.descriptorSetCount = 1;
			ai.pSetLayouts = &layout->layout;

			VK_CHECK(vkAllocateDescriptorSets(
				m_Ctx.device->logical(), &ai, &group.set));

			for (auto& r : desc.Resources) {
				VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				w.dstSet = group.set;
				w.dstBinding = r.binding;
				w.dstArrayElement = r.arrayIndex; // Support array indexing
				w.descriptorType = ToVulkanDescriptorType(r.type);
				w.descriptorCount = 1;

				switch (r.type) {
					// case ResourceType::Texture_SRV:
					// case ResourceType::Texture_UAV: {
					//	auto* texView = g_TextureViewPool.get(r.textureView);

					//	VkDescriptorImageInfo& ii = imageInfos.emplace_back();
					//	ii.imageView = texView->view;
					//	ii.imageLayout = (r.type == ResourceType::Texture_SRV)
					//						 ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					//						 : VK_IMAGE_LAYOUT_GENERAL;
					//	ii.sampler = VK_NULL_HANDLE;

					//	w.pImageInfo = &ii;
					//	break;
					//}

					// case ResourceType::Sampler: {
					//	auto* sampler = g_SamplerPool.get(r.sampler);

					//	VkDescriptorImageInfo& ii = imageInfos.emplace_back();
					//	ii.sampler = sampler->sampler;
					//	ii.imageView = VK_NULL_HANDLE;
					//	ii.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

					//	w.pImageInfo = &ii;
					//	break;
					//}

					// case ResourceType::CombinedTextureSampler: {
					//	auto* texView = g_TextureViewPool.get(r.combinedHandles.texture);
					//	auto* sampler = g_SamplerPool.get(r.combinedHandles.sampler);

					//	VkDescriptorImageInfo& ii = imageInfos.emplace_back();
					//	ii.imageView = texView->view;
					//	ii.sampler = sampler->sampler;
					//	ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					//	w.pImageInfo = &ii;
					//	break;
					//}

				case ResourceType::CONSTANT_BUFFER:
				case ResourceType::STORAGE_BUFFER:
				case ResourceType::RW_STORAGE_BUFFER: {
					auto* bufView = g_BufferViewPool.get(r.bufferView);
					auto* buf = g_BufferPool.get(bufView->buffer);

					VkDescriptorBufferInfo& bi = bufferInfos.emplace_back();
					bi.buffer = buf->buffer;
					bi.offset = bufView->offset;
					bi.range = (bufView->range == 0) ? VK_WHOLE_SIZE : bufView->range;

					w.pBufferInfo = &bi;
					break;
				}
				}

				writes.push_back(w);
			}

			if (!writes.empty()) {
				vkUpdateDescriptorSets(
					m_Ctx.device->logical(),
					(uint32_t)writes.size(), writes.data(), 0, nullptr);
			}
		}

		// BINDLESS PATH
		if (group.model == DescriptorBindingModel::Bindless) {
			VkDescriptorSetAllocateInfo ai{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO
			};
			ai.descriptorPool = m_Ctx.descriptorPoolManager->acquire(false, true, timelineValue);
			ai.descriptorSetCount = 1;
			ai.pSetLayouts = &layout->layout;

			VK_CHECK(vkAllocateDescriptorSets(
				m_Ctx.device->logical(), &ai, &group.set));

			for (auto& r : desc.Resources) {
				VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				w.dstSet = group.set;
				w.dstBinding = r.binding;
				w.dstArrayElement = r.arrayIndex; // Support array indexing
				w.descriptorType = ToVulkanDescriptorType(r.type);
				w.descriptorCount = 1;

				switch (r.type) {
					// case ResourceType::Texture_SRV:
					// case ResourceType::Texture_UAV: {
					//	auto* texView = g_TextureViewPool.get(r.textureView);

					//	VkDescriptorImageInfo& ii = imageInfos.emplace_back();
					//	ii.imageView = texView->view;
					//	ii.imageLayout = (r.type == ResourceType::Texture_SRV)
					//						 ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					//						 : VK_IMAGE_LAYOUT_GENERAL;
					//	ii.sampler = VK_NULL_HANDLE;

					//	w.pImageInfo = &ii;
					//	break;
					//}

					// case ResourceType::Sampler: {
					//	auto* sampler = g_SamplerPool.get(r.sampler);

					//	VkDescriptorImageInfo& ii = imageInfos.emplace_back();
					//	ii.sampler = sampler->sampler;
					//	ii.imageView = VK_NULL_HANDLE;
					//	ii.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

					//	w.pImageInfo = &ii;
					//	break;
					//}

					// case ResourceType::CombinedTextureSampler: {
					//	auto* texView = g_TextureViewPool.get(r.combinedHandles.texture);
					//	auto* sampler = g_SamplerPool.get(r.combinedHandles.sampler);

					//	VkDescriptorImageInfo& ii = imageInfos.emplace_back();
					//	ii.imageView = texView->view;
					//	ii.sampler = sampler->sampler;
					//	ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					//	w.pImageInfo = &ii;
					//	break;
					//}

				case ResourceType::CONSTANT_BUFFER:
				case ResourceType::STORAGE_BUFFER:
				case ResourceType::RW_STORAGE_BUFFER: {
					auto* bufView = g_BufferViewPool.get(r.bufferView);
					auto* buf = g_BufferPool.get(bufView->buffer);

					VkDescriptorBufferInfo& bi = bufferInfos.emplace_back();
					bi.buffer = buf->buffer;
					bi.offset = bufView->offset;
					bi.range = (bufView->range == 0) ? VK_WHOLE_SIZE : bufView->range;

					w.pBufferInfo = &bi;
					break;
				}
				}

				writes.push_back(w);
			}

			if (!writes.empty()) {
				vkUpdateDescriptorSets(
					m_Ctx.device->logical(),
					(uint32_t)writes.size(), writes.data(), 0, nullptr);
			}
		}

		// DYNAMIC UNIFORM PATH
		else if (group.model == DescriptorBindingModel::DynamicUniform) {
			VkDescriptorSetAllocateInfo ai{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO
			};
			ai.descriptorPool = m_Ctx.descriptorPoolManager->acquire(true, false, timelineValue);
			ai.descriptorSetCount = 1;
			ai.pSetLayouts = &layout->layout;

			VK_CHECK(vkAllocateDescriptorSets(
				m_Ctx.device->logical(), &ai, &group.set));

			for (auto& r : desc.Resources) {
				auto* bufView = g_BufferViewPool.get(r.bufferView);
				auto* buf = g_BufferPool.get(bufView->buffer);

				VkDescriptorBufferInfo& bi = bufferInfos.emplace_back();
				bi.buffer = buf->buffer;
				bi.offset = 0; // Offset provided at bind time
				bi.range = VK_WHOLE_SIZE;

				VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				w.dstSet = group.set;
				w.dstBinding = r.binding;
				w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				w.descriptorCount = 1;
				w.pBufferInfo = &bi;

				writes.push_back(w);
			}

			if (!writes.empty()) {
				vkUpdateDescriptorSets(
					m_Ctx.device->logical(),
					(uint32_t)writes.size(), writes.data(), 0, nullptr);
			}

			// Store dynamic offset from desc
			group.dynamicOffset = desc.dynamicOffset;
		}

		// DESCRIPTOR BUFFER PATH
		else if (group.model == DescriptorBindingModel::DescriptorBuffer) {
			// Calculate required size based on layout
			size_t descriptorSize = 256; // Query from device properties
			size_t totalSize = descriptorSize * desc.Resources.size();

			VkBufferCreateInfo bi{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			bi.size = totalSize;
			bi.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
					   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

			VmaAllocationCreateInfo ai{};
			ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

			VK_CHECK(vmaCreateBuffer(
				m_Ctx.allocator->handle(),
				&bi, &ai,
				&group.descriptorBuffer.buffer,
				&group.descriptorBuffer.allocation,
				nullptr));

			VkBufferDeviceAddressInfo addrInfo{
				VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO
			};
			addrInfo.buffer = group.descriptorBuffer.buffer;
			group.descriptorBuffer.address =
				vkGetBufferDeviceAddress(m_Ctx.device->logical(), &addrInfo);

			group.descriptorBuffer.stride = 0; // Set index for binding
		}

		// STATIC PATH
		else {
			// Similar to bindless but from static pool
			VkDescriptorSetAllocateInfo ai{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO
			};
			ai.descriptorPool = m_Ctx.descriptorPoolManager->acquire(false, false, timelineValue);
			ai.descriptorSetCount = 1;
			ai.pSetLayouts = &layout->layout;

			VK_CHECK(vkAllocateDescriptorSets(
				m_Ctx.device->logical(), &ai, &group.set));

			// Same write logic as bindless
			for (auto& r : desc.Resources) {
				VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				w.dstSet = group.set;
				w.dstBinding = r.binding;
				w.descriptorType = ToVulkanDescriptorType(r.type);
				w.descriptorCount = 1;

				switch (r.type) {
					/*case ResourceType::Texture_SRV:
					case ResourceType::Texture_UAV: {
						auto* texView = g_TextureViewPool.get(r.textureView);

						VkDescriptorImageInfo& ii = imageInfos.emplace_back();
						ii.imageView = texView->view;
						ii.imageLayout = (r.type == ResourceType::Texture_SRV)
											 ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
											 : VK_IMAGE_LAYOUT_GENERAL;
						ii.sampler = VK_NULL_HANDLE;

						w.pImageInfo = &ii;
						break;
					}*/

				case ResourceType::CONSTANT_BUFFER:
				case ResourceType::STORAGE_BUFFER:
				case ResourceType::RW_STORAGE_BUFFER: {
					auto* bufView = g_BufferViewPool.get(r.bufferView);
					auto* buf = g_BufferPool.get(bufView->buffer);

					VkDescriptorBufferInfo& bi = bufferInfos.emplace_back();
					bi.buffer = buf->buffer;
					bi.offset = bufView->offset;
					bi.range = (bufView->range == 0) ? VK_WHOLE_SIZE : bufView->range;

					w.pBufferInfo = &bi;
					break;
				}
				}

				writes.push_back(w);
			}

			if (!writes.empty()) {
				vkUpdateDescriptorSets(
					m_Ctx.device->logical(),
					(uint32_t)writes.size(), writes.data(), 0, nullptr);
			}
		}

		return group;
	}
	void VulkanDescriptorManager::bind(
		VkCommandBuffer cmd,
		VkPipelineLayout layout,
		uint32_t setIndex,
		const VulkanResourceGroup& group) {
		switch (group.model) {
		case DescriptorBindingModel::Static:
		case DescriptorBindingModel::Bindless:
		case DescriptorBindingModel::Dynamic:
			vkCmdBindDescriptorSets(
				cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				layout, setIndex, 1, &group.set, 0, nullptr);
			break;

		case DescriptorBindingModel::DynamicUniform:
			vkCmdBindDescriptorSets(
				cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				layout, setIndex, 1, &group.set,
				1, &group.dynamicOffset);
			break;

		case DescriptorBindingModel::DescriptorBuffer: {
			VkDescriptorBufferBindingInfoEXT bindingInfo{};
			bindingInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			bindingInfo.address = group.descriptorBuffer.address;
			bindingInfo.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			// vkCmdBindDescriptorBuffersEXT(cmd, 1, &bindingInfo);

			uint32_t bufferIndex = 0;
			VkDeviceSize offset = 0;
			// vkCmdSetDescriptorBufferOffsetsEXT(
			// cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			// yout, setIndex, 1, &bufferIndex, &offset);
			break;
		}
		}
	}

	ResourceGroupLayoutHandle VKCreateResourceGroupLayout(const ResourceGroupLayoutDesc& desc) {
		auto& ctx = GetVulkanContext();
		auto Layout = ctx.descriptorSetManager->createLayout(desc);
		RENDERX_ASSERT_MSG(Layout.layout != VK_NULL_HANDLE, "Failed to create resource group layout");
		auto handle = g_ResourceGroupLayoutPool.allocate(Layout);
		if (handle.IsValid()) return handle;
		RENDERX_CRITICAL("Failed to allocate ResourceGroupLayout handle; please report this issue");
		return ResourceGroupLayoutHandle(0);
	}

	ResourceGroupHandle VKCreateResourceGroup(const ResourceGroupDesc& desc) {
		auto& ctx = GetVulkanContext();
		auto* layout = g_ResourceGroupLayoutPool.get(desc.layout);

		RENDERX_ASSERT_MSG(layout != nullptr, "Invalid layout handle");

		// Determine pool based on flags
		ResourcePool<VulkanResourceGroup, ResourceGroupHandle>* pool;
		bool shouldCache = false;

		if (Has(layout->flags, ResourceGroupFlags::BINDLESS)) {
			pool = &g_ResourceGroupPool;
			shouldCache = true;
		}
		else if (Has(layout->flags, ResourceGroupFlags::DYNAMIC) ||
				 Has(layout->flags, ResourceGroupFlags::DYNAMIC_UNIFORM)) {
			pool = &g_ResourceGroupPool;
			shouldCache = false;
		}
		else { // Static
			pool = &g_ResourceGroupPool;
			shouldCache = true;
		}

		// Check cache for persistent groups
		if (shouldCache) {
			Hash64 hash = ComputeResourceGroupHash(desc);
			auto it = g_ResourceGroupCache.find(hash);
			if (it != g_ResourceGroupCache.end()) {
				return it->second;
			}
		}

		// Create the group
		uint64_t timelineValue; //= ctx.getCurrentTimelineValue();
		auto group = ctx.descriptorSetManager->createGroup(desc.layout, desc, timelineValue);

		auto handle = pool->allocate(group);

		// Cache if needed
		if (shouldCache) {
			Hash64 hash = ComputeResourceGroupHash(desc);
			g_ResourceGroupCache[hash] = handle;
		}

		return handle;
	}

	void VKDestroyResourceGroup(ResourceGroupHandle& handle) {
	}
	void VKResetPersistentResourceGroups() {
		auto& ctx = GetVulkanContext();
		ctx.descriptorPoolManager->resetPersistent();
	}

	void VKResetBindlessResourceGroups() {
		auto& ctx = GetVulkanContext();
		ctx.descriptorPoolManager->resetBindless();
	}

	void VKResetDynamicResourceGroups(uint64_t timelineValue) {
		auto& ctx = GetVulkanContext();
		ctx.descriptorPoolManager->retire(timelineValue);
	}

} // namespace RxVK

} // namespace  Rx
