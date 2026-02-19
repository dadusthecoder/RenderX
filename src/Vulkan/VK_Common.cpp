
#include "VK_Common.h"
#include "VK_RenderX.h"
#include <algorithm>

// Define VMA implementation - must be included in exactly one source file
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#ifdef RX_DEBUG_BUILD
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

namespace Rx {
namespace RxVK {

// ResourcePool instances for resource management
ResourcePool<VulkanTexture, TextureHandle>         g_TexturePool;
ResourcePool<VulkanTextureView, TextureViewHandle> g_TextureViewPool;
ResourcePool<VulkanBuffer, BufferHandle>           g_BufferPool;
ResourcePool<VulkanShader, ShaderHandle>           g_ShaderPool;
ResourcePool<VulkanRenderPass, RenderPassHandle>   g_RenderPassPool;
ResourcePool<VulkanFramebuffer, FramebufferHandle> g_FramebufferPool;

VulkanContext& GetVulkanContext() {
    static VulkanContext g_Context;
    return g_Context;
}

void freeAllVulkanResources() {
    auto& ctx      = GetVulkanContext();
    auto  g_Device = ctx.device->logical();
    vkDeviceWaitIdle(ctx.device->logical());
    //-------------------------------------------------------------------------------------
    // The swapchain must be destroyed before resource pools.
    // "freeAllVulkanResources()" releases pooled resources, and the backend may
    // attempt to destroy swapchain images through those pools.
    // Swapchain images are owned by the driver and must be destroyed with the swapchain.
    delete ctx.swapchain;
    //-------------------------------------------------------------------------------------

    g_BufferViewPool.ForEach([](VulkanBufferView& view) { view.isValid = false; });

    g_BufferPool.ForEach([&](VulkanBuffer& buffer) {
        if (buffer.buffer != VK_NULL_HANDLE) {
            ctx.allocator->destroyBuffer(buffer.buffer, buffer.allocation);
            buffer.buffer     = VK_NULL_HANDLE;
            buffer.allocation = VK_NULL_HANDLE;
        }

        buffer.bindingCount = 0;
        buffer.allocInfo    = {};
    });

    g_TexturePool.ForEach([&](VulkanTexture& texture) {
        if (texture.image != VK_NULL_HANDLE) {
            ctx.allocator->destroyImage(texture.image, texture.allocation);
            texture.image      = VK_NULL_HANDLE;
            texture.allocation = VK_NULL_HANDLE;
        }
        texture.format    = VK_FORMAT_UNDEFINED;
        texture.width     = 0;
        texture.height    = 0;
        texture.mipLevels = 1;
    });

    g_ShaderPool.ForEach([&](VulkanShader& shader) {
        if (shader.shaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(g_Device, shader.shaderModule, nullptr);
            shader.shaderModule = VK_NULL_HANDLE;
        }

        shader.entryPoint.clear();
    });

    g_PipelineLayoutPool.ForEach([&](VulkanPipelineLayout& layout) {
        if (layout.layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(g_Device, layout.layout, nullptr);
        }
    });

 

    g_PipelinePool.ForEach([&](VulkanPipeline& pipeline) {
        if (pipeline.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(g_Device, pipeline.pipeline, nullptr);
            pipeline.pipeline = VK_NULL_HANDLE;
        }
    });

    g_RenderPassPool.ForEach([&](VulkanRenderPass& rp) {
        if (rp.renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(g_Device, rp.renderPass, nullptr);
        }
    });

    g_BufferPool.clear();
    g_ShaderPool.clear();
    g_TexturePool.clear();
    g_TextureViewPool.clear();
    g_PipelinePool.clear();
    g_BufferViewPool.clear();
    g_RenderPassPool.clear();
    g_BufferViewCache.clear();
    g_FramebufferPool.clear();
    g_PipelineLayoutPool.clear();
}

void VKShutdownCommon() {
    auto& ctx = GetVulkanContext();
    vkDeviceWaitIdle(ctx.device->logical());
    freeAllVulkanResources();

    //---------------------------------------
    // must follow this destruction order
    delete ctx.stagingAllocator;
    delete ctx.deferredUploader;
    delete ctx.immediateUploader;
    delete ctx.graphicsQueue;
    delete ctx.computeQueue;
    delete ctx.transferQueue;
    delete ctx.allocator;
    delete ctx.device;
    delete ctx.instance;
    //---------------------------------------
}

} // namespace RxVK
} // namespace Rx
