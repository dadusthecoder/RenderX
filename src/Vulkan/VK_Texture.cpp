#include "VK_Common.h"
#include "VK_RenderX.h"

namespace Rx {
namespace RxVK {

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
    imageInfo.usage         = BuildImageUsageFlags(desc); // ← use desc flags directly

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

} // namespace RxVK
} // namespace Rx