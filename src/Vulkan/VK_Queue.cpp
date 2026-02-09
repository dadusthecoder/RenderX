#include "VK_Common.h"
#include "VK_RenderX.h"

#include <Windows.h>
#include <vector>
#include <cstring>

namespace Rx::RxVK {


	static bool HasFlag(VkQueueFlags flags, VkQueueFlags bit) {
		return (flags & bit) == bit;
	}

	VulkanCommandQueue::VulkanCommandQueue(VkDevice device, VkQueue queue, uint32_t family, QueueType type)
		: m_Device(device), m_Queue(queue), m_Family(family), m_Type(type) {
		VkSemaphoreTypeCreateInfo typeInfo{};
		typeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		typeInfo.initialValue = 0;
		VkSemaphoreCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info.pNext = &typeInfo;
		VK_CHECK(vkCreateSemaphore(m_Device, &info, nullptr, &m_TimelineSemaphore));
	}

	VulkanCommandQueue::~VulkanCommandQueue() {
		WaitIdle();
		if (m_TimelineSemaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(m_Device, m_TimelineSemaphore, nullptr);
	}

	VkQueue VulkanCommandQueue::Queue() {
		return m_Queue;
	}


	Timeline VulkanCommandQueue::Submit(const SubmitInfo& submitInfo) {
		m_WaitCount = 0;

		auto& ctx = GetVulkanContext();

		for (const auto& dep : submitInfo.waitDependencies) {
			switch (dep.waitQueue) {
			case QueueType::GRAPHICS:
				addWait(
					ctx.graphicsQueue->Semaphore(),
					dep.waitValue.value,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
				break;

			case QueueType::COMPUTE:
				addWait(
					ctx.computeQueue->Semaphore(),
					dep.waitValue.value,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
				break;

			case QueueType::TRANSFER:
				addWait(
					ctx.transferQueue->Semaphore(),
					dep.waitValue.value,
					VK_PIPELINE_STAGE_TRANSFER_BIT);
				break;

			default:
				addWait(
					ctx.graphicsQueue->Semaphore(),
					dep.waitValue.value,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
				break;
			}
		}

		VulkanCommandList* list =
			static_cast<VulkanCommandList*>(submitInfo.commandList);

		const uint64_t signalValue = ++m_Submitted;

		VkTimelineSemaphoreSubmitInfo timeline{};
		timeline.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timeline.waitSemaphoreValueCount = m_WaitCount;
		timeline.pWaitSemaphoreValues = m_WaitValues;
		timeline.signalSemaphoreValueCount = 1;
		timeline.pSignalSemaphoreValues = &signalValue;

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pNext = &timeline;

		submit.waitSemaphoreCount = m_WaitCount;
		submit.pWaitSemaphores = m_WaitSemaphores;
		submit.pWaitDstStageMask = m_WaitStages;

		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &list->m_CommandBuffer;

		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &m_TimelineSemaphore;

		VK_CHECK(vkQueueSubmit(m_Queue, 1, &submit, VK_NULL_HANDLE));

		return Timeline(signalValue);
	}

	CommandAllocator* VulkanCommandQueue::CreateCommandAllocator(const char* debugName) {
		VkCommandPoolCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		ci.queueFamilyIndex = m_Family;
		VkCommandPool pool;
		vkCreateCommandPool(m_Device, &ci, nullptr, &pool);
		return new VulkanCommandAllocator(pool, m_Device);
	}

	void VulkanCommandQueue::DestroyCommandAllocator(CommandAllocator* allocator) {
		WaitIdle();
		VulkanCommandAllocator* vulkanallocator = reinterpret_cast<VulkanCommandAllocator*>(allocator);
		vkDestroyCommandPool(m_Device, vulkanallocator->m_Pool, nullptr);
		delete allocator;
	}

	Timeline VulkanCommandQueue::Submit(CommandList* commandList) {
		VulkanCommandList* list =
			static_cast<VulkanCommandList*>(commandList);

		const uint64_t signalValue = ++m_Submitted;
		VkTimelineSemaphoreSubmitInfo timeline{};
		timeline.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timeline.signalSemaphoreValueCount = 1;
		timeline.pSignalSemaphoreValues = &signalValue;

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pNext = &timeline;

		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &list->m_CommandBuffer;

		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &m_TimelineSemaphore;

		VK_CHECK(vkQueueSubmit(m_Queue, 1, &submit, VK_NULL_HANDLE));

		return Timeline(signalValue);
	}

	void VulkanCommandQueue::addWait(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags stage) {
		RENDERX_ASSERT_MSG(semaphore != VK_NULL_HANDLE, " vulkanQueue : semaphore id VK_NULL_HANDLE");
		RENDERX_ASSERT(m_WaitCount < 2);
		m_WaitSemaphores[m_WaitCount] = semaphore;
		m_WaitValues[m_WaitCount] = value;
		m_WaitStages[m_WaitCount] = stage;
		++m_WaitCount;
	}

	bool VulkanCommandQueue::Wait(Timeline value, uint64_t timeout) {
		PROFILE_FUNCTION();
		VkSemaphoreWaitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		info.semaphoreCount = 1;
		info.pSemaphores = &m_TimelineSemaphore;
		info.pValues = &value.value;
		return CheckVk(vkWaitSemaphores(m_Device, &info, timeout));
	}

	void VulkanCommandQueue::WaitIdle() {
		PROFILE_FUNCTION();
		VkSemaphoreWaitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		info.semaphoreCount = 1;
		info.pSemaphores = &m_TimelineSemaphore;
		info.pValues = &m_Submitted;
		CheckVk(vkWaitSemaphores(m_Device, &info, UINT64_MAX));
	}

	bool VulkanCommandQueue::Poll(Timeline value) {
		if (value.value == 0 || value.value > m_Submitted)
			return false;

		vkGetSemaphoreCounterValue(m_Device, m_TimelineSemaphore, &m_Completed);
		return value.value < m_Completed;
	}

	Timeline VulkanCommandQueue::Completed() {
		vkGetSemaphoreCounterValue(m_Device, m_TimelineSemaphore, &m_Completed);
		return Timeline(m_Completed);
	}

	Timeline VulkanCommandQueue::Submitted() const {
		return Timeline(m_Submitted);
	}

	float VulkanCommandQueue::TimestampFrequency() const {
		return GetVulkanContext().device->limits().timestampPeriod;
	}

	CommandQueue* VKGetGpuQueue(QueueType type) {
		switch (type) {
		case QueueType::GRAPHICS:
			return GetVulkanContext().graphicsQueue.get();
		case QueueType::COMPUTE:
			return GetVulkanContext().computeQueue.get();
		case QueueType::TRANSFER:
			return GetVulkanContext().transferQueue.get();
		default: {
			RENDERX_ERROR("Unknown Queue Type");
			return nullptr;
		}
		}
	}
}
