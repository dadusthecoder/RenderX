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
    return format == VK_FORMAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
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
    VkImageCreateInfo imageInfo{};
    imageInfo.extent.width = desc.width;
    imageInfo.extent.height =
        (desc.type == TextureType::TEXTURE_1D || desc.type == TextureType::TEXTURE_1D_ARRAY) ? 1 : desc.height;
    imageInfo.extent.depth = (desc.type == TextureType::TEXTURE_3D) ? desc.depth : 1;
    if (desc.generateMips || desc.mipLevels > 1)
        imageInfo.mipLevels = desc.mipLevels;
    else
        imageInfo.mipLevels = 1;
    // TODO
    VulkanTexture temp{};
    return g_TexturePool.allocate(temp);
}

void VKDestroyTexture(TextureHandle& handle) {
    g_TexturePool.free(handle);
}
inline VkImageView CreateImageView(VkImage            image,
                                   VkFormat           format,
                                   VkImageViewType    viewType,
                                   VkImageAspectFlags aspectFlags,
                                   uint32_t           mipLevels,
                                   uint32_t           arrayLayers,
                                   uint32_t           baseMipLevel   = 0,
                                   uint32_t           baseArrayLayer = 0) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = image;
    viewInfo.viewType = viewType;
    viewInfo.format   = format;

    // Component mapping (usually identity)
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // Subresource range
    viewInfo.subresourceRange.aspectMask     = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel   = baseMipLevel;
    viewInfo.subresourceRange.levelCount     = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
    viewInfo.subresourceRange.layerCount     = arrayLayers;

    VkImageView imageView;
    VkResult    result = vkCreateImageView(GetVulkanContext().device->logical(), &viewInfo, nullptr, &imageView);

    if (result != VK_SUCCESS) {
        RENDERX_ERROR("Failed to create image view: {}", VkResultToString(result));
        return VK_NULL_HANDLE;
    }

    return imageView;
}

TextureViewHandle VKCreateTextureView(const TextureViewDesc& desc) {
    VulkanTextureView view{};
    return g_TextureViewPool.allocate(view);
}

void VKDestroyTextureView(TextureViewHandle& handle) {
    g_TextureViewPool.free(handle);
}

} // namespace RxVK
} // namespace Rx