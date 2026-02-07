#include "VK_Common.h"
#include "VK_RenderX.h"

#include <Windows.h>
#include <vector>
#include <cstring>

namespace Rx::RxVK {

	VulkanQueue::VulkanQueue(VkDevice device, VkQueue queue, uint32_t family)
		: m_Device(device), m_Queue(queue), m_Family(family) {
		VkSemaphoreTypeCreateInfo typeInfo{};
		typeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		typeInfo.initialValue = 0;

		VkSemaphoreCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info.pNext = &typeInfo;

		vkCreateSemaphore(m_Device, &info, nullptr, &m_Timeline);
	}

	VulkanQueue::~VulkanQueue() {
		vkDestroySemaphore(m_Device, m_Timeline, nullptr);
	}

	VulkanCommandBuffer* VulkanQueue::acquire() {
		std::lock_guard<std::mutex> lock(m_Mutex);

		std::shared_ptr<VulkanCommandBuffer> cmd;

		if (m_Free.empty()) {
			cmd = std::make_shared<VulkanCommandBuffer>();
			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = m_Family;
			poolInfo.flags =
				VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
				VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

			VK_CHECK(vkCreateCommandPool(
				m_Device, &poolInfo, nullptr, &cmd->pool));

			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = cmd->pool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;
			VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocInfo, &cmd->buffer));
		}
		else {
			cmd = m_Free.front();
			m_Free.pop_front();
			vkResetCommandPool(m_Device, cmd->pool, 0);
		}

		m_InFlight.push_back(cmd);
		return cmd.get();
	}


	uint64_t VulkanQueue::submit(VulkanCommandBuffer** buffers, uint32_t count) {
		m_Submitted++;

		std::vector<VkCommandBuffer> vkBuffers(count);
		for (uint32_t i = 0; i < count; i++) {
			buffers[i]->submissionID = m_Submitted;
			vkBuffers[i] = buffers[i]->buffer;
			m_InFlight.emplace_back(buffers[i]);
		}

		m_SignalSemaphores.push_back(m_Timeline);
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
		submit.commandBufferCount = count;
		submit.pCommandBuffers = vkBuffers.data();
		submit.waitSemaphoreCount = uint32_t(m_WaitSemaphores.size());
		submit.pWaitSemaphores = m_WaitSemaphores.data();
		submit.signalSemaphoreCount = uint32_t(m_SignalSemaphores.size());
		submit.pSignalSemaphores = m_SignalSemaphores.data();

		vkQueueSubmit(m_Queue, 1, &submit, VK_NULL_HANDLE);

		m_WaitSemaphores.clear();
		m_WaitValues.clear();
		m_SignalSemaphores.clear();
		m_SignalValues.clear();

		return m_Submitted;
	}

	bool VulkanQueue::submitImmediate(VkCommandBuffer cmdBuffer, VkFence fence) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		VkResult result = vkQueueSubmit(m_Queue, 1, &submitInfo, fence);
		return result == VK_SUCCESS;
	}


	void VulkanQueue::addWait(VkSemaphore semaphore, uint64_t value) {
		RENDERX_ASSERT_MSG(semaphore != VK_NULL_HANDLE, " vulkanQueue : semaphore id VK_NULL_HANDLE");
		m_WaitSemaphores.push_back(semaphore);
		m_WaitValues.push_back(value);
	}

	void VulkanQueue::addSignal(VkSemaphore semaphore, uint64_t value) {
		RENDERX_ASSERT_MSG(semaphore != VK_NULL_HANDLE, " vulkanQueue : semaphore id VK_NULL_HANDLE");
		m_SignalSemaphores.push_back(semaphore);
		m_SignalValues.push_back(value);
	}

	bool VulkanQueue::poll(uint64_t submissionID) {
		if (submissionID == 0 || submissionID > m_Submitted)
			return false;

		vkGetSemaphoreCounterValue(m_Device, m_Timeline, &m_Completed);
		return m_Completed >= submissionID;
	}

	void VulkanQueue::wait(uint64_t id, uint64_t timeout) {
		VkSemaphoreWaitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		info.semaphoreCount = 1;
		info.pSemaphores = &m_Timeline;
		info.pValues = &id;
		vkWaitSemaphores(m_Device, &info, timeout);
	}

	void VulkanQueue::retire() {
		vkGetSemaphoreCounterValue(m_Device, m_Timeline, &m_Completed);
		std::vector<std::shared_ptr<VulkanCommandBuffer>> remaining;
		for (auto& cmd : m_InFlight) {
			if (cmd->submissionID <= m_Completed) {
				vkResetCommandPool(m_Device, cmd->pool, 0);
				cmd->submissionID = 0;
				m_Free.emplace_back(std::move(cmd));
			}
			else {
				remaining.emplace_back(std::move(cmd));
			}
		}
		m_InFlight.swap(remaining);
	}

	VkQueue VulkanQueue::getVkQueue() {
		return m_Queue;
	}

	uint64_t VulkanQueue::getLastSubmittedID() {
		return m_Submitted;
	}

	uint64_t VulkanQueue::getCompletedID() {
		return m_Completed;
	}

	static bool HasFlag(VkQueueFlags flags, VkQueueFlags bit) {
		return (flags & bit) == bit;
	}

}
