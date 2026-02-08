#include "RenderX/Log.h"
#include "VK_RenderX.h"
#include "VK_Common.h"

namespace Rx {

	namespace RxVK {

		ResourcePool<VulkanPipelineLayout, PipelineLayoutHandle> g_PipelineLayoutPool;
	} // namespace RxVK


	// ==== SHADER ====
	ShaderHandle RxVK::VKCreateShader(const ShaderDesc& desc) {
		RENDERX_ASSERT_MSG(desc.bytecode.size() > 0, "VKCreateShader: bytecode is empty");
		RENDERX_ASSERT_MSG(desc.bytecode.size() % sizeof(uint32_t) == 0, "VKCreateShader: bytecode size not aligned to uint32_t");
		RENDERX_ASSERT_MSG(!desc.entryPoint.empty(), "VKCreateShader: entryPoint is empty");

		VkShaderModuleCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ci.codeSize = desc.bytecode.size();
		ci.pCode = reinterpret_cast<const uint32_t*>(desc.bytecode.data());

		VkShaderModule shaderModule;
		auto& ctx = RxVK::GetVulkanContext();
		RENDERX_ASSERT_MSG(ctx.device && ctx.device->logical() != VK_NULL_HANDLE, "VKCreateShader: device is VK_NULL_HANDLE");

		VkResult result = vkCreateShaderModule(ctx.device->logical(), &ci, nullptr, &shaderModule);
		if (!CheckVk(result, "VKCreateShader: Failed to create shader module")) {
			return ShaderHandle{};
		}
		RENDERX_ASSERT_MSG(shaderModule != VK_NULL_HANDLE, "VKCreateShader: created shader module is null");

		// Use ResourcePool for shader management
		VulkanShader shader{ desc.entryPoint, desc.type, shaderModule };
		ShaderHandle handle = RxVK::g_ShaderPool.allocate(shader);

		return handle;
	}

	// void RxVK::VKDestroyShader(ShaderHandle handle) {
	// 	auto it = RxVK::s_Shaders.find(handle.id);
	// 	if (it == RxVK::s_Shaders.end()) return;

	// 	auto& ctx = RxVK::GetVulkanContext();
	// 	vkDestroyShaderModule(ctx.device, it->second.shaderModule, nullptr);
	// 	RxVK::s_Shaders.erase(it);
	// }

	// Pipeline LAyout Creation
	PipelineLayoutHandle RxVK::VKCreatePipelineLayout(const ResourceGroupLayoutHandle* playouts, uint32_t layoutcount) {
		// Store the created layout in a local registry and return a handle
		RENDERX_ASSERT_MSG(layoutcount > 0, "VKCreatePipelineLayout: layout count is zero");
		RENDERX_ASSERT_MSG(playouts != nullptr, "VKCreatePipelineLayout: playouts pointer is null");
		auto& ctx = GetVulkanContext();
		RENDERX_ASSERT_MSG(ctx.device && ctx.device->logical() != VK_NULL_HANDLE, "VKCreatePipelineLayout: device is VK_NULL_HANDLE");

		std::vector<VkDescriptorSetLayout> setLayouts;

		for (int i = 0; i < layoutcount; ++i) {
			// auto layout = g_Re
		}

		// ---------- Pipeline Layout ----------
		VkPipelineLayout pipelinelayout;
		VkPipelineLayoutCreateInfo lci{};
		lci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		lci.setLayoutCount = (uint32_t)setLayouts.size();
		lci.pSetLayouts = setLayouts.data();
		VkResult lres = vkCreatePipelineLayout(ctx.device->logical(), &lci, nullptr, &pipelinelayout);
		if (lres != VK_SUCCESS) {
			RENDERX_ERROR("VKCreatePipelineLayout: Failed to create pipeline layout: {}", VkResultToString(lres));
			return PipelineLayoutHandle{};
		}
		RENDERX_ASSERT_MSG(pipelinelayout != VK_NULL_HANDLE, "VKCreatePipelineLayout: created pipeline layout is null");

		// Debug naming
		// if (desc.&& desc.debugName[0] != '\0') {
		//	VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		//	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		//	nameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
		//	nameInfo.objectHandle = (uint64_t)pipelinelayout;
		//	nameInfo.pObjectName = desc.debugName;
		//	// vkSetDebugUtilsObjectNameEXT(m_device, &nameInfo);
		//}

		VulkanPipelineLayout rxPipelneLayout{};
		rxPipelneLayout.layout = pipelinelayout;
		rxPipelneLayout.setlayouts = setLayouts;

		// handle
		auto handle = g_PipelineLayoutPool.allocate(rxPipelneLayout);
		return handle;
	}

	// ==== GRAPHICS PIPELINE ====
	PipelineHandle RxVK::VKCreateGraphicsPipeline(PipelineDesc& desc) {
		auto& ctx = GetVulkanContext();

		// ---------- Shader Stages ----------
		std::vector<VkPipelineShaderStageCreateInfo> stages;
		for (auto& sh : desc.shaders) {
			// Try ResourcePool first
			auto* shader = g_ShaderPool.get(sh);
			if (!shader || shader->shaderModule == VK_NULL_HANDLE) {
				RENDERX_CRITICAL("Invalid shader handle");
				return PipelineHandle{};
			}

			VkPipelineShaderStageCreateInfo stage{};
			stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.stage = ToVulkanShaderStage(shader->type);
			stage.module = shader->shaderModule;
			stage.pName = shader->entryPoint.c_str();
			stages.push_back(stage);
		}

		// ---------- Vertex Input ----------
		std::vector<VkVertexInputBindingDescription> vertexBindings;
		for (auto& b : desc.vertexInputState.vertexBindings) {
			vertexBindings.push_back({ b.binding,
				b.stride,
				b.instanceData ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX });
		}

		std::vector<VkVertexInputAttributeDescription> attrs;
		for (auto& a : desc.vertexInputState.attributes) {
			attrs.push_back({ a.location,
				a.binding,
				ToVulkanFormat(a.format),
				a.offset });
		}

		VkPipelineVertexInputStateCreateInfo vertexInput{};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = (uint32_t)vertexBindings.size();
		vertexInput.pVertexBindingDescriptions = vertexBindings.data();
		vertexInput.vertexAttributeDescriptionCount = (uint32_t)attrs.size();
		vertexInput.pVertexAttributeDescriptions = attrs.data();

		// ---------- Input Assembly ----------
		VkPipelineInputAssemblyStateCreateInfo inputAsm{};
		inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAsm.topology = ToVulkanTopology(desc.primitiveType);

		// temp
		//  ---------- Viewport ----------
		VkViewport viewport{};
		viewport.width = (float)ctx.swapchain->extent().width;
		viewport.height = (float)ctx.swapchain->extent().height;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = ctx.swapchain->extent();

		VkPipelineViewportStateCreateInfo vp{};
		vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vp.viewportCount = 1;
		vp.pViewports = &viewport;
		vp.scissorCount = 1;
		vp.pScissors = &scissor;

		// ---------- Rasterizer ----------
		VkPipelineRasterizationStateCreateInfo rast{};
		rast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rast.polygonMode = ToVulkanPolygonMode(desc.rasterizer.fillMode);
		rast.cullMode = ToVulkanCullMode(desc.rasterizer.cullMode);
		rast.frontFace = desc.rasterizer.frontCounterClockwise
							 ? VK_FRONT_FACE_COUNTER_CLOCKWISE
							 : VK_FRONT_FACE_CLOCKWISE;
		rast.lineWidth = 1.0f;

		// ---------- Multisample ----------
		VkPipelineMultisampleStateCreateInfo ms{};
		ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// ---------- Depth ----------
		VkPipelineDepthStencilStateCreateInfo depth{};
		depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth.depthTestEnable = desc.depthStencil.depthEnable;
		depth.depthWriteEnable = desc.depthStencil.depthWriteEnable;
		depth.depthCompareOp = ToVulkanCompareOp(desc.depthStencil.depthFunc);


		auto& b = desc.blend;
		VkPipelineColorBlendAttachmentState blend{};
		blend.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;

		blend.blendEnable = b.enable ? VK_TRUE : VK_FALSE;

		// Color blending
		blend.srcColorBlendFactor = ToVulkanBlendFactor(b.srcColor);
		blend.dstColorBlendFactor = ToVulkanBlendFactor(b.dstColor);
		blend.colorBlendOp = ToVulkanBlendOp(b.colorOp);

		// Alpha blending
		blend.srcAlphaBlendFactor = ToVulkanBlendFactor(b.srcAlpha);
		blend.dstAlphaBlendFactor = ToVulkanBlendFactor(b.dstAlpha);
		blend.alphaBlendOp = ToVulkanBlendOp(b.alphaOp);

		VkPipelineColorBlendStateCreateInfo cb{};
		cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cb.logicOpEnable = VK_FALSE;
		cb.attachmentCount = 1;
		cb.pAttachments = &blend;
		cb.blendConstants[0] = b.blendFactor.r;
		cb.blendConstants[1] = b.blendFactor.g;
		cb.blendConstants[2] = b.blendFactor.b;
		cb.blendConstants[3] = b.blendFactor.a;

		// ---------- Create Pipeline ----------
		VkGraphicsPipelineCreateInfo pci{};
		pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pci.stageCount = (uint32_t)stages.size();
		pci.pStages = stages.data();
		pci.pVertexInputState = &vertexInput;
		pci.pInputAssemblyState = &inputAsm;
		pci.pViewportState = &vp;
		pci.pRasterizationState = &rast;
		pci.pMultisampleState = &ms;
		pci.pDepthStencilState = &depth;
		pci.pColorBlendState = &cb;
		pci.renderPass = *g_RenderPassPool.get(desc.renderPass);
		auto* layout = g_PipelineLayoutPool.get(desc.layout);
		pci.layout = layout->layout;

		VkPipeline pipeline;
		if (vkCreateGraphicsPipelines(ctx.device->logical(), VK_NULL_HANDLE, 1, &pci, nullptr, &pipeline) != VK_SUCCESS) {
			RENDERX_CRITICAL("Failed to create graphics pipeline");
			// destroy the pipeline layout to avoid leaking it
			if (layout->layout != VK_NULL_HANDLE) {
				vkDestroyPipelineLayout(ctx.device->logical(), layout->layout, nullptr);
				layout->isBound = false;
				g_PipelineLayoutPool.free(desc.layout);
			}
			return {};
		}

		VulkanPipeline vkpipe{};
		vkpipe.pipeline = pipeline;
		vkpipe.layout = desc.layout;

		auto handle = g_PipelinePool.allocate(vkpipe);
		RENDERX_INFO("Created Vulkan Graphics Pipeline with ID {}", handle.id);
		return handle;
	}

} // namespace Rx
