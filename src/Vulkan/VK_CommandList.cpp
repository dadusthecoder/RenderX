#include "RenderX/Log.h"
#include "RenderX/DebugProfiler.h"
#include "VK_RenderX.h"
#include "VK_Common.h"

namespace Rx {

namespace RxVK {

	VulkanCommandList::VulkanCommandList() {
	}

	VulkanCommandList::~VulkanCommandList() {
	}

	void VulkanCommandList::open() {
	}

	void VulkanCommandList::close() {
	}

	void VulkanCommandList::setPipeline(const PipelineHandle& pipeline) {
	}

	void VulkanCommandList::setVertexBuffer(const BufferHandle& buffer, uint64_t offset) {
	}

	void VulkanCommandList::setIndexBuffer(const BufferHandle& buffer, uint64_t offset) {
	}

	void VulkanCommandList::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
	}

	void VulkanCommandList::drawIndexed(uint32_t indexCount, int32_t vertexOffset, uint32_t instanceCount, uint32_t firstIndex, uint32_t firstInstance) {
	}

	void VulkanCommandList::beginRenderPass(RenderPassHandle pass, const void* clearValues, uint32_t clearCount) {
	}

	void VulkanCommandList::endRenderPass() {
	}

	void VulkanCommandList::writeBuffer(BufferHandle handle, const void* data, uint32_t offset, uint32_t size) {
	}

	void VulkanCommandList::setResourceGroup(const ResourceGroupHandle& handle) {
	}

} // namespace RxVK

} // namespace RxRENDERX_ASSERT_MSG
