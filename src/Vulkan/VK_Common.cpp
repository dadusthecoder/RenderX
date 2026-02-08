
#include "VK_Common.h"
#include "VK_RenderX.h"
#include "Windows.h"
#include <algorithm>
#include <vulkan/vulkan_win32.h>

// Define VMA implementation - must be included in exactly one source file
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#ifdef RENDERX_DEBUG
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

namespace Rx {
namespace RxVK {

	uint32_t MAX_FRAMES_IN_FLIGHT;
	static std::vector<FrameContex> g_Frames;

	// ResourcePool instances for resource management
	ResourcePool<VulkanTexture, TextureHandle> g_TexturePool;
	ResourcePool<VulkanBuffer, BufferHandle> g_BufferPool;
	ResourcePool<VulkanShader, ShaderHandle> g_ShaderPool;
	ResourcePool<VulkanPipeline, PipelineHandle> g_PipelinePool;
	ResourcePool<VkRenderPass, RenderPassHandle> g_RenderPassPool;

	// // Legacy unordered_map declarations (deprecated, kept for compatibility)
	// std::unordered_map<uint64_t, VulkanBuffer> g_Buffers;
	// std::unordered_map<uint64_t, VulkanShader> g_Shaders;
	// std::unordered_map<uint64_t, VkRenderPass> g_RenderPasses;

	uint32_t g_CurrentFrame = 0;

	VulkanContext& GetVulkanContext() {
		static VulkanContext g_Context;
		return g_Context;
	}

	FrameContex& GetFrameContex(uint32_t index) {
		return g_Frames[index];
	}

	void freeAllVulkanResources() {
		auto& ctx = GetVulkanContext();
		auto g_Device = ctx.device->logical();
		vkDeviceWaitIdle(ctx.device->logical());

		g_BufferViewPool.ForEach([](VulkanBufferView& view) {
			view.isValid = false;
		});

		g_BufferPool.ForEach([&](VulkanBuffer& buffer) {
			if (buffer.buffer != VK_NULL_HANDLE) {
				ctx.allocator->destroyBuffer(buffer.buffer, buffer.allocation);
				buffer.buffer = VK_NULL_HANDLE;
				buffer.allocation = VK_NULL_HANDLE;
			}

			buffer.bindingCount = 0;
			buffer.allocInfo = {};
		});

		g_TexturePool.ForEach([&](VulkanTexture& texture) {
			if (texture.view != VK_NULL_HANDLE) {
				vkDestroyImageView(g_Device, texture.view, nullptr);
				texture.view = VK_NULL_HANDLE;
			}

			if (texture.image != VK_NULL_HANDLE) {
				vkDestroyImage(g_Device, texture.image, nullptr);
				texture.image = VK_NULL_HANDLE;
			}

			if (texture.memory != VK_NULL_HANDLE) {
				vkFreeMemory(g_Device, texture.memory, nullptr);
				texture.memory = VK_NULL_HANDLE;
			}

			texture.format = VK_FORMAT_UNDEFINED;
			texture.width = 0;
			texture.height = 0;
			texture.mipLevels = 1;
			texture.isValid = false;
		});

		g_ShaderPool.ForEach([&](VulkanShader& shader) {
			if (shader.shaderModule != VK_NULL_HANDLE) {
				vkDestroyShaderModule(g_Device, shader.shaderModule, nullptr);
				shader.shaderModule = VK_NULL_HANDLE;
			}

			shader.entryPoint.clear();
		});

		g_PipelineLayoutPool.ForEach([&](VulkanPipelineLayout& layout) {
			if (layout.layout != VK_NULL_HANDLE && layout.setlayouts.size() != 0) {
				for (auto setlayout : layout.setlayouts)
					vkDestroyDescriptorSetLayout(g_Device, setlayout, nullptr);

				vkDestroyPipelineLayout(g_Device, layout.layout, nullptr);
				layout.layout = VK_NULL_HANDLE;
			}
		});

		g_PipelinePool.ForEach([&](VulkanPipeline& pipeline) {
			if (pipeline.pipeline != VK_NULL_HANDLE) {
				vkDestroyPipeline(g_Device, pipeline.pipeline, nullptr);
				pipeline.pipeline = VK_NULL_HANDLE;
			}
		});

		g_RenderPassPool.ForEach([&](VkRenderPass& rp) {
			if (rp != VK_NULL_HANDLE) {
				vkDestroyRenderPass(g_Device, rp, nullptr);
				rp = VK_NULL_HANDLE;
			}
		});

		g_BufferViewPool.clear();
		g_BufferPool.clear();
		g_TexturePool.clear();
		g_ShaderPool.clear();
		g_PipelineLayoutPool.clear();
		g_PipelinePool.clear();
		g_RenderPassPool.clear();
		g_BufferViewCache.clear();
	}

	void VKShutdownCommon() {
		auto& ctx = GetVulkanContext();
		vkDeviceWaitIdle(ctx.device->logical());
		freeAllVulkanResources();

		// must folllow this order
		ctx.descriptorPoolManager.reset();
		ctx.descriptorSetManager.reset();
		ctx.graphicsQueue.reset();
		ctx.computeQueue.reset();
		ctx.transferQueue.reset();
		ctx.swapchain.reset();
		ctx.allocator.reset();
		ctx.device.reset();
		ctx.instance.reset();
	}

} // namespace RxVK
} // namespace Rx