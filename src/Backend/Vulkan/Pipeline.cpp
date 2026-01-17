#include "RenderX/Log.h"
#include "RenderXVK.h"
#include "CommonVK.h"

namespace RenderX {

	VkFormat TOVKFormat(DataFormat format) {
		switch (format) {
		case DataFormat::R8: return VK_FORMAT_R8_UNORM;
		case DataFormat::RG8: return VK_FORMAT_R8G8_UNORM;
		case DataFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;

		// NOTE: Vulkan has NO RGB8 format for vertex / textures
		case DataFormat::RGB8:
			RENDERX_ERROR("RGB8 is not supported in Vulkan. Use RGBA8 instead.");
			return VK_FORMAT_UNDEFINED;

		case DataFormat::R16F: return VK_FORMAT_R16_SFLOAT;
		case DataFormat::RG16F: return VK_FORMAT_R16G16_SFLOAT;
		case DataFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;

		// No native RGB16F
		case DataFormat::RGB16F:
			RENDERX_ERROR("RGB16F is not supported in Vulkan. Use RGBA16F instead.");
			return VK_FORMAT_UNDEFINED;

		case DataFormat::R32F: return VK_FORMAT_R32_SFLOAT;
		case DataFormat::RG32F: return VK_FORMAT_R32G32_SFLOAT;
		case DataFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;

		// Vulkan supports this ONLY for vertex buffers (not textures)
		case DataFormat::RGB32F:
			return VK_FORMAT_R32G32B32_SFLOAT;

		default:
			RENDERX_ERROR("Unknown DataFormat");
			return VK_FORMAT_UNDEFINED;
		}
	}

	VkFormat TOVKTextureFormat(TextureFormat format) {
		switch (format) {
		case TextureFormat::R8: return VK_FORMAT_R8_UNORM;
		case TextureFormat::RG8: return VK_FORMAT_R8G8_UNORM;
		case TextureFormat::RGB8: return VK_FORMAT_R8G8B8_UNORM;

		case TextureFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
		case TextureFormat::R16F: return VK_FORMAT_R16_SFLOAT;
		case TextureFormat::RG16F: return VK_FORMAT_R16G16_SFLOAT;
		case TextureFormat::RGB16F: return VK_FORMAT_R16G16B16_SFLOAT;
		case TextureFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case TextureFormat::R32F: return VK_FORMAT_R32_SFLOAT;
		case TextureFormat::RG32F: return VK_FORMAT_R32G32_SFLOAT;
		case TextureFormat::RGB32F: return VK_FORMAT_R32G32B32_SFLOAT;
		case TextureFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case TextureFormat::Depth16: return VK_FORMAT_D16_UNORM;
		case TextureFormat::Depth24: return VK_FORMAT_D24_UNORM_S8_UINT;
		case TextureFormat::Depth32F: return VK_FORMAT_D32_SFLOAT;
		case TextureFormat::Depth24Stencil8: return VK_FORMAT_D24_UNORM_S8_UINT;
		case TextureFormat::Depth32FStencil8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		// Compressed formats
		case TextureFormat::BC1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case TextureFormat::BC2: return VK_FORMAT_BC2_UNORM_BLOCK;
		case TextureFormat::BC3: return VK_FORMAT_BC3_UNORM_BLOCK;
		case TextureFormat::BC4: return VK_FORMAT_BC4_UNORM_BLOCK;
		case TextureFormat::BC5: return VK_FORMAT_BC5_UNORM_BLOCK;
		default:
			RENDERX_ERROR("Unknown TextureFormat");
			return VK_FORMAT_UNDEFINED;
		}
	}

	VkShaderStageFlagBits TOVKShaderStage(ShaderType type) {
		switch (type) {
		case ShaderType::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderType::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderType::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
		case ShaderType::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderType::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case ShaderType::TessEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		default:
			RENDERX_ERROR("Unknown ShaderType");
			return VK_SHADER_STAGE_VERTEX_BIT;
		}
	}

	VkPrimitiveTopology TOVKPrimitiveTopology(PrimitiveType type) {
		switch (type) {
		case PrimitiveType::Points: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case PrimitiveType::Lines: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case PrimitiveType::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case PrimitiveType::Triangles: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case PrimitiveType::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case PrimitiveType::TriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		default:
			RENDERX_ERROR("Unknown PrimitiveType");
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}

	VkCullModeFlags TOVKCullMode(CullMode mode) {
		switch (mode) {
		case CullMode::None: return VK_CULL_MODE_NONE;
		case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
		case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
		case CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
		default:
			RENDERX_ERROR("Unknown CullMode");
			return VK_CULL_MODE_BACK_BIT;
		}
	}

	VkCompareOp TOVKCompareOp(CompareFunc func) {
		switch (func) {
		case CompareFunc::Never: return VK_COMPARE_OP_NEVER;
		case CompareFunc::Less: return VK_COMPARE_OP_LESS;
		case CompareFunc::Equal: return VK_COMPARE_OP_EQUAL;
		case CompareFunc::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CompareFunc::Greater: return VK_COMPARE_OP_GREATER;
		case CompareFunc::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
		case CompareFunc::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case CompareFunc::Always: return VK_COMPARE_OP_ALWAYS;
		default:
			RENDERX_ERROR("Unknown CompareFunc");
			return VK_COMPARE_OP_ALWAYS;
		}
	}

	VkPolygonMode TOVKPolygonMode(FillMode mode) {
		switch (mode) {
		case RenderX::FillMode::Solid:
			return VK_POLYGON_MODE_FILL;
			break;
		case RenderX::FillMode::Wireframe:
			return VK_POLYGON_MODE_LINE;
			break;
		case RenderX::FillMode::Point:
			return VK_POLYGON_MODE_POINT;
			break;
		default:
			return VK_POLYGON_MODE_FILL;
			break;
		}
	}
	// ===================== RENDERXVK =====================

	namespace RenderXVK {

		std::unordered_map<uint32_t, VulkanShader> s_Shaders;
		static std::unordered_map<uint32_t, VkPipeline> s_Pipelines;
		static std::unordered_map<uint32_t, VkPipelineLayout> s_PipelineLayouts;
		static std::unordered_map<uint32_t, VkRenderPass> s_RenderPasses;

		static uint32_t s_NextRenderPassId = 1;
		static uint32_t s_NextShaderId = 1;
		static uint32_t s_NextPipelineId = 1;

		//====================== RENDERPASS =====================
		VkRenderPass GetVulkanRenderPass(RenderPassHandle handle) {
			auto it = s_RenderPasses.find(handle.id);
			if (it == s_RenderPasses.end())
				return VK_NULL_HANDLE;

			return it->second;
		}

		RenderPassHandle CreateRederPassVK(const RenderPassDesc& desc) {
			auto& ctx = GetVulkanContext();

			std::vector<VkAttachmentDescription> attachments;
			std::vector<VkAttachmentReference> colorRefs;

			// ---------- Color Attachments ----------
			for (uint32_t i = 0; i < desc.colorAttachments.size(); i++) {
				const auto& color = desc.colorAttachments[i];

				VkAttachmentDescription att{};
				att.format = TOVKTextureFormat(color.format);
				att.samples = VK_SAMPLE_COUNT_1_BIT;
				att.loadOp = color.clear
								 ? VK_ATTACHMENT_LOAD_OP_CLEAR
								 : VK_ATTACHMENT_LOAD_OP_LOAD;
				att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

				attachments.push_back(att);

				VkAttachmentReference ref{};
				ref.attachment = i;
				ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorRefs.push_back(ref);
			}

			// ---------- Subpass ----------
			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = (uint32_t)colorRefs.size();
			subpass.pColorAttachments = colorRefs.data();

			// ---------- Dependency ----------
			VkSubpassDependency dependency{};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			// ---------- Render Pass ----------
			VkRenderPassCreateInfo rpci{};
			rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			rpci.attachmentCount = (uint32_t)attachments.size();
			rpci.pAttachments = attachments.data();
			rpci.subpassCount = 1;
			rpci.pSubpasses = &subpass;
			rpci.dependencyCount = 1;
			rpci.pDependencies = &dependency;

			VkRenderPass renderPass;
			if (vkCreateRenderPass(ctx.device, &rpci, nullptr, &renderPass) != VK_SUCCESS) {
				RENDERX_CRITICAL("Failed to create render pass");
				return {};
			}

			RenderPassHandle handle{ s_NextRenderPassId++ };
			s_RenderPasses[handle.id] = renderPass;
			return handle;
		}

	} // namespace RenderXVK


	// ===================== SHADER =====================
	ShaderHandle RenderXVK::VKCreateShader(const ShaderDesc& desc) {
		VkShaderModuleCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ci.codeSize = desc.bytecode.size();
		ci.pCode = reinterpret_cast<const uint32_t*>(desc.bytecode.data());

		VkShaderModule shaderModule;
		auto& ctx = RenderXVK::GetVulkanContext();

		if (vkCreateShaderModule(ctx.device, &ci, nullptr, &shaderModule) != VK_SUCCESS) {
			RENDERX_CRITICAL("Failed to create shader module");
			return {};
		}

		ShaderHandle handle{ RenderXVK::s_NextShaderId++ };
		RenderXVK::s_Shaders[handle.id] = { desc.entryPoint, desc.type, shaderModule };
		return handle;
	}

	void VKDestroyShader(ShaderHandle handle) {
		auto it = RenderXVK::s_Shaders.find(handle.id);
		if (it == RenderXVK::s_Shaders.end()) return;

		auto& ctx = RenderXVK::GetVulkanContext();
		vkDestroyShaderModule(ctx.device, it->second.shaderModule, nullptr);
		RenderXVK::s_Shaders.erase(it);
	}

	// ===================== GRAPHICS PIPELINE =====================
	const PipelineHandle RenderXVK::VKCreateGraphicsPipeline(PipelineDesc& desc) {
		auto& ctx = GetVulkanContext();

		// ---------- Shader Stages ----------
		std::vector<VkPipelineShaderStageCreateInfo> stages;
		for (auto& sh : desc.shaders) {
			auto it = s_Shaders.find(sh.id);
			if (it == s_Shaders.end()) {
				RENDERX_CRITICAL("Invalid shader handle");
				return {};
			}

			VkPipelineShaderStageCreateInfo stage{};
			stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.stage = TOVKShaderStage(it->second.type);
			stage.module = it->second.shaderModule;
			stage.pName = it->second.entryPoint.c_str();
			stages.push_back(stage);
		}

		// ---------- Vertex Input ----------
		std::vector<VkVertexInputBindingDescription> bindings;
		for (auto& b : desc.vertexLayout.bindings) {
			bindings.push_back({ b.binding,
				b.stride,
				b.instanceData ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX });
		}

		std::vector<VkVertexInputAttributeDescription> attrs;
		for (auto& a : desc.vertexLayout.attributes) {
			attrs.push_back({ a.location,
				a.binding,
				TOVKFormat(a.datatype), // EXPECTS VkFormat
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
		inputAsm.topology = TOVKPrimitiveTopology(desc.primitiveType);

		// ---------- Viewport ----------
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
		rast.polygonMode = TOVKPolygonMode(desc.rasterizer.fillMode);
		rast.cullMode = TOVKCullMode(desc.rasterizer.cullMode);
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
		depth.depthCompareOp = TOVKCompareOp(desc.depthStencil.depthFunc);

		// ---------- Color Blend ----------
		VkPipelineColorBlendAttachmentState blend{};
		blend.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo cb{};
		cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cb.attachmentCount = 1;
		cb.pAttachments = &blend;

		// ---------- Pipeline Layout ----------
		VkPipelineLayout layout;
		VkPipelineLayoutCreateInfo lci{};
		lci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		vkCreatePipelineLayout(ctx.device, &lci, nullptr, &layout);

		// Minimal renderpass
		VkRenderPass renderPass;
		VkAttachmentDescription att{};
		att.format = ctx.swapchainImageFormat;
		att.samples = VK_SAMPLE_COUNT_1_BIT;
		att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorRef{};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription sub{};
		sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		sub.colorAttachmentCount = 1;
		sub.pColorAttachments = &colorRef;

		VkRenderPassCreateInfo rpci{};
		rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rpci.attachmentCount = 1;
		rpci.pAttachments = &att;
		rpci.subpassCount = 1;
		rpci.pSubpasses = &sub;

		VK_CHECK(vkCreateRenderPass(ctx.device, &rpci, nullptr, &renderPass));

		ctx.RenderPass = renderPass;

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
		pci.renderPass = renderPass;
		pci.layout = layout;

		VkPipeline pipeline;
		if (vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pci, nullptr, &pipeline) != VK_SUCCESS) {
			RENDERX_CRITICAL("Failed to create graphics pipeline");
			return {};
		}

		PipelineHandle handle{ s_NextPipelineId++ };
		s_Pipelines[handle.id] = pipeline;
		s_PipelineLayouts[handle.id] = layout;
		RENDERX_INFO("Created Vulkan Graphics Pipeline with ID {}", handle.id);
		return handle;
	}

	// command List Functions

	void RenderXVK::VKCmdSetPipeline(CommandList& cmdList, PipelineHandle pipeline) {
		PROFILE_FUNCTION();
		if (!s_CommandLists[cmdList.id].isRecording)
			return;
		auto it = s_Pipelines.find(pipeline.id);
		if (it == s_Pipelines.end()) {
			RENDERX_ERROR("Invalid Pipeline Handle  , line:{} File:{}", __LINE__, __FILE__);
			return;
		}
		// temp vkBindPoint
		vkCmdBindPipeline(s_CommandLists[cmdList.id].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, it->second);
	}

} // namespace RenderX
