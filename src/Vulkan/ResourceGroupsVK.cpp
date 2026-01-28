#include "CommonVK.h"

namespace Rx {
namespace RxVK {

	ResourceGroupLayoutHandle VKCreateResourceGroupLayout(const ResourceGroupLayoutDesc& desc) {
		// Store the created layout in a local registry and return a handle
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (auto& srgItem : desc.Resources) {
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = srgItem.binding;
			binding.descriptorCount = srgItem.count;
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
		VK_CHECK(vkCreateDescriptorSetLayout(ctx.device, &ci, nullptr, &setlayout));

		//temp
		return ResourceGroupLayoutHandle{};
	}

	void VKCmdSetResourceGroup(const CommandList& cmd, ResourceGroupHandle handle) {
		PROFILE_FUNCTION();
		RENDERX_ASSERT_MSG(handle.IsValid(), " vkCmdSetResourceGroup :Invalid handle");
		RENDERX_ASSERT_MSG(cmd.IsValid(), " vkCmdSetResourceGroup :Invalid CommandList");
		RENDERX_ASSERT_MSG(cmd.isOpen, " vkCmdSetResourceGroup :Command List is Not in Recording State , Use CommandList.open() to start Recording");
   
		auto ctx = GetVulkanContext();
		auto frame = GetCurrentFrameContex(g_CurrentFrame);

		//vkCmdBindDescriptorSets( frame.commandBuffers[cmd.id] , VK_PIPELINE_BIND_POINT_GRAPHICS , , )

	}

} // namespace RxVK

} // namespace  Rx
