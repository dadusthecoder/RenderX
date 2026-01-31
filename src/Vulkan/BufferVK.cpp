#include "RenderX/Log.h"
#include "RenderXVK.h"
#include "CommonVK.h"

namespace Rx {

namespace RxVK {

	std::unordered_map<Hash64, BufferViewHandle> g_BufferViewCache;
	ResourcePool<VulkanBufferView, BufferViewHandle> g_BufferViewPool;

	// Helper functions - VMA handles memory type selection now

	inline Hash64 ComputeBufferViewHash(const BufferViewDesc& desc) {
		Hash64 hash = 0xcbf29ce484222325;

		hash ^= desc.buffer.id;
		hash *= 0x100000001b3;

		hash ^= desc.offset;
		hash *= 0x100000001b3;

		hash ^= desc.range;
		hash *= 0x100000001b3;

		return hash;
	}

	void UploadViaStagingBufferVK(VulkanBuffer& destBuffer, const void* data, size_t size, size_t offset = 0) {
		PROFILE_FUNCTION();
		
		// Safety validation
		if (!data) {
			RENDERX_ERROR("UploadViaStagingBufferVK: data pointer is null");
			return;
		}
		if (size == 0) {
			RENDERX_WARN("UploadViaStagingBufferVK: size is zero");
			return;
		}
		if (destBuffer.buffer == VK_NULL_HANDLE) {
			RENDERX_ERROR("UploadViaStagingBufferVK: destBuffer is VK_NULL_HANDLE");
			return;
		}
		if (offset + size > destBuffer.size) {
			RENDERX_ERROR("UploadViaStagingBufferVK: offset ({}) + size ({}) exceeds buffer size ({})", 
				offset, size, destBuffer.size);
			return;
		}
		
		VulkanContext& ctx = GetVulkanContext();

		// Create staging buffer with VMA
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
						  VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
		VmaAllocationInfo stagingAllocInfo;

		VkResult result = vmaCreateBuffer(ctx.allocator, &bufferInfo, &allocInfo,
			&stagingBuffer, &stagingAllocation, &stagingAllocInfo);
		if (!CheckVk(result, "Failed to create Vulkan staging buffer")) {
			return;
		}

		// Upload data to staging buffer (already mapped)
		if (!stagingAllocInfo.pMappedData) {
			RENDERX_ERROR("UploadViaStagingBufferVK: staging buffer mapping failed");
			vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingAllocation);
			return;
		}
		memcpy(stagingAllocInfo.pMappedData, data, size);

		// Copy from staging buffer to destination buffer
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

		VkCommandBufferAllocateInfo allocInfoCmd{};
		allocInfoCmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfoCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfoCmd.commandPool = GetFrameContex(g_CurrentFrame).commandPool;
		allocInfoCmd.commandBufferCount = 1;
		
		if (allocInfoCmd.commandPool == VK_NULL_HANDLE) {
			RENDERX_ERROR("UploadViaStagingBufferVK: command pool is VK_NULL_HANDLE");
			vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingAllocation);
			return;
		}
		
		result = vkAllocateCommandBuffers(ctx.device, &allocInfoCmd, &commandBuffer);
		if (!CheckVk(result, "Failed to allocate command buffer for staging copy")) {
			vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingAllocation);
			return;
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if (!CheckVk(result, "Failed to begin command buffer")) {
			vkFreeCommandBuffers(ctx.device, allocInfoCmd.commandPool, 1, &commandBuffer);
			vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingAllocation);
			return;
		}

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = offset;
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer, destBuffer.buffer, 1, &copyRegion);

		result = vkEndCommandBuffer(commandBuffer);
		if (!CheckVk(result, "Failed to end command buffer")) {
			vkFreeCommandBuffers(ctx.device, allocInfoCmd.commandPool, 1, &commandBuffer);
			vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingAllocation);
			return;
		}

		// Submit command buffer
		if (ctx.graphicsQueue == VK_NULL_HANDLE) {
			RENDERX_ERROR("UploadViaStagingBufferVK: graphics queue is VK_NULL_HANDLE");
			vkFreeCommandBuffers(ctx.device, allocInfoCmd.commandPool, 1, &commandBuffer);
			vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingAllocation);
			return;
		}
		
		VkSubmitInfo submitinfo{};
		submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitinfo.commandBufferCount = 1;
		submitinfo.pCommandBuffers = &commandBuffer;
		result = vkQueueSubmit(ctx.graphicsQueue, 1, &submitinfo, VK_NULL_HANDLE);
		if (!CheckVk(result, "Failed to submit command buffer")) {
			vkFreeCommandBuffers(ctx.device, allocInfoCmd.commandPool, 1, &commandBuffer);
			vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingAllocation);
			return;
		}
		
		result = vkQueueWaitIdle(ctx.graphicsQueue);
		if (!CheckVk(result, "Failed to wait for queue idle")) {
			RENDERX_WARN("Queue wait idle failed, but continuing cleanup");
		}

		// Cleanup staging buffer
		vkFreeCommandBuffers(ctx.device, allocInfoCmd.commandPool, 1, &commandBuffer);
		vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingAllocation);
	}

	// ==== BUFFER ====
	BufferHandle VKCreateBuffer(const BufferDesc& desc) {
		PROFILE_FUNCTION();
		
		// Safety validation
		if (desc.size == 0) {
			RENDERX_ERROR("VKCreateBuffer: buffer size is zero");
			return BufferHandle{};
		}
		
		VulkanContext& ctx = GetVulkanContext();
		if (ctx.allocator == VK_NULL_HANDLE) {
			RENDERX_ERROR("VKCreateBuffer: VMA allocator is VK_NULL_HANDLE");
			return BufferHandle{};
		}
		
		// Create buffer
		VulkanBuffer vulkanBuffer{};
		vulkanBuffer.size = desc.size;
		vulkanBuffer.flags = desc.flags;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = desc.size;
		bufferInfo.usage = ToVkBufferFlags(desc.flags);
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		auto memFlags = ToVmaMomoryType(desc.momory, desc.flags);
		VkResult result = vmaCreateBuffer(ctx.allocator, &bufferInfo, &memFlags,
			&vulkanBuffer.buffer, &vulkanBuffer.allocation, &vulkanBuffer.allocInfo);
		if (!CheckVk(result, "Failed to create Vulkan buffer")) {
			return BufferHandle{};
		}

		// Upload initial data if provided
		if (desc.initialData) {
			if (Has(desc.flags, BufferFlags::Static)) {
				UploadViaStagingBufferVK(vulkanBuffer, desc.initialData, desc.size);
			}
			else {
				// Direct upload for dynamic and stream buffers
				void* ptr = nullptr;
				if (vulkanBuffer.allocInfo.pMappedData) {
					// Already persistently mapped
					ptr = vulkanBuffer.allocInfo.pMappedData;
				}
				else {
					VkResult mapResult = vmaMapMemory(ctx.allocator, vulkanBuffer.allocation, &ptr);
					if (mapResult != VK_SUCCESS) {
						RENDERX_ERROR("Failed to map memory for buffer upload: {}", VkResultToString(mapResult));
						vmaDestroyBuffer(ctx.allocator, vulkanBuffer.buffer, vulkanBuffer.allocation);
						return BufferHandle{};
					}
				}
				if (!ptr) {
					RENDERX_ERROR("Buffer mapping resulted in null pointer");
					if (!vulkanBuffer.allocInfo.pMappedData) {
						vmaUnmapMemory(ctx.allocator, vulkanBuffer.allocation);
					}
					vmaDestroyBuffer(ctx.allocator, vulkanBuffer.buffer, vulkanBuffer.allocation);
					return BufferHandle{};
				}
				memcpy(ptr, desc.initialData, desc.size);
				if (!vulkanBuffer.allocInfo.pMappedData) {
					vmaUnmapMemory(ctx.allocator, vulkanBuffer.allocation);
				}
			}
		}

		vulkanBuffer.bindingCount = desc.bindingCount;
		BufferHandle handle = g_BufferPool.allocate(vulkanBuffer);
		RENDERX_INFO("Vulkan: Created Buffer | ID: {} | Size: {} bytes", handle.id, desc.size);
		return handle;
	}

	void VKDestroyBuffer(BufferHandle& handle) {
		PROFILE_FUNCTION();
		
		if (!handle.IsValid()) {
			RENDERX_WARN("VKDestroyBuffer: invalid buffer handle");
			return;
		}
		
		if (!g_BufferPool.IsAlive(handle)) {
			RENDERX_WARN("VKDestroyBuffer: buffer handle is stale");
			return;
		}
		
		RxVK::VulkanContext& ctx = RxVK::GetVulkanContext();
		if (ctx.allocator == VK_NULL_HANDLE) {
			RENDERX_ERROR("VKDestroyBuffer: allocator is VK_NULL_HANDLE");
			return;
		}

		auto* buffer = g_BufferPool.get(handle);
		if (!buffer) {
			RENDERX_ERROR("VKDestroyBuffer: failed to retrieve buffer from pool");
			return;
		}
		
		if (buffer->buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(ctx.allocator, buffer->buffer, buffer->allocation);
			g_BufferPool.free(handle);
		}
		else {
			RENDERX_WARN("VKDestroyBuffer: buffer handle already null");
		}
	}

	void VKCmdCopyBuffer(const CommandList& cmdList, BufferHandle src, BufferHandle dst) {
	}

	inline void* AllocateUpload(
		VulkanUploadContext& upload,
		uint32_t size,
		uint32_t alignment,
		uint32_t& outOffset) {
		uint32_t aligned =
			(upload.offset + alignment - 1) & ~(alignment - 1);

		RENDERX_ASSERT(aligned + size <= upload.size);

		outOffset = aligned;
		upload.offset = aligned + size;

		return upload.mappedPtr + aligned;
	}

	void VKCmdWriteBuffer(const CommandList& cmdList, BufferHandle handle, void* data, uint32_t offset, uint32_t size) {
		PROFILE_FUNCTION();
		RENDERX_ASSERT_MSG(cmdList.IsValid(), "CommandList is not a valid handle");
		RENDERX_ASSERT_MSG(handle.IsValid(), "Invalid BufferHandle");
		RENDERX_ASSERT_MSG(g_BufferPool.IsAlive(handle), "Buffer is not alive");
		RENDERX_ASSERT(data);

		auto& frame = GetFrameContex(g_CurrentFrame);
		auto& ctx = GetVulkanContext();
		auto* cmd = g_CommandListPool.get(cmdList);
		auto* buf = g_BufferPool.get(handle);
		
		if (!cmd || !buf) {
			RENDERX_ERROR("VKCmdWriteBuffer: failed to retrieve command list or buffer from pool");
			return;
		}
		
		auto flags = buf->flags;
		// Determine which upload path to use based on buffer flags

		uint32_t alignment = Has(flags, BufferFlags::Uniform)
			? ctx.deviceLimits.minUniformBufferOffsetAlignment
			: ctx.deviceLimits.minStorageBufferOffsetAlignment;
		
		if (alignment == 0) {
			RENDERX_ERROR("VKCmdWriteBuffer: invalid buffer alignment (0)");
			return;
		}
		
		uint32_t uploadOffset;
		void* ptr = AllocateUpload(frame.upload, size, alignment, uploadOffset);
		if (!ptr) {
			RENDERX_ERROR("VKCmdWriteBuffer: failed to allocate upload memory");
			return;
		}
		memcpy(ptr, data, size);

		VkBufferCopy copy{};
		copy.srcOffset = uploadOffset;
		copy.dstOffset = offset;
		copy.size = size;
		vkCmdCopyBuffer(cmd->cmdBuffer, frame.upload.buffer, buf->buffer, 1, &copy);
		return;
	}

	BufferViewHandle VKCreateBufferView(const BufferViewDesc& desc) {
		if (!desc.buffer.IsValid()) {
			RENDERX_ERROR("VKCreateBufferView: invalid buffer handle");
			return BufferViewHandle{};
		}
		
		if (!g_BufferPool.IsAlive(desc.buffer)) {
			RENDERX_ERROR("VKCreateBufferView: buffer handle is stale");
			return BufferViewHandle{};
		}
		
		VulkanBuffer* buffer = g_BufferPool.get(desc.buffer);
		if (!buffer || buffer->buffer == VK_NULL_HANDLE) {
			RENDERX_ERROR("VKCreateBufferView: buffer is VK_NULL_HANDLE");
			return BufferViewHandle{};
		}
		
		if (desc.offset + desc.range > buffer->size) {
			RENDERX_ERROR("VKCreateBufferView: view range exceeds buffer size (offset: {}, range: {}, buffer size: {})",
				desc.offset, desc.range, buffer->size);
			return BufferViewHandle{};
		}

		Hash64 hash = ComputeBufferViewHash(desc);

		auto it = g_BufferViewCache.find(hash);
		if (it != g_BufferViewCache.end())
			return it->second;

		VulkanBufferView bufferview{};
		bufferview.buffer = desc.buffer;
		bufferview.range = desc.range;
		bufferview.offset = desc.offset;
		bufferview.hash = hash;
		bufferview.isValid = true;

		auto handle = g_BufferViewPool.allocate(bufferview);
		g_BufferViewCache[hash] = handle;
		return handle;
	}

	void VKDestroyBufferView(BufferViewHandle& handle) {
		g_BufferViewPool.free(handle);
	}

	// command list related functions can go here
	void VKCmdSetVertexBuffer(CommandList& cmdList, BufferHandle handle, uint64_t offset) {
		PROFILE_FUNCTION();

		RENDERX_ASSERT_MSG(cmdList.IsValid(), "VKCmdSetVertexBuffer: Invalid CommandList");
		auto* vkCmdList = g_CommandListPool.get(cmdList);
		RENDERX_ASSERT_MSG(vkCmdList != nullptr, "VKCmdSetVertexBuffer: retrieved null command list");
		RENDERX_ASSERT_MSG(vkCmdList->isOpen, "VKCmdSetVertexBuffer: CommandList must be open");
		RENDERX_ASSERT_MSG(handle.IsValid(), "VKCmdSetVertexBuffer: Invalid BufferHandle");

		auto& frame = GetFrameContex(g_CurrentFrame);

		// get from ResourcePool
		auto* buffer = g_BufferPool.get(handle);
		RENDERX_ASSERT_MSG(buffer != nullptr, "VKCmdSetVertexBuffer: retrieved null buffer");
		RENDERX_ASSERT_MSG(buffer->buffer != VK_NULL_HANDLE, "VKCmdSetVertexBuffer: Buffer is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(offset <= buffer->size, "VKCmdSetVertexBuffer: offset exceeds buffer size");

		VkDeviceSize vkOffset = static_cast<VkDeviceSize>(offset);

		vkCmdBindVertexBuffers(
			vkCmdList->cmdBuffer,
			0,					  // binding
			buffer->bindingCount, // binding count
			&buffer->buffer,
			&vkOffset);
	}

	void VKCmdSetIndexBuffer(CommandList& cmdList, BufferHandle buffer, uint64_t offset) {
		PROFILE_FUNCTION();

		RENDERX_ASSERT_MSG(cmdList.IsValid(), "VKCmdSetIndexBuffer: Invalid CommandList");
		auto* vkCmdList = g_CommandListPool.get(cmdList);
		RENDERX_ASSERT_MSG(vkCmdList != nullptr, "VKCmdSetIndexBuffer: retrieved null command list");
		RENDERX_ASSERT_MSG(vkCmdList->isOpen, "VKCmdSetIndexBuffer: CommandList must be open");
		RENDERX_ASSERT_MSG(buffer.IsValid(), "VKCmdSetIndexBuffer: Invalid BufferHandle");

		// Try to get from ResourcePool first
		auto* bufferData = g_BufferPool.get(buffer);
		RENDERX_ASSERT_MSG(bufferData != nullptr, "VKCmdSetIndexBuffer: retrieved null buffer");
		RENDERX_ASSERT_MSG(bufferData->buffer != VK_NULL_HANDLE, "VKCmdSetIndexBuffer: Buffer is VK_NULL_HANDLE");
		RENDERX_ASSERT_MSG(offset <= bufferData->size, "VKCmdSetIndexBuffer: offset exceeds buffer size");

		// RENDERX_ASSERT_MSG(it->second.type == BufferType::Index, "Buffer is not an index buffer");

		// Choose index type (extend this later)
		VkIndexType indexType = VK_INDEX_TYPE_UINT32;

		vkCmdBindIndexBuffer(
			vkCmdList->cmdBuffer,
			bufferData->buffer,
			offset,
			indexType);
	}

} // namespace RxVk

} // namespace Rx