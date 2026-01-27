#include "CommonVK.h"

namespace RenderX {
namespace RenderXVK {

	inline VkDescriptorType ToVkDescriptorType(ResourceType type) {
		switch (type) {
		case ResourceType::ConstantBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		case ResourceType::StorageBuffer:
		case ResourceType::RWStorageBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		case ResourceType::Texture_SRV:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

		case ResourceType::Texture_UAV:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		case ResourceType::Sampler:
			return VK_DESCRIPTOR_TYPE_SAMPLER;

		case ResourceType::CombinedTextureSampler:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		case ResourceType::AccelerationStructure:
			return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

		default:
			RENDERX_ERROR("Unknown ResourceType");
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}
	}

	inline VkShaderStageFlags ToVkShaderStageFlags(ShaderStage stages) {
		VkShaderStageFlags vkStages = 0;

		if (Has(stages, ShaderStage::Vertex))
			vkStages |= VK_SHADER_STAGE_VERTEX_BIT;

		if (Has(stages, ShaderStage::Fragment))
			vkStages |= VK_SHADER_STAGE_FRAGMENT_BIT;

		if (Has(stages, ShaderStage::Geometry))
			vkStages |= VK_SHADER_STAGE_GEOMETRY_BIT;

		if (Has(stages, ShaderStage::TessControl))
			vkStages |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

		if (Has(stages, ShaderStage::TessEvaluation))
			vkStages |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

		if (Has(stages, ShaderStage::Compute))
			vkStages |= VK_SHADER_STAGE_COMPUTE_BIT;

		return vkStages;
	}


	SRGLayoutHandle VKCreateSRGLayout(const SRGLayoutDesc& desc) {

    std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (auto& srgItem : desc.Resources) {
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = srgItem.binding;
			binding.descriptorCount = 1;
			binding.stageFlags = ToVkShaderStageFlags(srgItem.stages);
			binding.descriptorType = ToVkDescriptorType(srgItem.type);

			// temp
			// binding.pImmutableSamplers = srgItem.
            bindings.push_back(binding);
		}

		VkDescriptorSetLayoutCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		ci.flags = 0;
		ci.bindingCount = (uint32_t)desc.Resources.size();
		ci.pBindings = bindings.data();
        
        auto& ctx = GetVulkanContext();

         VkDescriptorSetLayout setlayout;
        VK_CHECK(vkCreateDescriptorSetLayout(ctx.device ,&ci , nullptr ,&setlayout));

	}

} // namespace RenderXVK

} // namespace  RenderX
