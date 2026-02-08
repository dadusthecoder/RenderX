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

		vkCreateSemaphore(m_Device, &info, nullptr, &m_Semaphore);
	}

	VulkanCommandQueue::~VulkanCommandQueue() {
		if (m_Semaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(m_Device, m_Semaphore, nullptr);
	}

	VkQueue VulkanCommandQueue::Queue() {
		return m_Queue;
	}

	std::shared_ptr<VulkanCommandQueue::VulkanCommandPool> VulkanCommandQueue::CommandPool() {
		if (m_CurrentCommandPool->pool != VK_NULL_HANDLE)
			return m_CurrentCommandPool;
		else if (!m_Free.empty()) {
			m_CurrentCommandPool = m_Free.back();
			m_Free.pop_back();
		}
		else {
			auto pool = std::make_shared<VulkanCommandQueue::VulkanCommandPool>();
			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = m_Family;
			poolInfo.flags =
				VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
				VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

			VK_CHECK(vkCreateCommandPool(
				m_Device, &poolInfo, nullptr, &pool->pool));
			m_CurrentCommandPool = pool;
		}
		return m_CurrentCommandPool;
	}


	Timeline VulkanCommandQueue::Submit(CommandList* commandList) {
		++m_Submitted;
		VulkanCommandList* list = static_cast<VulkanCommandList*>(commandList);

		m_SignalSemaphores.push_back(m_Semaphore);
		m_SignalValues.push_back(m_Submitted);

		VkTimelineSemaphoreSubmitInfo timeline{};
		timeline.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timeline.waitSemaphoreValueCount = uint32_t(m_WaitValues.size());
		timeline.pWaitSemaphoreValues = m_WaitValues.data();
		timeline.signalSemaphoreValueCount = uint32_t(m_SignalValues.size());
		timeline.pSignalSemaphoreValues = m_SignalValues.data();

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pNext = &timeline;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &list->m_CommandBuffer;
		submit.waitSemaphoreCount = uint32_t(m_WaitSemaphores.size());
		submit.pWaitSemaphores = m_WaitSemaphores.data();
		submit.signalSemaphoreCount = uint32_t(m_SignalSemaphores.size());
		submit.pSignalSemaphores = m_SignalSemaphores.data();

		vkQueueSubmit(m_Queue, 1, &submit, VK_NULL_HANDLE);

		m_WaitSemaphores.clear();
		m_WaitValues.clear();
		m_SignalSemaphores.clear();
		m_SignalValues.clear();

		return Timeline(m_Submitted);
	}

	Timeline VulkanCommandQueue::Submit(const SubmitInfo& submitInfo) {
		++m_Submitted;
		auto& ctx = GetVulkanContext();
		for (auto depends : submitInfo.waitDependencies) {
			switch (depends.waitQueue) {
			case QueueType::COMPUTE: {
				addWait(ctx.computeQueue->Semaphore(), depends.waitValue.value);
				break;
			}
			case QueueType::TRANSFER: {
				addWait(ctx.transferQueue->Semaphore(), depends.waitValue.value);
				break;
			}
			case QueueType::GRAPHICS: {
				addWait(ctx.graphicsQueue->Semaphore(), depends.waitValue.value);
				break;
			}
			default:
				break;
			}
		}

		std::vector<VkCommandBuffer> cmds;

		for (uint32_t i = 0; i < submitInfo.commandListCount; ++i) {
			VulkanCommandList* list = static_cast<VulkanCommandList*>(submitInfo.commandList);
			cmds.push_back(list->m_CommandBuffer);
		}


		m_SignalSemaphores.push_back(m_Semaphore);
		m_SignalValues.push_back(m_Submitted);

		VkTimelineSemaphoreSubmitInfo timeline{};
		timeline.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timeline.waitSemaphoreValueCount = uint32_t(m_WaitValues.size());
		timeline.pWaitSemaphoreValues = m_WaitValues.data();
		timeline.signalSemaphoreValueCount = uint32_t(m_SignalValues.size());
		timeline.pSignalSemaphoreValues = m_SignalValues.data();

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pNext = &timeline;
		submit.commandBufferCount = submitInfo.commandListCount;
		submit.pCommandBuffers = cmds.data();
		submit.waitSemaphoreCount = uint32_t(m_WaitSemaphores.size());
		submit.pWaitSemaphores = m_WaitSemaphores.data();
		submit.signalSemaphoreCount = uint32_t(m_SignalSemaphores.size());
		submit.pSignalSemaphores = m_SignalSemaphores.data();

		vkQueueSubmit(m_Queue, 1, &submit, VK_NULL_HANDLE);

		m_WaitSemaphores.clear();
		m_WaitValues.clear();
		m_SignalSemaphores.clear();
		m_SignalValues.clear();

		return Timeline(m_Submitted);
	}

	void VulkanCommandQueue::Flush() {
		for (auto it = m_InFlight.begin(); it != m_InFlight.end(); ++it) {
			if (it->get()->pool != VK_NULL_HANDLE) {
				vkResetCommandPool(m_Device, it->get()->pool, 0);
				m_InFlight.erase(it);
				m_Free.push_back(*it);
			}
		}
	}

	void VulkanCommandQueue::addWait(VkSemaphore semaphore, uint64_t value) {
		RENDERX_ASSERT_MSG(semaphore != VK_NULL_HANDLE, " vulkanQueue : semaphore id VK_NULL_HANDLE");
		m_WaitSemaphores.push_back(semaphore);
		m_WaitValues.push_back(value);
	}

	void VulkanCommandQueue::addSignal(VkSemaphore semaphore, uint64_t value) {
		RENDERX_ASSERT_MSG(semaphore != VK_NULL_HANDLE, " vulkanQueue : semaphore id VK_NULL_HANDLE");
		m_SignalSemaphores.push_back(semaphore);
		m_SignalValues.push_back(value);
	}

	//
	CommandList* VulkanCommandQueue::CreateCommandList(const char* debugName) {
		auto pool = CommandPool();
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = pool->pool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		VkCommandBuffer buffer;
		VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocInfo, &buffer));
		return new VulkanCommandList(buffer, pool->pool, m_Type);
	}

	void VulkanCommandQueue::DestroyCommandList(CommandList* commandList) {
		delete commandList;
	}

	bool VulkanCommandQueue::Wait(Timeline value, uint64_t timeout) {
		VkSemaphoreWaitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		info.semaphoreCount = 1;
		info.pSemaphores = &m_Semaphore;
		info.pValues = &value.value;
		return CheckVk(vkWaitSemaphores(m_Device, &info, timeout));
	}

	void VulkanCommandQueue::WaitIdle() {
		VkSemaphoreWaitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		info.semaphoreCount = 1;
		info.pSemaphores = &m_Semaphore;
		info.pValues = &m_Submitted;
		CheckVk(vkWaitSemaphores(m_Device, &info, UINT64_MAX));
	}

	bool VulkanCommandQueue::Poll(Timeline value) {
		if (value.value == 0 || value.value > m_Submitted)
			return false;

		vkGetSemaphoreCounterValue(m_Device, m_Semaphore, &m_Completed);
		return value.value < m_Completed;
	}

	Timeline VulkanCommandQueue::Completed() const {
		return Timeline(m_Completed);
	}

	Timeline VulkanCommandQueue::Submitted() const {
		return Timeline(m_Submitted);
	}

	uint64_t VulkanCommandQueue::TimestampFrequency() const {
		return 64;
	}

}
