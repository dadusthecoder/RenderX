#include "RenderX/Log.h"
#include "RenderXVK.h"
#include "CommonVK.h"

namespace Rx {

	inline VkBufferUsageFlags ToVkBufferType(BufferDesc desc) {
		VkBufferUsageFlags usage = 0;
		switch (desc.type) {
		case BufferType::Vertex:
			usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case BufferType::Index:
			usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;
		case BufferType::Uniform:
			usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;
		case BufferType::Storage:
			usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			break;
		case BufferType::Indirect:
			usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			break;
		}

		if (desc.usage == BufferUsage::Static)
			usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		if (desc.initialData)
			usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		return usage;
	}

	inline VkMemoryPropertyFlags ToVkBufferUsage(BufferUsage usage) {
		switch (usage) {
		case BufferUsage::Static:
			return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		case BufferUsage::Dynamic:
		case BufferUsage::Stream:
			return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}

		return 0;
	}

	namespace RxVK {

		// Helper functions
		uint32_t VKFindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
			PROFILE_FUNCTION();
			VkPhysicalDeviceMemoryProperties memPoroerties{};
			vkGetPhysicalDeviceMemoryProperties(RxVK::GetVulkanContext().physicalDevice, &memPoroerties);

			for (uint32_t i = 0; i < memPoroerties.memoryTypeCount; i++) {
				if (typeFilter & (1 << i) && (memPoroerties.memoryTypes[i].propertyFlags & properties) == properties)
					return i;
			}

			RENDERX_ERROR("Failed to find suitable memory type!");
			return 0;
		}

		void UploadViaStagingBufferVK(VulkanBuffer& destBuffer, const void* data, size_t size) {
			PROFILE_FUNCTION();
			// Create staging buffer
			VulkanBuffer stagingBuffer{};
			stagingBuffer.size = size;

			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferInfo.sharingMode = /*TEMP*/ VK_SHARING_MODE_EXCLUSIVE;
			VulkanContext& ctx = GetVulkanContext();
			VkResult result = vkCreateBuffer(ctx.device, &bufferInfo, nullptr, &stagingBuffer.buffer);
			if (!CheckVk(result, "Failed to create Vulkan staging buffer")) {
				return;
			}

			//  Allocate memory for staging buffer
			VkMemoryRequirements memReq{};
			vkGetBufferMemoryRequirements(ctx.device, stagingBuffer.buffer, &memReq);
			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memReq.size;
			allocInfo.memoryTypeIndex = VKFindMemoryType(memReq.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			result = vkAllocateMemory(ctx.device, &allocInfo, nullptr, &stagingBuffer.memory);
			if (!CheckVk(result, "Failed to allocate Vulkan staging buffer memory")) {
				return;
			}

			// Upload data to staging buffer
			vkBindBufferMemory(ctx.device, stagingBuffer.buffer, stagingBuffer.memory, 0);
			void* ptr;
			vkMapMemory(ctx.device, stagingBuffer.memory, 0, stagingBuffer.size, 0, &ptr);
			memcpy(ptr, data, size);
			vkUnmapMemory(ctx.device, stagingBuffer.memory);

			// Copy from staging buffer to destination buffer
			VkCommandBuffer commandBuffer; // TODO: Get a command buffer from a command pool

			VkCommandBufferAllocateInfo allocInfoCmd{};
			allocInfoCmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfoCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfoCmd.commandPool = GetCurrentFrameContex(g_CurrentFrame).commandPool;
			allocInfoCmd.commandBufferCount = 1;
			result = vkAllocateCommandBuffers(ctx.device, &allocInfoCmd, &commandBuffer);
			if (!CheckVk(result, "Failed to allocate command buffer for staging copy")) {
				return;
			}
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(commandBuffer, &beginInfo);
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, destBuffer.buffer, 1, &copyRegion);
			vkEndCommandBuffer(commandBuffer);

			// Submit command buffer
			VkSubmitInfo submitinfo{};
			submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitinfo.commandBufferCount = 1;
			submitinfo.pCommandBuffers = &commandBuffer;
			vkQueueSubmit(ctx.graphicsQueue, 1, &submitinfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(ctx.graphicsQueue);

			// Cleanup staging buffer
			vkFreeCommandBuffers(ctx.device, allocInfoCmd.commandPool, 1, &commandBuffer);
			vkDestroyBuffer(ctx.device, stagingBuffer.buffer, nullptr);
			vkFreeMemory(ctx.device, stagingBuffer.memory, nullptr);
		}

		// ===================== BUFFER =====================
		BufferHandle VKCreateBuffer(const BufferDesc& desc) {
			PROFILE_FUNCTION();
			BufferHandle handle;

			// Create buffer
			VulkanBuffer vulkanBuffer{};
			vulkanBuffer.size = desc.size;

			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = desc.size;
			bufferInfo.usage = ToVkBufferType(desc);
			bufferInfo.sharingMode = /*temp*/ VK_SHARING_MODE_EXCLUSIVE;
			VulkanContext& ctx = GetVulkanContext();
			VkResult result = vkCreateBuffer(ctx.device, &bufferInfo, nullptr, &vulkanBuffer.buffer);
			if (!CheckVk(result, "Failed to create Vulkan buffer")) {
				return BufferHandle{};
			}

			// Allocate memory
			VkMemoryRequirements memeReauirments{};
			vkGetBufferMemoryRequirements(ctx.device, vulkanBuffer.buffer, &memeReauirments);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memeReauirments.size;
			allocInfo.memoryTypeIndex = VKFindMemoryType(memeReauirments.memoryTypeBits, ToVkBufferUsage(desc.usage));

			result = vkAllocateMemory(ctx.device, &allocInfo, nullptr, &vulkanBuffer.memory);
			if (!CheckVk(result, "Failed to allocate Vulkan buffer memory")) {
				return BufferHandle{};
			}


			vkBindBufferMemory(ctx.device, vulkanBuffer.buffer, vulkanBuffer.memory, 0);
			if (desc.initialData) {
				if (desc.usage == BufferUsage::Static) {
					UploadViaStagingBufferVK(vulkanBuffer, desc.initialData, desc.size);
				}
				else {
					// Direct upload for dynamic and stream buffers
					void* ptr;
					vkMapMemory(ctx.device, vulkanBuffer.memory, 0, vulkanBuffer.size, 0, &ptr);
					memcpy(ptr, desc.initialData, desc.size);
					vkUnmapMemory(ctx.device, vulkanBuffer.memory);
				}
			}

			vulkanBuffer.bindingCount = desc.bindingCount;
			handle = g_BufferPool.allocate(vulkanBuffer);
			RENDERX_INFO("Vulkan: Created Buffer | ID: {} | Size: {} bytes", handle.id, desc.size);
			return handle;
		}

		void VKDestroyBuffer(const BufferHandle& handle) {
			PROFILE_FUNCTION();
			RxVK::VulkanContext& ctx = RxVK::GetVulkanContext();

			// Try ResourcePool first
			auto& buffer = g_BufferPool.get(handle);
			if (buffer.buffer != VK_NULL_HANDLE) {
				vkDestroyBuffer(ctx.device, buffer.buffer, nullptr);
				vkFreeMemory(ctx.device, buffer.memory, nullptr);
				g_BufferPool.free(const_cast<BufferHandle&>(handle));
			}
		}

		// command list related functions can go here
		void VKCmdSetVertexBuffer(CommandList& cmdList, BufferHandle handle, uint64_t offset) {
			PROFILE_FUNCTION();

			RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid CommandList");
			RENDERX_ASSERT_MSG(cmdList.isOpen, "CommandList must be open");
			RENDERX_ASSERT_MSG(handle.IsValid(), "Invalid BufferHandle");

			auto& frame = GetCurrentFrameContex(g_CurrentFrame);

			// get from ResourcePool 
			auto& buffer = g_BufferPool.get(handle);
			if (buffer.buffer == VK_NULL_HANDLE) {
				RENDERX_ASSERT(false);
			}

			VkDeviceSize vkOffset = static_cast<VkDeviceSize>(offset);

			vkCmdBindVertexBuffers(
				frame.commandBuffers[cmdList.id],
				0,					 // binding
				buffer.bindingCount, // binding count
				&buffer.buffer,
				&vkOffset);
		}


		void VKCmdSetIndexBuffer(CommandList& cmdList, BufferHandle buffer, uint64_t offset) {
			PROFILE_FUNCTION();
			RENDERX_ASSERT_MSG(cmdList.IsValid(), "Invalid CommandList");
			RENDERX_ASSERT_MSG(cmdList.isOpen, "CommandList must be open");

			auto& frame = GetCurrentFrameContex(g_CurrentFrame);

			// Try to get from ResourcePool first
			auto& bufferData = g_BufferPool.get(buffer);
			if (bufferData.buffer == VK_NULL_HANDLE) {
				RENDERX_ASSERT(false);
			}

			// RENDERX_ASSERT_MSG(it->second.type == BufferType::Index, "Buffer is not an index buffer");

			// Choose index type (extend this later)
			VkIndexType indexType = VK_INDEX_TYPE_UINT32;

			vkCmdBindIndexBuffer(
				frame.commandBuffers[cmdList.id],
				bufferData.buffer,
				offset,
				indexType);
		}


	} // namespace RxVk

} // namespace Rx
