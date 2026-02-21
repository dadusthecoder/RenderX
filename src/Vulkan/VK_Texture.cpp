#include "VK_Common.h"
#include "VK_RenderX.h"

namespace Rx {
namespace RxVK {

ResourcePool<VulkanSampler, SamplerHandle> g_SamplerPool;

static uint32_t CalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1) {
    uint32_t maxDim    = std::max({width, height, depth});
    uint32_t mipLevels = 1;
    while (maxDim > 1) {
        maxDim >>= 1;
        mipLevels++;
    }
    return mipLevels;
}

static bool IsDepthFormat(VkFormat format) {
    return format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT ||
           format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

static bool IsStencilFormat(VkFormat format) {
    return format == VK_FORMAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT ||
           format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

static bool IsDepthStencilFormat(VkFormat format) {
    return IsDepthFormat(format) || IsStencilFormat(format);
}

// Build VkImageUsageFlags directly from TextureDesc::usage.
// This replaces GetDefaultImageUsageFlags which ignored the desc flags entirely.
static VkImageUsageFlags BuildImageUsageFlags(const TextureDesc& desc) {
    VkImageUsageFlags usage = 0;

    if (Has(desc.usage, TextureUsage::TRANSFER_SRC))
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    if (Has(desc.usage, TextureUsage::TRANSFER_DST))
        usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (Has(desc.usage, TextureUsage::SAMPLED))
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

    if (Has(desc.usage, TextureUsage::STORAGE))
        usage |= VK_IMAGE_USAGE_STORAGE_BIT;

    if (Has(desc.usage, TextureUsage::RENDER_TARGET))
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (Has(desc.usage, TextureUsage::DEPTH_STENCIL))
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    // Mip generation requires both SRC and DST
    if (desc.generateMips)
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    RENDERX_ASSERT_MSG(usage != 0, "CreateTexture: TextureDesc has no usage flags set");

    return usage;
}

TextureHandle VKCreateTexture(const TextureDesc& desc) {
    auto& ctx = GetVulkanContext();

    VulkanTexture texture{};
    texture.width       = desc.width;
    texture.height      = desc.height;
    texture.arrayLayers = desc.arrayLayers > 0 ? desc.arrayLayers : 1;
    texture.format      = ToVulkanFormat(desc.format);
    texture.mipLevels =
        desc.generateMips ? CalculateMipLevels(desc.width, desc.height, desc.depth) : std::max(1u, desc.mipLevels);
    texture.debugName        = desc.debugName;
    texture.isSwapchainImage = false;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = ToVulkanImageType(desc.type);
    imageInfo.extent.width  = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth  = desc.type == TextureType::TEXTURE_3D ? std::max(1u, desc.depth) : 1;
    imageInfo.mipLevels     = texture.mipLevels;
    imageInfo.arrayLayers   = texture.arrayLayers;
    imageInfo.format        = ToVulkanFormat(desc.format);
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags         = 0;
    imageInfo.usage         = BuildImageUsageFlags(desc); // use desc flags directly

    if (desc.type == TextureType::TEXTURE_CUBE) {
        imageInfo.flags       |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageInfo.arrayLayers  = 6; // cubemaps always 6 faces
    }

    VmaAllocationCreateInfo allocInfo =
        ToVmaAllocationCreateInfoForImage(desc.memoryType, Has(desc.usage, TextureUsage::RENDER_TARGET), false);

    VmaAllocationInfo vmaInfo{};
    if (!ctx.allocator->createImage(
            imageInfo, allocInfo.usage, allocInfo.flags, texture.image, texture.allocation, &vmaInfo)) {
        RENDERX_ERROR("VKCreateTexture: failed to allocate image ({}x{} fmt={})",
                      desc.width,
                      desc.height,
                      static_cast<int>(desc.format));
        return {};
    }

    // Set initial resource state
    // Depth and color images track state separately
    bool isDepth = IsDepthStencilFormat(texture.format);

    VulkanAccessState initialState{};
    initialState.layout      = VK_IMAGE_LAYOUT_UNDEFINED;
    initialState.stageMask   = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    initialState.accessMask  = 0;
    initialState.queueFamily = ctx.device->graphicsFamily();

    if (isDepth) {
        texture.state.global.depth   = initialState;
        texture.state.global.stencil = initialState;
    } else {
        texture.state.global.color = initialState;
    }

    if (desc.initialData) {
        TextureCopy cpy = TextureCopy::FullTexture(desc.width, desc.height, desc.depth);
        ctx.loadTimeStagingUploader->uploadTexture(texture.image, desc.initialData, desc.size, cpy);
    }

    return g_TexturePool.allocate(std::move(texture));
}

void VKDestroyTexture(TextureHandle& handle) {
    auto* tex = g_TexturePool.get(handle);
    if (!tex)
        return;

    auto& ctx = GetVulkanContext();

    if (!tex->isSwapchainImage && tex->image != VK_NULL_HANDLE) {
        ctx.allocator->destroyImage(tex->image, tex->allocation);
    }

    g_TexturePool.free(handle);
}

TextureViewHandle VKCreateTextureView(const TextureViewDesc& desc) {
    auto* texture = g_TexturePool.get(desc.texture);
    if (!texture) {
        RENDERX_ERROR("VKCreateTextureView: invalid texture handle");
        return {};
    }

    auto& ctx = GetVulkanContext();

    // Resolve format — if caller left it UNDEFINED, inherit from the texture
    VkFormat resolvedFormat = (desc.format == Format::UNDEFINED) ? texture->format : ToVulkanFormat(desc.format);

    VulkanTextureView view{};
    view.texture         = desc.texture;
    view.format          = resolvedFormat;
    view.viewType        = ToVulkanViewType(desc.viewType);
    view.baseMipLevel    = desc.baseMipLevel;
    view.mipLevelCount   = desc.mipLevelCount;
    view.baseArrayLayer  = desc.baseArrayLayer;
    view.arrayLayerCount = desc.arrayLayerCount;
    view.debugName       = desc.debugName;

    // Derive aspect from the resolved format, not the descriptor format
    // This correctly sets DEPTH_BIT for D32_SFLOAT etc.
    VkImageAspectFlags aspect = 0;
    if (IsDepthFormat(resolvedFormat))
        aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if (IsStencilFormat(resolvedFormat))
        aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    if (aspect == 0)
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = texture->image;
    viewInfo.viewType = view.viewType;
    viewInfo.format   = resolvedFormat;

    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.subresourceRange.aspectMask     = aspect;
    viewInfo.subresourceRange.baseMipLevel   = view.baseMipLevel;
    viewInfo.subresourceRange.levelCount     = view.mipLevelCount;
    viewInfo.subresourceRange.baseArrayLayer = view.baseArrayLayer;
    viewInfo.subresourceRange.layerCount     = view.arrayLayerCount;

    VkResult result = vkCreateImageView(ctx.device->logical(), &viewInfo, nullptr, &view.view);
    if (result != VK_SUCCESS) {
        RENDERX_ERROR("VKCreateTextureView: vkCreateImageView failed: {}", VkResultToString(result));
        return {};
    }

    return g_TextureViewPool.allocate(std::move(view));
}

void VKDestroyTextureView(TextureViewHandle& handle) {
    auto* view = g_TextureViewPool.get(handle);
    if (!view)
        return;

    if (view->view != VK_NULL_HANDLE) {
        vkDestroyImageView(GetVulkanContext().device->logical(), view->view, nullptr);
    }

    g_TextureViewPool.free(handle);
}

static VkFilter ToVkFilter(Filter f) {
    switch (f) {
    case Filter::NEAREST:
        return VK_FILTER_NEAREST;
    case Filter::LINEAR:
        return VK_FILTER_LINEAR;
    }
    return VK_FILTER_LINEAR;
}

static VkSamplerMipmapMode ToVkMipFilter(Filter f) {
    switch (f) {
    case Filter::NEAREST:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case Filter::LINEAR:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
    return VK_SAMPLER_MIPMAP_MODE_LINEAR;
}

static VkSamplerAddressMode ToVkAddressMode(AddressMode m) {
    switch (m) {
    case AddressMode::REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case AddressMode::MIRRORED_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case AddressMode::CLAMP_TO_EDGE:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case AddressMode::CLAMP_TO_BORDER:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case AddressMode::MIRROR_CLAMP_TO_EDGE:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    }
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

static VkBorderColor ToVkBorderColor(BorderColor c) {
    switch (c) {
    case BorderColor::FLOAT_TRANSPARENT_BLACK:
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    case BorderColor::INT_TRANSPARENT_BLACK:
        return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    case BorderColor::FLOAT_OPAQUE_BLACK:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    case BorderColor::INT_OPAQUE_BLACK:
        return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    case BorderColor::FLOAT_OPAQUE_WHITE:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    case BorderColor::INT_OPAQUE_WHITE:
        return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    }
    return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
}

static VkCompareOp ToVkCompareOp(CompareOp op) {
    switch (op) {
    case CompareOp::NEVER:
        return VK_COMPARE_OP_NEVER;
    case CompareOp::LESS:
        return VK_COMPARE_OP_LESS;
    case CompareOp::EQUAL:
        return VK_COMPARE_OP_EQUAL;
    case CompareOp::LESS_EQUAL:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOp::GREATER:
        return VK_COMPARE_OP_GREATER;
    case CompareOp::NOT_EQUAL:
        return VK_COMPARE_OP_NOT_EQUAL;
    case CompareOp::GREATER_EQUAL:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case CompareOp::ALWAYS:
        return VK_COMPARE_OP_ALWAYS;
    }
    return VK_COMPARE_OP_ALWAYS;
}

SamplerHandle VKCreateSampler(const SamplerDesc& desc) {
    auto& ctx = GetVulkanContext();

    // Query device limits for validation
    const VkPhysicalDeviceLimits& limits = ctx.device->limits();

    // Clamp anisotropy to what the device actually supports
    float aniso       = desc.maxAnisotropy;
    bool  enableAniso = (aniso > 1.0f) && limits.maxSamplerAnisotropy >= 1.0f;
    if (enableAniso)
        aniso = std::min(aniso, limits.maxSamplerAnisotropy);

    VkSamplerCreateInfo info     = {};
    info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter               = ToVkFilter(desc.magFilter);
    info.minFilter               = ToVkFilter(desc.minFilter);
    info.mipmapMode              = ToVkMipFilter(desc.mipFilter);
    info.addressModeU            = ToVkAddressMode(desc.addressU);
    info.addressModeV            = ToVkAddressMode(desc.addressV);
    info.addressModeW            = ToVkAddressMode(desc.addressW);
    info.mipLodBias              = desc.mipLodBias;
    info.anisotropyEnable        = enableAniso ? VK_TRUE : VK_FALSE;
    info.maxAnisotropy           = enableAniso ? aniso : 1.0f;
    info.compareEnable           = desc.comparisonEnable ? VK_TRUE : VK_FALSE;
    info.compareOp               = ToVkCompareOp(desc.compareOp);
    info.minLod                  = desc.minLod;
    info.maxLod                  = desc.maxLod;
    info.borderColor             = ToVkBorderColor(desc.borderColor);
    info.unnormalizedCoordinates = desc.unnormalizedCoords ? VK_TRUE : VK_FALSE;

    // Unnormalized coords have strict constraints — validate early
    if (desc.unnormalizedCoords) {
        RENDERX_ASSERT_MSG(desc.minFilter == desc.magFilter,
                           "VK_CreateSampler: unnormalized coords requires minFilter == magFilter");
        RENDERX_ASSERT_MSG(desc.mipFilter == Filter::NEAREST && desc.minLod == 0.0f && desc.maxLod == 0.0f,
                           "VK_CreateSampler: unnormalized coords requires NEAREST mip mode and lod 0..0");
        RENDERX_ASSERT_MSG(!desc.comparisonEnable,
                           "VK_CreateSampler: unnormalized coords cannot be used with comparison samplers");
    }

    // Comparison samplers must not use anisotropy
    if (desc.comparisonEnable && enableAniso) {
        RENDERX_WARN("VK_CreateSampler: comparison sampler '{}' had anisotropy enabled — disabling",
                     desc.debugName ? desc.debugName : "(unnamed)");
        info.anisotropyEnable = VK_FALSE;
        info.maxAnisotropy    = 1.0f;
    }

    VulkanSampler sampler = {};

    VkResult result = vkCreateSampler(ctx.device->logical(), &info, nullptr, &sampler.vkSampler);
    if (result != VK_SUCCESS) {
        RENDERX_ERROR("VK_CreateSampler: vkCreateSampler failed: {}", VkResultToString(result));
        return {};
    }

    // // Debug label
    // if (desc.debugName && ctx.device->debugUtilsEnabled()) {
    //     VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    //     nameInfo.sType                         = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    //     nameInfo.objectType                    = VK_OBJECT_TYPE_SAMPLER;
    //     nameInfo.objectHandle                  = reinterpret_cast<uint64_t>(sampler.sampler);
    //     nameInfo.pObjectName                   = desc.debugName;
    //     vkSetDebugUtilsObjectNameEXT(ctx.device->logical(), &nameInfo);
    // }

    return g_SamplerPool.allocate(sampler);
}

void VKDestroySampler(SamplerHandle& handle) {
    auto* s = g_SamplerPool.get(handle);
    if (!s)
        return;

    if (s->vkSampler != VK_NULL_HANDLE)
        vkDestroySampler(GetVulkanContext().device->logical(), s->vkSampler, nullptr);

    g_SamplerPool.free(handle);
}

} // namespace RxVK
} // namespace Rx