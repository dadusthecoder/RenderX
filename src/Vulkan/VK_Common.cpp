
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

void VKPrintHandles() {
    RENDERX_INFO("---- Buffers ----");
    g_BufferPool.ForEachAlive([](VulkanBuffer& buffer, BufferHandle handle) {
        RENDERX_INFO("Buffer[{}] | VkBuffer={} | Size={} | BindCount={}",
                     handle.id,
                     fmt::ptr(buffer.buffer),
                     buffer.allocInfo.size,
                     buffer.bindingCount);
    });

    RENDERX_INFO("---- Buffer Views ----");
    g_BufferViewPool.ForEachAlive([](VulkanBufferView& view, BufferViewHandle handle) {
        RENDERX_INFO("BufferView[{}] | Parent Buffer=[{}] | Valid={}", handle.id, view.buffer.id, view.isValid);
    });

    RENDERX_INFO("---- Textures ----");
    g_TexturePool.ForEachAlive([](VulkanTexture& tex, TextureHandle handle) {
        RENDERX_INFO("Texture[{}] | VkImage={} | {}x{} | Mips={} | Format={}",
                     handle.id,
                     fmt::ptr(tex.image),
                     tex.width,
                     tex.height,
                     tex.mipLevels,
                     (uint32_t)tex.format);
    });

    RENDERX_INFO("---- Texture Views ----");
    g_TextureViewPool.ForEachAlive([](VulkanTextureView& view, TextureViewHandle handle) {
        RENDERX_INFO("TextureView[{}] | VkImageView={}", handle.id, fmt::ptr(view.view));
    });

    RENDERX_INFO("---- Shaders ----");
    g_ShaderPool.ForEachAlive([](VulkanShader& shader, ShaderHandle handle) {
        RENDERX_INFO("Shader[{}] | VkShaderModule={} | EntryPoint={}",
                     handle.id,
                     fmt::ptr(shader.shaderModule),
                     shader.entryPoint);
    });

    RENDERX_INFO("---- Pipeline Layouts ----");
    g_PipelineLayoutPool.ForEachAlive([](VulkanPipelineLayout& layout, PipelineLayoutHandle handle) {
        RENDERX_INFO("PipelineLayout[{}] | VkPipelineLayout={}", handle.id, fmt::ptr(layout.vkLayout));
    });

    RENDERX_INFO("---- Pipelines ----");
    g_PipelinePool.ForEachAlive([](VulkanPipeline& pipe, PipelineHandle handle) {
        RENDERX_INFO("Pipeline[{}] | VkPipeline={}", handle.id, fmt::ptr(pipe.vkPipeline));
    });

    RENDERX_INFO("---- Render Passes ----");
    g_RenderPassPool.ForEachAlive([](VulkanRenderPass& rp, RenderPassHandle handle) {
        RENDERX_INFO("RenderPass[{}] | VkRenderPass={}", handle.id, fmt::ptr(rp.renderPass));
    });
    RENDERX_INFO("---- Framebuffers ----");
    g_FramebufferPool.ForEachAlive([](VulkanFramebuffer& fb, FramebufferHandle handle) {
        RENDERX_INFO("Framebuffer[{}] | VkFramebuffer={}", handle.id, fmt::ptr(fb.framebuffer));
    });

    RENDERX_INFO("---- Descriptor Sets ----");
    g_SetPool.ForEachAlive([](VulkanSet& set, SetHandle handle) {
        RENDERX_INFO("DescriptorSet[{}] | VkDescriptorSet={}", handle.id, fmt::ptr(set.vkSet));
    });

    RENDERX_INFO("---- Descriptor Set Layouts ----");
    g_SetLayoutPool.ForEachAlive([](VulkanSetLayout& layout, SetLayoutHandle handle) {
        RENDERX_INFO("SetLayout[{}] | VkDescriptorSetLayout={}", handle.id, fmt::ptr(layout.vkLayout));
    });

    RENDERX_INFO("---- Descriptor Pools ----");
    g_DescriptorPoolPool.ForEachAlive([](VulkanDescriptorPool& pool, DescriptorPoolHandle handle) {
        RENDERX_INFO("DescriptorPool[{}] | VkDescriptorPool={}", handle.id, fmt::ptr(pool.vkPool));
    });

    RENDERX_INFO("---- Descriptor Heaps ----");
    g_DescriptorHeapPool.ForEachAlive([](VulkanDescriptorHeap& heap, DescriptorHeapHandle handle) {
        RENDERX_INFO(
            "DescriptorHeap[{}] | Buffer={} | GPUAddress={}", handle.id, fmt::ptr(heap.buffer), heap.gpuAddress);
    });

    RENDERX_INFO("---- Samplers ----");
    g_SamplerPool.ForEachAlive([](VulkanSampler& sampler, SamplerHandle handle) {
        RENDERX_INFO("Sampler[{}] | VkSampler={}", handle.id, fmt::ptr(sampler.vkSampler));
    });
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

    vkDeviceWaitIdle(g_Device);

    g_BufferViewPool.ForEach([](VulkanBufferView& view) {
        view.buffer = BufferHandle(0);
        ;
        view.isValid = false;
    });

    g_BufferPool.ForEach([&](VulkanBuffer& buffer) {
        if (buffer.buffer != VK_NULL_HANDLE) {
            ctx.allocator->destroyBuffer(buffer.buffer, buffer.allocation);
            buffer.buffer     = VK_NULL_HANDLE;
            buffer.allocation = VK_NULL_HANDLE;
        }

        buffer.bindingCount = 0;
        buffer.allocInfo    = {};
    });

    g_TextureViewPool.ForEach([&](VulkanTextureView& view) {
        if (view.view != VK_NULL_HANDLE) {
            vkDestroyImageView(g_Device, view.view, nullptr);
            view.view = VK_NULL_HANDLE;
        }
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
        if (layout.vkLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(g_Device, layout.vkLayout, nullptr);
            layout.vkLayout = VK_NULL_HANDLE;
        }
    });

    g_PipelinePool.ForEach([&](VulkanPipeline& pipeline) {
        if (pipeline.vkPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(g_Device, pipeline.vkPipeline, nullptr);
            pipeline.vkPipeline = VK_NULL_HANDLE;
        }
    });
    g_RenderPassPool.ForEach([&](VulkanRenderPass& rp) {
        if (rp.renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(g_Device, rp.renderPass, nullptr);
            rp.renderPass = VK_NULL_HANDLE;
        }
    });

    g_FramebufferPool.ForEach([&](VulkanFramebuffer& fb) {
        if (fb.framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(g_Device, fb.framebuffer, nullptr);
            fb.framebuffer = VK_NULL_HANDLE;
        }
    });

    g_SetPool.ForEach([](VulkanSet& set) { set.vkSet = VK_NULL_HANDLE; });

    g_SetLayoutPool.ForEach([&](VulkanSetLayout& layout) {
        if (layout.vkLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(g_Device, layout.vkLayout, nullptr);
            layout.vkLayout = VK_NULL_HANDLE;
        }
    });

    g_DescriptorPoolPool.ForEach([&](VulkanDescriptorPool& pool) {
        if (pool.vkPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(g_Device, pool.vkPool, nullptr);
            pool.vkPool = VK_NULL_HANDLE;
        }
    });

    g_DescriptorHeapPool.ForEach([&](VulkanDescriptorHeap& heap) {
        if (heap.buffer != VK_NULL_HANDLE) {
            ctx.allocator->destroyBuffer(heap.buffer, heap.allocation);
            heap.buffer     = VK_NULL_HANDLE;
            heap.allocation = VK_NULL_HANDLE;
        }
    });

    g_SamplerPool.ForEach([&](VulkanSampler& sampler) {
        if (sampler.vkSampler != VK_NULL_HANDLE) {
            vkDestroySampler(g_Device, sampler.vkSampler, nullptr);
            sampler.vkSampler = VK_NULL_HANDLE;
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
    g_DescriptorPoolPool.clear();
    g_SetLayoutPool.clear();
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
    delete ctx.loadTimeStagingUploader;
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
