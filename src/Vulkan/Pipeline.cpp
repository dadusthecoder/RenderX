#include "RenderX/Log.h"
#include "RenderXVK.h"
#include "CommonVK.h"

namespace Rx {

	// ===================== SHADER =====================
	ShaderHandle RxVK::VKCreateShader(const ShaderDesc& desc) {
		VkShaderModuleCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ci.codeSize = desc.bytecode.size();
		ci.pCode = reinterpret_cast<const uint32_t*>(desc.bytecode.data());

		VkShaderModule shaderModule;
		auto& ctx = RxVK::GetVulkanContext();

		if (vkCreateShaderModule(ctx.device, &ci, nullptr, &shaderModule) != VK_SUCCESS) {
			RENDERX_CRITICAL("Failed to create shader module");
			return {};
		}

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

	// ===================== GRAPHICS PIPELINE =====================
	PipelineHandle RxVK::VKCreateGraphicsPipeline(PipelineDesc& desc) {
		auto& ctx = GetVulkanContext();

		// ---------- Shader Stages ----------
		std::vector<VkPipelineShaderStageCreateInfo> stages;
		for (auto& sh : desc.shaders) {
			// Try ResourcePool first
			auto& shader = g_ShaderPool.get(sh);
			if (shader.shaderModule == VK_NULL_HANDLE) {
				RENDERX_CRITICAL("Invalid shader handle");
				return PipelineHandle{};
			}

			VkPipelineShaderStageCreateInfo stage{};
			stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.stage = ToVkShaderStage(shader.type);
			stage.module = shader.shaderModule;
			stage.pName = shader.entryPoint.c_str();
			stages.push_back(stage);
		}

		// ---------- Vertex Input ----------
		std::vector<VkVertexInputBindingDescription> bindings;
		for (auto& b : desc.vertexInputState.bindings) {
			bindings.push_back({ b.binding,
				b.stride,
				b.instanceData ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX });
		}

		std::vector<VkVertexInputAttributeDescription> attrs;
		for (auto& a : desc.vertexInputState.attributes) {
			attrs.push_back({ a.location,
				a.binding,
				ToVkFormat(a.datatype),
				a.offset });
		}

		VkPipelineVertexInputStateCreateInfo vertexInput{};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = (uint32_t)bindings.size();
		vertexInput.pVertexBindingDescriptions = bindings.data();
		vertexInput.vertexAttributeDescriptionCount = (uint32_t)attrs.size();
		vertexInput.pVertexAttributeDescriptions = attrs.data();

		// ---------- Input Assembly ----------
		VkPipelineInputAssemblyStateCreateInfo inputAsm{};
		inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAsm.topology = ToVkPrimitiveTopology(desc.primitiveType);

		// temp
		//  ---------- Viewport ----------
		VkViewport viewport{};
		viewport.width = (float)ctx.swapchainExtent.width;
		viewport.height = (float)ctx.swapchainExtent.height;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = ctx.swapchainExtent;

		VkPipelineViewportStateCreateInfo vp{};
		vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vp.viewportCount = 1;
		vp.pViewports = &viewport;
		vp.scissorCount = 1;
		vp.pScissors = &scissor;

		// ---------- Rasterizer ----------
		VkPipelineRasterizationStateCreateInfo rast{};
		rast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rast.polygonMode = ToVkPolygonMode(desc.rasterizer.fillMode);
		rast.cullMode = ToVkCullMode(desc.rasterizer.cullMode);
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
		depth.depthCompareOp = ToVkCompareOp(desc.depthStencil.depthFunc);

		auto& b = desc.blend;
		VkPipelineColorBlendAttachmentState blend{};
		blend.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;

		blend.blendEnable = b.enable ? VK_TRUE : VK_FALSE;

		// Color blending
		blend.srcColorBlendFactor = ToVkBlendFactor(b.srcColor);
		blend.dstColorBlendFactor = ToVkBlendFactor(b.dstColor);
		blend.colorBlendOp = ToVkBlendOp(b.colorOp);

		// Alpha blending
		blend.srcAlphaBlendFactor = ToVkBlendFactor(b.srcAlpha);
		blend.dstAlphaBlendFactor = ToVkBlendFactor(b.dstAlpha);
		blend.alphaBlendOp = ToVkBlendOp(b.alphaOp);

		VkPipelineColorBlendStateCreateInfo cb{};
		cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cb.logicOpEnable = VK_FALSE;
		cb.attachmentCount = 1;
		cb.pAttachments = &blend;
		cb.blendConstants[0] = b.blendFactor.r;
		cb.blendConstants[1] = b.blendFactor.g;
		cb.blendConstants[2] = b.blendFactor.b;
		cb.blendConstants[3] = b.blendFactor.a;

		// ---------- Pipeline Layout ----------
		VkPipelineLayout layout;
		VkPipelineLayoutCreateInfo lci{};
		lci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		VkResult lres = vkCreatePipelineLayout(ctx.device, &lci, nullptr, &layout);
		if (lres != VK_SUCCESS) {
			RENDERX_ERROR("Failed to create pipeline layout: {}", VkResultToString(lres));
			return {};
		}

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
		pci.renderPass = GetVulkanRenderPass(desc.renderPass);
		pci.layout = layout;

		VkPipeline pipeline;
		if (vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pci, nullptr, &pipeline) != VK_SUCCESS) {
			RENDERX_CRITICAL("Failed to create graphics pipeline");
			// destroy the pipeline layout to avoid leaking it
			if (layout != VK_NULL_HANDLE) {
				vkDestroyPipelineLayout(ctx.device, layout, nullptr);
			}
			return {};
		}

		auto handle = g_PipelinePool.allocate(pipeline);
		RENDERX_INFO("Created Vulkan Graphics Pipeline with ID {}", handle.id);
		return handle;
	}

	// command List Functions

	void RxVK::VKCmdSetPipeline(CommandList& cmdList, PipelineHandle pipeline) {
		PROFILE_FUNCTION();

		RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid CommandList");
		RENDERX_ASSERT_MSG(cmdList.isOpen, "CommandList must be open");

		auto& frame = GetCurrentFrameContex(g_CurrentFrame);
		// temp vkBindPoint
		vkCmdBindPipeline(frame.commandBuffers[cmdList.id], VK_PIPELINE_BIND_POINT_GRAPHICS, g_PipelinePool.get(pipeline));
	}

} // namespace Rx
