#include "VK_Common.h"

namespace Rx {
namespace RxVK {

	struct VulkanSampler {
		VkSampler sampler = VK_NULL_HANDLE;
		bool isValid = false;
	};

	// cache
	std::unordered_map<Hash64, ResourceGroupHandle> g_ResourceGroupCache;
	// resource pools
	ResourcePool<VulkanResourceGroup, ResourceGroupHandle> g_TransientResourceGroupPool;
	ResourcePool<VulkanResourceGroup, ResourceGroupHandle> g_PersistentResourceGroupPool;


	static VkDescriptorPool g_PersistentDescriptorPool;

	bool CreatePersistentDescriptorPool() {
		// Predefined pool sizes based on lifetime
		static DescriptorPoolSizes kPersistentPoolSizes;
		kPersistentPoolSizes.uniformBufferCount = 10000;
		kPersistentPoolSizes.storageBufferCount = 5000;
		kPersistentPoolSizes.sampledImageCount = 40000;
		kPersistentPoolSizes.storageImageCount = 2000;
		kPersistentPoolSizes.samplerCount = 20000;
		kPersistentPoolSizes.combinedImageSamplerCount = 10000;
		kPersistentPoolSizes.maxSets = 10000;

		g_PersistentDescriptorPool = CreateDescriptorPool(kPersistentPoolSizes, false);

		if (g_PersistentDescriptorPool == VK_NULL_HANDLE) {
			RENDERX_ERROR("CreatePersistentDescriptorPool: Failed to create descriptor pool");
			return false;
		}
		RENDERX_ASSERT_MSG(g_PersistentDescriptorPool != VK_NULL_HANDLE, "CreatePersistentDescriptorPool: pool is null");
		return true;
	}

	inline Hash64 ComputeResourceGroupHash(const ResourceGroupDesc& desc) {
		Hash64 hash = 0xcbf29ce484222325;

		hash ^= desc.layout.id;
		hash *= 0x100000001b3;

		for (const auto& binding : desc.Resources) {
			hash ^= binding.binding;
			hash *= 0x100000001b3;

			hash ^= static_cast<uint64_t>(binding.type);
			hash *= 0x100000001b3;

			switch (binding.type) {
			case ResourceType::ConstantBuffer:
			case ResourceType::StorageBuffer:
			case ResourceType::RWStorageBuffer:
				hash ^= binding.bufferView.id;
				break;

			case ResourceType::Texture_SRV:
			case ResourceType::Texture_UAV:
				hash ^= binding.textureView.id;
				break;

			case ResourceType::Sampler:
				hash ^= binding.sampler.id;
				break;
			}

			hash *= 0x100000001b3;
		}

		return hash;
	}

	inline void WriteDescriptorSet(VkDescriptorSet& set, const ResourceGroupDesc& desc) {
		auto ctx = GetVulkanContext();
		RENDERX_ASSERT_MSG(ctx.device != VK_NULL_HANDLE, "WriteDescriptorSet: device is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(set != VK_NULL_HANDLE, "WriteDescriptorSet: descriptor set is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(desc.Resources.size() > 0, "WriteDescriptorSet: resources list is empty");

		std::vector<VkWriteDescriptorSet> writes;
		writes.reserve(desc.Resources.size());

		// Storage for descriptor infos (must outlive vkUpdateDescriptorSets)
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkDescriptorImageInfo> imageInfos;

		bufferInfos.reserve(desc.Resources.size());
		imageInfos.reserve(desc.Resources.size());

		for (const auto& binding : desc.Resources) {
			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = set;
			write.dstBinding = binding.binding;
			write.dstArrayElement = binding.arrayIndex;
			write.descriptorCount = 1;
			write.descriptorType = ToVkDescriptorType(binding.type);

			switch (binding.type) {
			//  Buffers
			case ResourceType::ConstantBuffer:
			case ResourceType::StorageBuffer:
			case ResourceType::RWStorageBuffer: {
				auto* view = g_BufferViewPool.get(binding.bufferView);
				RENDERX_ASSERT_MSG(view != nullptr, "WriteDescriptorSet: retrieved null buffer view");
				RENDERX_ASSERT_MSG(view && view->isValid,
					"WriteDescriptorSet: Invalid BufferView");

				auto* buffer = g_BufferPool.get(view->buffer);
				RENDERX_ASSERT_MSG(buffer != nullptr, "WriteDescriptorSet: retrieved null buffer");
				RENDERX_ASSERT_MSG(buffer,
					"WriteDescriptorSet: Invalid Buffer");
				RENDERX_ASSERT_MSG(buffer->buffer != VK_NULL_HANDLE, "WriteDescriptorSet: buffer is null");

				VkDescriptorBufferInfo info{};
				info.buffer = buffer->buffer;
				info.offset = view->offset;
				info.range = view->range;

				bufferInfos.push_back(info);
				write.pBufferInfo = &bufferInfos.back();
				break;
			}

			////  Textures
			// case ResourceType::Texture_SRV:
			// case ResourceType::Texture_UAV: {
			//	auto* view = g_TextureViewPool.get(binding.textureView);
			//	RENDERX_ASSERT_MSG(view && view->isValid,
			//		"WriteDescriptorSet: Invalid TextureView");

			//	VkDescriptorImageInfo info{};
			//	info.imageView = view->view;
			//	info.imageLayout =
			//		(binding.type == ResourceType::Texture_UAV)
			//			? VK_IMAGE_LAYOUT_GENERAL
			//			: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			//	imageInfos.push_back(info);
			//	write.pImageInfo = &imageInfos.back();
			//	break;
			//}

			//  Samplers
			case ResourceType::Sampler: {
				/*auto* sampler = g_SamplerPool.get(binding.sampler);
				RENDERX_ASSERT_MSG(sampler && sampler->isValid,
					"WriteDescriptorSet: Invalid Sampler");

				VkDescriptorImageInfo info{};
				info.sampler = sampler->sampler;

				imageInfos.push_back(info);
				write.pImageInfo = &imageInfos.back();
				break;*/
			}

			//  Combined
			case ResourceType::CombinedTextureSampler: {
				/*auto* view = g_TextureViewPool.get(binding.textureView);
				auto* sampler = g_SamplerPool.get(binding.sampler);

				RENDERX_ASSERT_MSG(view && view->isValid,
					"WriteDescriptorSet: Invalid TextureView");
				RENDERX_ASSERT_MSG(sampler && sampler->isValid,
					"WriteDescriptorSet: Invalid Sampler");

				VkDescriptorImageInfo info{};
				info.imageView = view->view;
				info.sampler = sampler->sampler;
				info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				imageInfos.push_back(info);
				write.pImageInfo = &imageInfos.back();
				break;*/
			}

			default:
				RENDERX_ASSERT_MSG(false, "WriteDescriptorSet: Unsupported ResourceType");
				continue;
			}

			writes.push_back(write);
		}

		if (!writes.empty()) {
			vkUpdateDescriptorSets(
				ctx.device,
				static_cast<uint32_t>(writes.size()),
				writes.data(),
				0,
				nullptr);
		}
	}

	inline ResourceGroupHandle CreatePersistentDescriptorSet(const ResourceGroupDesc& desc) {
		auto& frame = GetFrameContex(g_CurrentFrame);
		auto ctx = GetVulkanContext();


		VulkanPipelineLayout* layout = g_LayoutPool.get(desc.layout);
		RENDERX_ASSERT_MSG((layout->layout != VK_NULL_HANDLE), "ResourceGroup Layout is not valid");

		Hash64 hash = ComputeResourceGroupHash(desc);

		auto it = g_ResourceGroupCache.find(hash);
		if (it != g_ResourceGroupCache.end()) return it->second;

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout->setlayouts[desc.setIndex];
		allocInfo.descriptorPool = g_PersistentDescriptorPool;

		VkDescriptorSet set;
		auto res = vkAllocateDescriptorSets(ctx.device, &allocInfo, &set);

		// Pool exhausted - grow chain
		if (res == VK_ERROR_OUT_OF_POOL_MEMORY || res == VK_ERROR_FRAGMENTED_POOL) {
			// GrowPersistentPool();
			// pool = m_persistentPools[m_currentPersistentPoolIndex];
			// allocInfo.descriptorPool = pool;
			// result = vkAllocateDescriptorSets(m_device, &allocInfo, &set);
			RENDERX_CRITICAL("CreatePersistentDescriptorSet : descriptorPool Overflow");
		}

		if (res != VK_SUCCESS) {
			RENDERX_CRITICAL("CreatePersistentDescriptorSet: Cannot create Descriptor Set");
			return ResourceGroupHandle{};
		}

		WriteDescriptorSet(set, desc);

		VulkanResourceGroup resourceGroup{};
		resourceGroup.set = set;
		resourceGroup.setIndex = desc.setIndex;
		resourceGroup.pipelineLayout = desc.layout;
		resourceGroup.hash = hash;
		resourceGroup.isPersistent = true;
		auto handle = g_PersistentResourceGroupPool.allocate(resourceGroup);

		// Debug naming
		if (desc.debugName && desc.debugName[0] != '\0') {
			VkDebugUtilsObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
			nameInfo.objectHandle = (uint64_t)set;
			nameInfo.pObjectName = desc.debugName;
			// vkSetDebugUtilsObjectNameEXT(m_device, &nameInfo);
		}

		return handle;
	}

	inline ResourceGroupHandle CreateTransientDscriptorSet(const ResourceGroupDesc& desc) {
		auto& frame = GetFrameContex(g_CurrentFrame);
		auto ctx = GetVulkanContext();


		VulkanPipelineLayout* layout = g_LayoutPool.get(desc.layout);
		RENDERX_ASSERT_MSG((layout->layout != VK_NULL_HANDLE), "ResourceGroup Layout is not valid");

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout->setlayouts[desc.setIndex];
		allocInfo.descriptorPool = frame.DescriptorPool;

		VkDescriptorSet set;
		auto res = vkAllocateDescriptorSets(ctx.device, &allocInfo, &set);

		// Pool exhausted - grow chain
		if (res == VK_ERROR_OUT_OF_POOL_MEMORY || res == VK_ERROR_FRAGMENTED_POOL) {
			RENDERX_CRITICAL("CreateTransientDscriptorSet: Pool OverFlow");
			// GrowTransientPool();
		}

		if (res != VK_SUCCESS) {
			RENDERX_CRITICAL("CreateTransientDscriptorSet: Cannot create Descriptor Set");
			return ResourceGroupHandle{};
		}

		WriteDescriptorSet(set, desc);

		VulkanResourceGroup resourceGroup{};
		resourceGroup.set = set;
		resourceGroup.setIndex = desc.setIndex;
		resourceGroup.pipelineLayout = desc.layout;
		resourceGroup.hash = 0;
		resourceGroup.isPersistent = false;
		auto handle = g_TransientResourceGroupPool.allocate(resourceGroup);

		return handle;
	}

	ResourceGroupHandle VKCreateResourceGroup(const ResourceGroupDesc& desc) {
		switch (desc.flags) {
		case ResourceGroupLifetime::PerFrame:
			return CreateTransientDscriptorSet(desc);
		case ResourceGroupLifetime::Persistent:
			return CreatePersistentDescriptorSet(desc);
		default:
			return ResourceGroupHandle{};
		}
	}

	void VKDestroyResourceGroup(ResourceGroupHandle& handle) {
		if (!handle.IsValid()) return;
		auto ctx = GetVulkanContext();

		// Try to free from per-frame descriptor pools first
		auto* rg = g_TransientResourceGroupPool.get(handle);
		if (!rg) return;

		VkDescriptorSet set = rg->set;
		bool freed = false;
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			auto& frame = GetFrameContex(i);
			if (frame.DescriptorPool != VK_NULL_HANDLE) {
				VkResult res = vkFreeDescriptorSets(ctx.device, frame.DescriptorPool, 1, &set);
				if (res == VK_SUCCESS) {
					freed = true;
					break;
				}
			}
		}

		// If not freed from transient pools, try persistent pool (may or may not allow freeing)
		if (!freed && g_PersistentDescriptorPool != VK_NULL_HANDLE) {
			VkResult pres = vkFreeDescriptorSets(ctx.device, g_PersistentDescriptorPool, 1, &set);
			if (pres == VK_SUCCESS) freed = true;
		}

		// Remove from cache if present
		for (auto it = g_ResourceGroupCache.begin(); it != g_ResourceGroupCache.end(); ++it) {
			if (it->second == handle) {
				g_ResourceGroupCache.erase(it);
				break;
			}
		}

		// Free bookkeeping
		g_TransientResourceGroupPool.free(handle);
	}


	// Shutdown helper to cleanup persistent descriptor pool and caches
	void freeResourceGroups() {
		auto ctx = GetVulkanContext();
		// Clear cache
		g_ResourceGroupCache.clear();
		// Destroy persistent pool if created
		if (g_PersistentDescriptorPool != VK_NULL_HANDLE && ctx.device != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(ctx.device, g_PersistentDescriptorPool, nullptr);
			g_PersistentDescriptorPool = VK_NULL_HANDLE;
		}
		g_PersistentResourceGroupPool.clear();

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			auto frame = GetFrameContex(i);
			if (frame.DescriptorPool != VK_NULL_HANDLE && ctx.device != VK_NULL_HANDLE) {
				vkDestroyDescriptorPool(ctx.device, frame.DescriptorPool, nullptr);
			}
		}
	}

	void VKCmdSetResourceGroup(const CommandList& cmd, const ResourceGroupHandle& handle) {
		PROFILE_FUNCTION();
		RENDERX_ASSERT_MSG(handle.IsValid(), " vkCmdSetResourceGroup :Invalid handle");
		RENDERX_ASSERT_MSG(cmd.IsValid(), " vkCmdSetResourceGroup :Invalid CommandList");

		auto ctx = GetVulkanContext();
		auto frame = GetFrameContex(g_CurrentFrame);

		// Bind descriptor set to currently bound pipeline layout
		auto* vkCmdList = g_CommandListPool.get(cmd);
		RENDERX_ASSERT_MSG(vkCmdList && vkCmdList->isOpen, "vkCmdSetResourceGroup: CommandList not open or invalid");

		// try
		VulkanResourceGroup* rg = g_TransientResourceGroupPool.get(handle);
		if (!rg) rg = g_PersistentResourceGroupPool.get(handle);
		RENDERX_ASSERT_MSG(rg, "vkCmdSetResourceGroup: cannot  find ResourceGroup handle{}", handle.id);

		auto* pipelinLayout = g_LayoutPool.get(rg->pipelineLayout);

		VkDescriptorSet set = rg->set;
		RENDERX_ASSERT_MSG(set != VK_NULL_HANDLE, "vkCmdSetResourceGroup:");
		RENDERX_ASSERT_MSG(pipelinLayout, "vkCmdSetResourceGroup: Invalid pipeline layout")
		VkPipelineLayout layout = pipelinLayout->layout;
		RENDERX_ASSERT_MSG(layout != VK_NULL_HANDLE, "vkCmdSetResourceGroup: No pipeline layout bound. Call CmdSetPipeline first.");

		vkCmdBindDescriptorSets(
			vkCmdList->cmdBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			layout,
			rg->setIndex,
			1,
			&set,
			0,
			nullptr);
	}

} // namespace RxVK

} // namespace  Rx
