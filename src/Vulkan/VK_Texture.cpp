#include "VK_Common.h"
#include "VK_RenderX.h"

namespace Rx {

namespace RxVK {

inline uint32_t CalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1) {
    uint32_t maxDim    = std::max(std::max(width, height), depth);
    uint32_t mipLevels = 1;
    while (maxDim > 1) {
        maxDim >>= 1;
        mipLevels++;
    }
    return mipLevels;
}

inline VkSubresourceLayout
GetImageSubresourceLayout(VkImage image, VkFormat format, uint32_t mipLevel = 0, uint32_t arrayLayer = 0) {
    VkImageSubresource subresource{};
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.mipLevel   = mipLevel;
    subresource.arrayLayer = arrayLayer;
    VkSubresourceLayout layout;
    vkGetImageSubresourceLayout(GetVulkanContext().device->logical(), image, &subresource, &layout);
    return layout;
}

inline bool IsDepthFormat(VkFormat format) {
    return format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT ||
           format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

inline bool IsStencilFormat(VkFormat format) {
    return format == VK_FORMAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT ||
           format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

inline bool IsCompressedFormat(VkFormat format) {
    return (format >= VK_FORMAT_BC1_RGB_UNORM_BLOCK && format <= VK_FORMAT_BC7_SRGB_BLOCK) ||
           (format >= VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK && format <= VK_FORMAT_EAC_R11G11_SNORM_BLOCK) ||
           (format >= VK_FORMAT_ASTC_4x4_UNORM_BLOCK && format <= VK_FORMAT_ASTC_12x12_SRGB_BLOCK);
}

inline bool IsDepthStencilFormat(VkFormat format) {
    return IsDepthFormat(format) || IsStencilFormat(format);
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

    texture.isSwapchainImage = false;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = ToVulkanImageType(desc.type);
    imageInfo.extent.width  = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth  = desc.type == TextureType::TEXTURE_3D ? desc.depth : 1;
    imageInfo.mipLevels     = texture.mipLevels;
    imageInfo.arrayLayers   = texture.arrayLayers;
    imageInfo.format        = texture.format;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    imageInfo.usage = GetDefaultImageUsageFlags(desc.type, desc.format);

    // Allow UAV
    if (Has(desc.usage, TextureUsage::STORAGE))
        imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

    if (desc.type == TextureType::TEXTURE_CUBE)
        imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VmaAllocationCreateInfo allocInfo =
        ToVmaAllocationCreateInfoForImage(desc.memoryType, Has(desc.usage, TextureUsage::RENDER_TARGET), false);

    VmaAllocationInfo vmaInfo{};

    if (!ctx.allocator->createImage(
            imageInfo, allocInfo.usage, allocInfo.flags, texture.image, texture.allocation, &vmaInfo)) {
        RENDERX_ERROR("Failed to create Vulkan texture image");
        return {};
    }

    texture.state.global.color.layout      = VK_IMAGE_LAYOUT_UNDEFINED;
    texture.state.global.color.stageMask   = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    texture.state.global.color.accessMask  = 0;
    texture.state.global.color.queueFamily = ctx.device->graphicsFamily();

    return g_TexturePool.allocate(texture);
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
        RENDERX_ERROR("Invalid texture handle for view creation");
        return {};
    }

    auto& ctx = GetVulkanContext();

    VulkanTextureView view{};
    view.texture = desc.texture;
    view.format  = desc.format == Format::UNDEFINED ? texture->format : ToVulkanFormat(desc.format);

    view.viewType        = ToVulkanViewType(desc.viewType);
    view.baseMipLevel    = desc.baseMipLevel;
    view.mipLevelCount   = desc.mipLevelCount;
    view.baseArrayLayer  = desc.baseArrayLayer;
    view.arrayLayerCount = desc.arrayLayerCount;

    VkImageAspectFlags aspect =
        GetImageAspect(desc.format == Format::UNDEFINED ? VkFormatToFormat(texture->format) : desc.format);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = texture->image;
    viewInfo.viewType = view.viewType;
    viewInfo.format   = view.format;

    // TODO--------------------------------------------------------
    //  Component mapping (usually identity)
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    //-------------------------------------------------------------

    // Subresource range
    viewInfo.subresourceRange.aspectMask     = aspect;
    viewInfo.subresourceRange.baseMipLevel   = view.baseMipLevel;
    viewInfo.subresourceRange.levelCount     = view.mipLevelCount;
    viewInfo.subresourceRange.baseArrayLayer = view.baseArrayLayer;
    viewInfo.subresourceRange.layerCount     = view.arrayLayerCount;

    VK_CHECK(vkCreateImageView(GetVulkanContext().device->logical(), &viewInfo, nullptr, &view.view));

    if (view.view == VK_NULL_HANDLE)
        return {};

    return g_TextureViewPool.allocate(view);
}

void VKDestroyTextureView(TextureViewHandle& handle) {
    auto* view = g_TextureViewPool.get(handle);
    if (!view)
        return;

    auto& ctx = GetVulkanContext();

    if (view->view != VK_NULL_HANDLE) {
        vkDestroyImageView(ctx.device->logical(), view->view, nullptr);
    }

    g_TextureViewPool.free(handle);
}

} // namespace RxVK
} // namespace Rx