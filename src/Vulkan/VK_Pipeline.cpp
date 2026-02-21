#include "VK_Common.h"
#include "VK_RenderX.h"

namespace Rx {

namespace RxVK {

ResourcePool<VulkanPipelineLayout, PipelineLayoutHandle> g_PipelineLayoutPool;
ResourcePool<VulkanPipeline, PipelineHandle>             g_PipelinePool;

// SHADER
ShaderHandle VKCreateShader(const ShaderDesc& desc) {
    RENDERX_ASSERT_MSG(desc.bytecode.size() > 0, "VKCreateShader: bytecode is empty");
    RENDERX_ASSERT_MSG(desc.bytecode.size() % sizeof(uint32_t) == 0,
                       "VKCreateShader: bytecode size not aligned to uint32_t");
    RENDERX_ASSERT_MSG(!desc.entryPoint.empty(), "VKCreateShader: entryPoint is empty");

    VkShaderModuleCreateInfo ci{};
    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = desc.bytecode.size();
    ci.pCode    = reinterpret_cast<const uint32_t*>(desc.bytecode.data());

    VkShaderModule shaderModule;
    auto&          ctx = GetVulkanContext();
    RENDERX_ASSERT_MSG(ctx.device && ctx.device->logical() != VK_NULL_HANDLE,
                       "VKCreateShader: device is VK_NULL_HANDLE");

    VkResult result = vkCreateShaderModule(ctx.device->logical(), &ci, nullptr, &shaderModule);
    if (!CheckVk(result, "VKCreateShader: Failed to create shader module")) {
        return ShaderHandle{};
    }
    RENDERX_ASSERT_MSG(shaderModule != VK_NULL_HANDLE, "VKCreateShader: created shader module is null");

    VulkanShader shader{desc.entryPoint, desc.stage, shaderModule};
    ShaderHandle handle = g_ShaderPool.allocate(shader);
    return handle;
}

void VKDestroyShader(ShaderHandle& handle) {
    auto* shader = g_ShaderPool.get(handle);
    auto& ctx    = GetVulkanContext();
    vkDestroyShaderModule(ctx.device->logical(), shader->shaderModule, nullptr);
    g_ShaderPool.free(handle);
}

VkPushConstantRange ToVkPushConstantRange(const PushConstantRange& r) {
    VkPushConstantRange vkr = {};
    vkr.stageFlags          = MapShaderStage(r.stages);
    vkr.offset              = r.offset;
    vkr.size                = r.size;
    return vkr;
}

// Pipeline LAyout Creation
PipelineLayoutHandle VKCreatePipelineLayout(const SetLayoutHandle*   pLayouts,
                                            uint32_t                 LayoutCount,
                                            const PushConstantRange* pushRanges,
                                            uint32_t                 pushRangeCount) {
    if (LayoutCount == 0)
        RENDERX_WARN("setlayoutCount is zero");
    if (pLayouts == nullptr)
        RENDERX_WARN("playouts pointer is null");

    auto& ctx = GetVulkanContext();

    // Translate set layouts
    VkDescriptorSetLayout vkSetLayouts[16] = {};
    RENDERX_ASSERT_MSG(LayoutCount <= 16, "VK_CreatePipelineLayout: too many set layouts (max 16)");
    for (uint32_t i = 0; i < LayoutCount; i++) {
        auto* sl = g_SetLayoutPool.get(pLayouts[i]);
        RENDERX_ASSERT_MSG(sl, "VK_CreatePipelineLayout: invalid SetLayoutHandle at index {}", i);
        vkSetLayouts[i] = sl->vkLayout;
    }

    // Translate push constant ranges
    VkPushConstantRange vkRanges[VulkanPipelineLayout::MAX_PUSH_RANGES] = {};
    RENDERX_ASSERT_MSG(pushRangeCount <= VulkanPipelineLayout::MAX_PUSH_RANGES,
                       "VKCreatePipelineLayout: too many push constant ranges (max {})",
                       VulkanPipelineLayout::MAX_PUSH_RANGES);

    // Validate total size against device limit (guaranteed minimum is 128 bytes)
    uint32_t totalSize = 0;
    for (uint32_t i = 0; i < pushRangeCount; i++) {
        vkRanges[i] = ToVkPushConstantRange(pushRanges[i]);
        totalSize   = std::max(totalSize, pushRanges[i].offset + pushRanges[i].size);
    }
    RENDERX_ASSERT_MSG(totalSize <= ctx.device->limits().maxPushConstantsSize,
                       "VKCreatePipelineLayout: push constant total size {} exceeds device limit {}",
                       totalSize,
                       ctx.device->limits().maxPushConstantsSize);

    VkPipelineLayoutCreateInfo info = {};
    info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount             = LayoutCount;
    info.pSetLayouts                = LayoutCount > 0 ? vkSetLayouts : nullptr;
    info.pushConstantRangeCount     = pushRangeCount;
    info.pPushConstantRanges        = pushRangeCount > 0 ? vkRanges : nullptr;

    VulkanPipelineLayout layout = {};

    VkResult result = vkCreatePipelineLayout(ctx.device->logical(), &info, nullptr, &layout.vkLayout);

    if (result != VK_SUCCESS) {
        RENDERX_ERROR("VK_CreatePipelineLayout: vkCreatePipelineLayout failed: {}", VkResultToString(result));
        return {};
    }

    // Store push range metadata for later retrieval by setPipeline / pushConstants
    layout.pushRangeCount = pushRangeCount;
    for (uint32_t i = 0; i < pushRangeCount; i++) {
        layout.pushRanges[i].offset     = pushRanges[i].offset;
        layout.pushRanges[i].size       = pushRanges[i].size;
        layout.pushRanges[i].stageFlags = vkRanges[i].stageFlags;
    }

    return g_PipelineLayoutPool.allocate(layout);
}

void VKDestroyPipelineLayout(PipelineLayoutHandle& handle) {
    RENDERX_WARN("Not Implemented");
}
// GRAPHICS PIPELINE
PipelineHandle VKCreateGraphicsPipeline(PipelineDesc& desc) {
    auto& ctx = GetVulkanContext();

    //  Shader Stages
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    for (auto& sh : desc.shaders) {
        // Try ResourcePool first
        auto* shader = g_ShaderPool.get(sh);
        if (!shader || shader->shaderModule == VK_NULL_HANDLE) {
            RENDERX_CRITICAL("Invalid shader handle");
            return PipelineHandle{};
        }

        VkPipelineShaderStageCreateInfo stage{};
        stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.stage  = MapShaderStage(shader->type);
        stage.module = shader->shaderModule;
        stage.pName  = shader->entryPoint.c_str();
        stages.push_back(stage);
    }

    //  Vertex Input
    std::vector<VkVertexInputBindingDescription> vertexBindings;
    for (auto& b : desc.vertexInputState.vertexBindings) {
        vertexBindings.push_back(
            {b.binding, b.stride, b.instanceData ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX});
    }

    std::vector<VkVertexInputAttributeDescription> attrs;
    for (auto& a : desc.vertexInputState.attributes) {
        attrs.push_back({a.location, a.binding, ToVulkanFormat(a.format), a.offset});
    }

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount   = (uint32_t)vertexBindings.size();
    vertexInput.pVertexBindingDescriptions      = vertexBindings.data();
    vertexInput.vertexAttributeDescriptionCount = (uint32_t)attrs.size();
    vertexInput.pVertexAttributeDescriptions    = attrs.data();

    //  Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAsm{};
    inputAsm.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAsm.topology = ToVulkanTopology(desc.primitiveType);

    static constexpr VkDynamicState kDynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates    = kDynamicStates;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;
    viewportState.pViewports    = nullptr; // nullptr = dynamic
    viewportState.pScissors     = nullptr; // nullptr = dynamic

    //  Rasterizer
    VkPipelineRasterizationStateCreateInfo rast{};
    rast.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.polygonMode = ToVulkanPolygonMode(desc.rasterizer.fillMode);
    rast.cullMode    = ToVulkanCullMode(desc.rasterizer.cullMode);
    rast.frontFace = desc.rasterizer.frontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    rast.lineWidth = 1.0f;

    //  Multisample
    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    //  Depth
    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable  = desc.depthStencil.depthEnable;
    depth.depthWriteEnable = desc.depthStencil.depthWriteEnable;
    depth.depthCompareOp   = ToVulkanCompareOp(desc.depthStencil.depthFunc);

    auto&                               b = desc.blend;
    VkPipelineColorBlendAttachmentState blend{};
    blend.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    blend.blendEnable = b.enable ? VK_TRUE : VK_FALSE;

    // Color blending
    blend.srcColorBlendFactor = ToVulkanBlendFactor(b.srcColor);
    blend.dstColorBlendFactor = ToVulkanBlendFactor(b.dstColor);
    blend.colorBlendOp        = ToVulkanBlendOp(b.colorOp);

    // Alpha blending
    blend.srcAlphaBlendFactor = ToVulkanBlendFactor(b.srcAlpha);
    blend.dstAlphaBlendFactor = ToVulkanBlendFactor(b.dstAlpha);
    blend.alphaBlendOp        = ToVulkanBlendOp(b.alphaOp);

    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.logicOpEnable     = VK_FALSE;
    cb.attachmentCount   = 1;
    cb.pAttachments      = &blend;
    cb.blendConstants[0] = b.blendFactor.r;
    cb.blendConstants[1] = b.blendFactor.g;
    cb.blendConstants[2] = b.blendFactor.b;
    cb.blendConstants[3] = b.blendFactor.a;

    // dynamic rendering
    std::vector<VkFormat> colorAttachmentFormats;
    for (auto format : desc.colorFromats)
        colorAttachmentFormats.push_back(ToVulkanFormat(format));

    VkPipelineRenderingCreateInfo rci{};
    rci.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rci.colorAttachmentCount    = (uint32_t)colorAttachmentFormats.size();
    rci.pColorAttachmentFormats = colorAttachmentFormats.data();
    rci.depthAttachmentFormat   = ToVulkanFormat(desc.depthFormat);

    //  Create Pipeline
    VkGraphicsPipelineCreateInfo pci{};
    pci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.stageCount          = (uint32_t)stages.size();
    pci.pStages             = stages.data();
    pci.pVertexInputState   = &vertexInput;
    pci.pInputAssemblyState = &inputAsm;
    pci.pDynamicState       = &dynamicState;
    pci.pViewportState      = &viewportState;
    pci.pRasterizationState = &rast;
    pci.pMultisampleState   = &ms;
    pci.pDepthStencilState  = &depth;
    pci.pColorBlendState    = &cb;
    pci.pNext               = &rci;

    // pci.renderPass = g_RenderPassPool.get(desc.renderPass)->renderPass;
    auto* layout = g_PipelineLayoutPool.get(desc.layout);

    RENDERX_ASSERT_MSG(layout, "Invlaid layout handle");
    pci.layout = layout->vkLayout;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(ctx.device->logical(), VK_NULL_HANDLE, 1, &pci, nullptr, &pipeline) != VK_SUCCESS) {
        RENDERX_CRITICAL("Failed to create graphics pipeline");
        // destroy the pipeline layout to avoid leaking it
        if (layout->vkLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(ctx.device->logical(), layout->vkLayout, nullptr);
            g_PipelineLayoutPool.free(desc.layout);
        }
        return {};
    }

    VulkanPipeline vkpipe{};
    vkpipe.vkPipeline = pipeline;
    vkpipe.layout     = desc.layout;

    auto handle = g_PipelinePool.allocate(vkpipe);
    RENDERX_INFO("Created Vulkan Graphics Pipeline with ID {}", handle.id);
    return handle;
}

void VKDestroyPipeline(PipelineHandle& handle) {
    RENDERX_WARN("Not implemented");
}
} // namespace RxVK
} // namespace Rx
