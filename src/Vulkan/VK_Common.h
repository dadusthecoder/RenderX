#pragma once

#include "RenderX/Log.h"
#include "ProLog/ProLog.h"
#include "RenderX/Common.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

#include <vector>
#include <array>
#include <optional>
#include <unordered_map>
#include <limits>
#include <random>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <deque>
#include <mutex>

namespace Rx {
namespace RxVK {

	// Type Aliases & Global Variables

	using Hash64 = uint64_t;

	extern uint32_t MAX_FRAMES_IN_FLIGHT;
	extern uint32_t g_CurrentFrame;

	// Utility Functions

	inline const char* VkResultToString(VkResult result) {
		switch (result) {
		case 0: return "VK_SUCCESS";
		case 1: return "VK_NOT_READY";
		case 2: return "VK_TIMEOUT";
		case 3: return "VK_EVENT_SET";
		case 4: return "VK_EVENT_RESET";
		case 5: return "VK_INCOMPLETE";
		case -1: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case -2: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case -3: return "VK_ERROR_INITIALIZATION_FAILED";
		case -4: return "VK_ERROR_DEVICE_LOST";
		case -5: return "VK_ERROR_MEMORY_MAP_FAILED";
		case -6: return "VK_ERROR_LAYER_NOT_PRESENT";
		case -7: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case -8: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case -9: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case -10: return "VK_ERROR_TOO_MANY_OBJECTS";
		case -11: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case -12: return "VK_ERROR_FRAGMENTED_POOL";
		case -13: return "VK_ERROR_UNKNOWN";
		case -1000069000: return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case -1000072003: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case -1000161000: return "VK_ERROR_FRAGMENTATION";
		case -1000257000: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case 1000297000: return "VK_PIPELINE_COMPILE_REQUIRED";
		case -1000174001: return "VK_ERROR_NOT_PERMITTED";
		case -1000000000: return "VK_ERROR_SURFACE_LOST_KHR";
		case -1000000001: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case 1000001003: return "VK_SUBOPTIMAL_KHR";
		case -1000001004: return "VK_ERROR_OUT_OF_DATE_KHR";
		case -1000003001: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case -1000011001: return "VK_ERROR_VALIDATION_FAILED_EXT";
		case -1000012000: return "VK_ERROR_INVALID_SHADER_NV";
		case -1000023000: return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
		case -1000023001: return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
		case -1000023002: return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
		case -1000023003: return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
		case -1000023004: return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
		case -1000023005: return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
		case -1000158000: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case -1000255000: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case 1000268000: return "VK_THREAD_IDLE_KHR";
		case 1000268001: return "VK_THREAD_DONE_KHR";
		case 1000268002: return "VK_OPERATION_DEFERRED_KHR";
		case 1000268003: return "VK_OPERATION_NOT_DEFERRED_KHR";
		case -1000299000: return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
		case -1000338000: return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
		case 1000482000: return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
		case 1000483000: return "VK_PIPELINE_BINARY_MISSING_KHR";
		case -1000483000: return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
		case 0x7FFFFFFF: return "VK_RESULT_MAX_ENUM";
		default: return "Unknown VKResult";
		}
	}

#define VK_CHECK(x)                                                                           \
	do {                                                                                      \
		VkResult err = x;                                                                     \
		if (err != VK_SUCCESS)                                                                \
			RENDERX_ERROR("[Vulkan] {} at {}:{}", VkResultToString(err), __FILE__, __LINE__); \
	} while (0)

	inline bool CheckVk(VkResult result, const char* message = nullptr) {
		if (result != VK_SUCCESS) {
			if (message)
				RENDERX_ERROR("[Vulkan] {} ({})", message, VkResultToString(result));
			else
				RENDERX_ERROR("[Vulkan] {} ", VkResultToString(result));
			return false;
		}
		return true;
	}

	// Constants

	constexpr std::array<VkDynamicState, 2> g_DynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	constexpr std::array<const char*, 1> g_RequestedValidationLayers{
		"VK_LAYER_KHRONOS_validation"
	};

	constexpr std::array<const char*, 1> g_RequestedDeviceExtensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};


	enum class DescriptorBindingModel {
		Static,
		DynamicUniform,
		Bindless,
		DescriptorBuffer,
		Dynamic
	};

	// Data Structures

	struct DescriptorPoolSizes {
		uint32_t uniformBufferCount = 0;
		uint32_t storageBufferCount = 0;
		uint32_t sampledImageCount = 0;
		uint32_t storageImageCount = 0;
		uint32_t samplerCount = 0;
		uint32_t combinedImageSamplerCount = 0;
		uint32_t maxSets = 0;
	};

	struct VulkanUploadContext {
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		uint8_t* mappedPtr = nullptr;
		uint32_t size = 0;
		uint32_t offset = 0;
	};

	struct FrameContex {
		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
		VkSemaphore presentSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderSemaphore = VK_NULL_HANDLE;
		VkFence fence = VK_NULL_HANDLE;
		VulkanUploadContext upload;
		uint32_t swapchainImageIndex = 0;
	};

	struct VulkanResourceGroupLayout {
		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		ResourceGroupFlags flags = ResourceGroupFlags::NONE;
		DescriptorBindingModel model = DescriptorBindingModel::Static;
	};

	struct VulkanDescriptorBuffer {
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VkDeviceAddress address = 0;
		VkDeviceSize stride = 0;
	};

	struct VulkanResourceGroup {
		DescriptorBindingModel model = DescriptorBindingModel::Static;
		ResourceGroupFlags flags = ResourceGroupFlags::NONE;

		// Static / Dynamic
		VkDescriptorSet set = VK_NULL_HANDLE;
		uint32_t dynamicOffset = 0;

		// Bindless
		std::vector<uint32_t> bindlessIndices;

		// Descriptor buffer
		VulkanDescriptorBuffer descriptorBuffer;
	};

	struct VulkanPipelineLayout {
		VkPipelineLayout layout;
		std::vector<VkDescriptorSetLayout> setlayouts;
		std::vector<Hash64> setHashs;
		bool isBound = false;
	};

	struct VulkanPipeline {
		VkPipeline pipeline;
		PipelineLayoutHandle layout;
		bool isBound = false;
	};

	struct SwapchainImageSync {
		VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
	};

	struct VulkanBufferConfig {
		VkBufferUsageFlags usage;
		VmaMemoryUsage vmaUsage;
		VmaAllocationCreateFlags vmaFlags;
	};

	struct VulkanBuffer {
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocationInfo allocInfo = {};
		VkDeviceSize size = 0;
		uint32_t bindingCount = 1;
		BufferFlags flags;

#ifdef RENDERX_DEBUG
		const char* debugName = nullptr;
#endif
	};

	struct VulkanBufferView {
		BufferHandle buffer;
		uint32_t offset;
		uint32_t range;
		Hash64 hash;
		bool isValid = false;
	};

	struct VulkanTexture {
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkFormat format = VK_FORMAT_UNDEFINED;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t mipLevels = 1;
		bool isValid = false;
	};

	struct VulkanShader {
		std::string entryPoint;
		ShaderStage type;
		VkShaderModule shaderModule = VK_NULL_HANDLE;
	};

	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct VulkanCommandBuffer {
		VkCommandPool pool = VK_NULL_HANDLE;
		VkCommandBuffer buffer = VK_NULL_HANDLE;
		uint64_t submissionID = 0;
	};

	struct SwapchainCreateInfo {
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t imageCount = 3;
		VkFormat preferredFormat = VK_FORMAT_B8G8R8A8_UNORM;
		VkColorSpaceKHR preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	};

	// Resource Pool Template

	template <typename ResourceType, typename Tag>
	class ResourcePool {
	public:
		using ValueType = uint32_t;

		ResourcePool() {
			_My_resource.emplace_back();
			_My_generation.emplace_back(1);
			_My_key = GenerateKey();
		}

		Tag allocate(ResourceType resource) {
			ValueType index;
			Tag handle;

			if (_My_freelist.empty()) {
				index = static_cast<ValueType>(_My_resource.size());
				_My_resource.push_back(resource);
				_My_generation.push_back(1);
			}
			else {
				index = _My_freelist.back();
				_My_freelist.pop_back();
				_My_resource[index] = resource;
				++_My_generation[index];
			}

			uint64_t raw =
				(static_cast<uint64_t>(_My_generation[index]) << 32) |
				static_cast<uint64_t>(index);

			handle.id = Encrypt(raw);
			return handle;
		}

		void free(Tag& handle) {
			RENDERX_ASSERT_MSG(handle.IsValid(),
				"ResourcePool::free : Trying to free invalid handle");

			uint64_t raw = Decrypt(handle.id);
			auto index = static_cast<ValueType>(raw & 0xFFFFFFFF);
			auto gen = static_cast<ValueType>(raw >> 32);

			RENDERX_ASSERT_MSG(
				index < _My_resource.size() && _My_generation[index] == gen,
				"ResourcePool::free : Stale or foreign handle detected");

			handle.id = 0;
			_My_resource[index] = ResourceType{};
			_My_freelist.push_back(index);
		}

		ResourceType* get(const Tag& handle) {
			RENDERX_ASSERT_MSG(handle.IsValid(),
				"ResourcePool::get : invalid handle");

			uint64_t raw = Decrypt(handle.id);
			auto index = static_cast<ValueType>(raw & 0xFFFFFFFF);
			auto gen = static_cast<ValueType>(raw >> 32);

			if (!(index < _My_resource.size() && _My_generation[index] == gen)) {
				RENDERX_WARN("ResourcePool::get : Stale or foreign handle detected");
				return nullptr;
			}

			return &_My_resource[index];
		}

		template <typename Fn>
		void ForEach(Fn&& fn) {
			for (size_t i = 1; i < _My_resource.size(); ++i) {
				if (_My_generation[i] != 0) {
					fn(_My_resource[i]);
				}
			}
		}

		bool IsAlive(const Tag& handle) const {
			if (!handle.IsValid())
				return false;

			uint64_t raw = Decrypt(handle.id);
			auto index = static_cast<ValueType>(raw & 0xFFFFFFFF);
			auto gen = static_cast<ValueType>(raw >> 32);

			return index < _My_resource.size() && _My_generation[index] == gen;
		}

		void clear() {
			_My_resource.clear();
			_My_generation.clear();
			_My_freelist.clear();
			_My_resource.emplace_back();
			_My_generation.emplace_back(1);
		}
	private:
		static uint64_t RotateLeft(uint64_t x, int r) {
			return (x << r) | (x >> (64 - r));
		}

		static uint64_t RotateRight(uint64_t x, int r) {
			return (x >> r) | (x << (64 - r));
		}

		uint64_t Encrypt(uint64_t value) const {
			value ^= _My_key;
			value = RotateLeft(value, 17);
			return value;
		}

		uint64_t Decrypt(uint64_t value) const {
			value = RotateRight(value, 17);
			value ^= _My_key;
			return value;
		}

		static uint64_t GenerateKey() {
			static std::atomic<uint64_t> counter{ 0xA5B35705F00DBAAD };
			return counter.fetch_add(0x9E3779B97F4A7C15ull);
		}
	private:
		std::vector<ResourceType> _My_resource;
		std::vector<ValueType> _My_generation;
		std::vector<ValueType> _My_freelist;
		uint64_t _My_key = 0;
	};

	struct VulkanContext;
	class VulkanInstance;
	class VulkanDevice;
	class VulkanSwapchain;
	class VulkanAllocator;
	class VulkanCommandQueue;
	class VulkanDescriptorPoolManager;
	class VulkanDescriptorManager;

	// Class Declarations

	class VulkanSwapchain {
	public:
		VulkanSwapchain() = default;
		~VulkanSwapchain();

		bool create(const SwapchainCreateInfo& info);
		void destroy();
		bool acquireNextImage(VkSemaphore signalSemaphore, uint32_t* outImageIndex);
		bool present(VkQueue presentQueue, VkSemaphore waitSemaphore, uint32_t imageIndex);
		void recreate(uint32_t width, uint32_t height);

		VkFormat format() const { return m_Format; }
		VkExtent2D extent() const { return m_Extent; }
		uint32_t imageCount() const { return uint32_t(m_Images.size()); }
		VkImage image(uint32_t index) const { return m_Images[index]; }
		VkImageView imageView(uint32_t index) const { return m_ImageViews[index]; }
	private:
		void createSwapchain(uint32_t width, uint32_t height);
		void destroySwapchain();
		VkSurfaceFormatKHR chooseSurfaceFormat() const;
		VkPresentModeKHR choosePresentMode() const;
		VkExtent2D chooseExtent(uint32_t width, uint32_t height) const;
	private:
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		VkFormat m_Format = VK_FORMAT_UNDEFINED;
		VkExtent2D m_Extent{};
		uint32_t m_ImageCount = 0;
		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
		SwapchainCreateInfo m_Info{};
	};

	class VulkanInstance {
	public:
		VulkanInstance(const Window& window);
		~VulkanInstance();

		VkInstance getInstance() const { return m_Instance; }
		VkSurfaceKHR getSurface() const { return m_Surface; }
	private:
		void createInstance(const Window& window);
		void createSurface(const Window& window);
	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	};

	struct DeviceInfo {
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceMemoryProperties memoryProperties;
		std::vector<VkQueueFamilyProperties> queueFamilies;
		std::vector<VkExtensionProperties> extensions;
		uint32_t score = 0;
		int index;
	};

	class VulkanDevice {
	public:
		VulkanDevice(
			VkInstance instance,
			VkSurfaceKHR surface,
			const std::vector<const char*>& requiredExtensions = {},
			const std::vector<const char*>& requiredLayers = {});
		~VulkanDevice();

		VkPhysicalDevice physical() const { return m_PhysicalDevice; }
		VkDevice logical() const { return m_Device; }
		VkQueue graphicsQueue() const { return m_GraphicsQueue; }
		VkQueue computeQueue() const { return m_ComputeQueue; }
		VkQueue transferQueue() const { return m_TransferQueue; }
		uint32_t graphicsFamily() const { return m_GraphicsFamily; }
		uint32_t computeFamily() const { return m_ComputeFamily; }
		uint32_t transferFamily() const { return m_TransferFamily; }
		const VkPhysicalDeviceLimits& limits() const { return m_Limits; }
	private:
		DeviceInfo gatherDeviceInfo(VkPhysicalDevice device) const;
		void logDeviceInfo(uint32_t index, const DeviceInfo& info) const;
		uint32_t scoreDevice(const DeviceInfo& info) const;
		int selectDevice(const std::vector<DeviceInfo>& devices) const;
		bool isDeviceSuitable(VkPhysicalDevice device) const;
		void createLogicalDevice(
			const std::vector<const char*>& requiredExtensions,
			const std::vector<const char*>& requiredLayers);
		void queryLimits();
	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_ComputeQueue = VK_NULL_HANDLE;
		VkQueue m_TransferQueue = VK_NULL_HANDLE;
		uint32_t m_GraphicsFamily = UINT32_MAX;
		uint32_t m_ComputeFamily = UINT32_MAX;
		uint32_t m_TransferFamily = UINT32_MAX;
		VkPhysicalDeviceLimits m_Limits{};
	};

	class VulkanAllocator {
	public:
		VulkanAllocator(
			VkInstance instance,
			VkPhysicalDevice physicalDevice,
			VkDevice device);
		~VulkanAllocator();

		bool createBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VmaMemoryUsage memoryUsage,
			VmaAllocationCreateFlags flags,
			VkBuffer& outBuffer,
			VmaAllocation& outAllocation,
			VmaAllocationInfo* outInfo = nullptr);

		void destroyBuffer(VkBuffer buffer, VmaAllocation allocation);

		bool createImage(
			const VkImageCreateInfo& imageInfo,
			VmaMemoryUsage memoryUsage,
			VmaAllocationCreateFlags flags,
			VkImage& outImage,
			VmaAllocation& outAllocation,
			VmaAllocationInfo* outInfo = nullptr);

		void destroyImage(VkImage image, VmaAllocation allocation);
		void* map(VmaAllocation allocation);
		void unmap(VmaAllocation allocation);
		VmaAllocator handle() const { return m_Allocator; }
	private:
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
	};


	class VulkanDescriptorPoolManager {
	public:
		VulkanDescriptorPoolManager(VulkanContext& ctx);
		~VulkanDescriptorPoolManager();

		VkDescriptorPool acquire(bool allowFree, bool updateAfterBind, uint64_t gpuValue);
		void submit(uint64_t submissionID);
		void retire(uint64_t completedSubmission);
		void resetPersistent();
		void resetBindless();
	private:
		VkDescriptorPool createPool(uint32_t maxSets, bool allowFree, bool updateAfterBind);
	private:
		VulkanContext& m_Ctx;


		struct VulkanDescriptorPool {
			VkDescriptorPool pool = VK_NULL_HANDLE;
			uint64_t submissionID = 0; // 0 = free
		};

		std::deque<VulkanDescriptorPool> m_usedPools;
		std::vector<VkDescriptorPool> m_freePools;
		VulkanDescriptorPool m_CurrentTransient;
		VkDescriptorPool m_Persistent = VK_NULL_HANDLE;
		VkDescriptorPool m_Bindless = VK_NULL_HANDLE;
	};

	class VulkanDescriptorManager {
	public:
		VulkanDescriptorManager(VulkanContext& ctx);
		~VulkanDescriptorManager();

		VulkanResourceGroupLayout createLayout(const ResourceGroupLayout& layout);
		VulkanResourceGroup createGroup(
			const ResourceGroupLayoutHandle& layout,
			const ResourceGroupDesc& desc, uint64_t timelineValue = 0);

		void bind(
			VkCommandBuffer cmd,
			VkPipelineLayout pipelineLayout,
			uint32_t setIndex,
			const VulkanResourceGroup& group);
	private:
		VulkanContext& m_Ctx;
	};



	class VulkanCommandList : public CommandList {
	public:
		VulkanCommandList(
			VkCommandBuffer cmdBuffer,
			QueueType queueType)
			: m_CommandBuffer(cmdBuffer), m_QueueType(queueType){}
		void open() override;
		void close() override;
		void setPipeline(const PipelineHandle& pipeline) override;
		void setVertexBuffer(const BufferHandle& buffer, uint64_t offset = 0) override;
		void setIndexBuffer(const BufferHandle& buffer, uint64_t offset = 0) override;

		void draw(
			uint32_t vertexCount,
			uint32_t instanceCount = 1,
			uint32_t firstVertex = 0,
			uint32_t firstInstance = 0) override;

		void drawIndexed(
			uint32_t indexCount,
			int32_t vertexOffset = 0,
			uint32_t instanceCount = 1,
			uint32_t firstIndex = 0,
			uint32_t firstInstance = 0) override;

		void beginRenderPass(
			RenderPassHandle pass,
			const void* clearValues,
			uint32_t clearCount) override;

		void endRenderPass() override;

		void writeBuffer(
			BufferHandle handle,
			const void* data,
			uint32_t offset,
			uint32_t size) override;

		void setResourceGroup(
			const ResourceGroupHandle& handle) override;

		friend class VulkanCommandAllocator;
		friend class VulkanCommandQueue;
	private:
		const char* m_DebugName;
		VkCommandBuffer m_CommandBuffer;
		QueueType m_QueueType;
	
		// Track currently bound pipeline / layout so descriptor sets can be bound
		PipelineHandle m_CurrentPipeline;
		PipelineLayoutHandle m_CurrentPipelineLayout;
		// Currently bound vertex/index buffers (handles) and offsets
		BufferHandle m_VertexBuffer;
		uint64_t m_VertexBufferOffset = 0;
		BufferHandle m_IndexBuffer;
		uint64_t m_IndexBufferOffset = 0;
	};

	class VulkanCommandAllocator : public CommandAllocator {
	public:
		VulkanCommandAllocator(VkCommandPool pool, VkDevice device, const char* debugName = nullptr)
			: m_device(device), m_Pool(pool) {};

		~VulkanCommandAllocator() = default;
		CommandList* Allocate() override;
		void Free(CommandList* list) override;
		void Reset(CommandList* list) override;
		void Reset() override;
		friend class VulkanCommandQueue;
	private:
		const char* m_DebugName;
		QueueType m_QueueType;
		VkCommandPool m_Pool;
		VkDevice m_device;
	};

	class VulkanCommandQueue : public CommandQueue {
	public:
		VulkanCommandQueue(VkDevice device, VkQueue queue, uint32_t family, QueueType type);
		~VulkanCommandQueue();
	private:
		void addWait(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags stage);
		VkQueue Queue();
		VkSemaphore Semaphore() { return m_TimelineSemaphore; }

		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_Queue = VK_NULL_HANDLE;
		uint32_t m_Family = UINT32_MAX;
		QueueType m_Type = QueueType::GRAPHICS;

		VkSemaphore m_TimelineSemaphore = VK_NULL_HANDLE;
		uint64_t m_Submitted = 0;
		uint64_t m_Completed = 0;

		VkSemaphore m_WaitSemaphores[3];
		uint64_t m_WaitValues[3];
		VkPipelineStageFlags m_WaitStages[3];
		uint32_t m_WaitCount = 0;
	public:
		CommandAllocator* CreateCommandAllocator(const char* debugName = nullptr) override;
		void DestroyCommandAllocator(CommandAllocator* allocator) override;
		Timeline Submit(CommandList* commandList) override;
		Timeline Submit(const SubmitInfo& submitInfo) override;
		bool Wait(Timeline value, uint64_t timeout = UINT64_MAX) override;
		void WaitIdle() override;
		bool Poll(Timeline value) override;
		Timeline Completed() override;
		Timeline Submitted() const override;
		float TimestampFrequency() const override;
	};

	struct VulkanContext {
		void* window = nullptr;
		std::unique_ptr<VulkanInstance> instance;
		std::unique_ptr<VulkanDevice> device;
		std::unique_ptr<VulkanSwapchain> swapchain;
		std::unique_ptr<VulkanCommandQueue> graphicsQueue;
		std::unique_ptr<VulkanCommandQueue> computeQueue;
		std::unique_ptr<VulkanCommandQueue> transferQueue;
		std::unique_ptr<VulkanAllocator> allocator;
		std::unique_ptr<VulkanDescriptorManager> descriptorSetManager;
		std::unique_ptr<VulkanDescriptorPoolManager> descriptorPoolManager;
	};

	// Global Resource Pools
	extern ResourcePool<VulkanBuffer, BufferHandle> g_BufferPool;
	extern ResourcePool<VulkanBufferView, BufferViewHandle> g_BufferViewPool;
	extern ResourcePool<VkRenderPass, RenderPassHandle> g_RenderPassPool;
	extern ResourcePool<VulkanTexture, TextureHandle> g_TexturePool;
	extern ResourcePool<VulkanShader, ShaderHandle> g_ShaderPool;
	extern ResourcePool<VulkanPipeline, PipelineHandle> g_PipelinePool;
	extern ResourcePool<VulkanPipelineLayout, PipelineLayoutHandle> g_PipelineLayoutPool;
	extern ResourcePool<VulkanResourceGroupLayout, ResourceGroupLayoutHandle> g_ResourceGroupLayoutPool;

	// ResourceGroup pool is defined in VK_ResourceGroups.cpp; expose extern so
	// other modules (e.g. command list) can bind descriptor sets.
	extern ResourcePool<VulkanResourceGroup, ResourceGroupHandle> g_ResourceGroupPool;

	extern std::unordered_map<Hash64, BufferViewHandle> g_BufferViewCache;
	extern std::unordered_map<Hash64, ResourceGroupHandle> g_ResourceGroupCache;

	// Context & Cleanup Functions


	VulkanContext& GetVulkanContext();
	void freeAllVulkanResources();
	void VKShutdownCommon();

	// Descriptor Pool Creation

	inline VkDescriptorPool CreateDescriptorPool(const DescriptorPoolSizes& sizes, bool allowFree) {
		std::vector<VkDescriptorPoolSize> poolSizes;
		auto& ctx = GetVulkanContext();

		if (sizes.uniformBufferCount > 0)
			poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizes.uniformBufferCount });
		if (sizes.storageBufferCount > 0)
			poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sizes.storageBufferCount });
		if (sizes.sampledImageCount > 0)
			poolSizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, sizes.sampledImageCount });
		if (sizes.storageImageCount > 0)
			poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, sizes.storageImageCount });
		if (sizes.samplerCount > 0)
			poolSizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER, sizes.samplerCount });
		if (sizes.combinedImageSamplerCount > 0)
			poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, sizes.combinedImageSamplerCount });

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = allowFree ? VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT : 0;
		poolInfo.maxSets = sizes.maxSets;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		VkDescriptorPool pool;
		VkResult result = vkCreateDescriptorPool(ctx.device->logical(), &poolInfo, nullptr, &pool);

		return (result == VK_SUCCESS) ? pool : VK_NULL_HANDLE;
	}


	// VMA Memory Allocation Conversion
	/// Convert RenderX MemoryType to VMA allocation info
	inline VmaAllocationCreateInfo ToVmaAllocationCreateInfo(MemoryType type, BufferFlags flags) {
		VmaAllocationCreateInfo allocInfo = {};
		bool isDynamic = Has(flags, BufferFlags::DYNAMIC);
		bool isStreaming = Has(flags, BufferFlags::STREAMING);

		switch (type) {
		case MemoryType::GPU_ONLY:
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			allocInfo.flags = 0;
			break;

		case MemoryType::CPU_TO_GPU:
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

			if (isDynamic || isStreaming) {
				allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}

			allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
			break;

		case MemoryType::GPU_TO_CPU:
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
							  VMA_ALLOCATION_CREATE_MAPPED_BIT;

			allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			break;

		case MemoryType::CPU_ONLY:
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
							  VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;

		case MemoryType::AUTO:
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;

			if (isDynamic || isStreaming) {
				allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
								   VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}
			break;

		default:
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = 0;
			break;
		}

		return allocInfo;
	}

	/// Legacy conversion to VkMemoryPropertyFlags
	/// VMA 3.0+ prefers VmaAllocationCreateInfo approach
	inline VkMemoryPropertyFlags ToVulkanMemoryPropertyFlags(MemoryType type) {
		switch (type) {
		case MemoryType::GPU_ONLY:
			return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		case MemoryType::CPU_TO_GPU:
			return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		case MemoryType::GPU_TO_CPU:
			return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
				   VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

		case MemoryType::CPU_ONLY:
			return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		case MemoryType::AUTO:
			return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		default:
			return 0;
		}
	}

	/// Extended allocation info with priority and dedicated allocation support
	inline VmaAllocationCreateInfo ToVmaAllocationCreateInfoEx(
		MemoryType type,
		BufferFlags flags,
		float priority = 0.5f,
		bool dedicatedAllocation = false) {
		VmaAllocationCreateInfo allocInfo = ToVmaAllocationCreateInfo(type, flags);
		allocInfo.priority = priority;
		if (dedicatedAllocation) {
			allocInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		}

		return allocInfo;
	}

	/// Get preferred flags vs required flags
	inline void GetMemoryRequirements(
		MemoryType type,
		VkMemoryPropertyFlags& preferredFlags,
		VkMemoryPropertyFlags& requiredFlags) {
		requiredFlags = 0;
		preferredFlags = 0;

		switch (type) {
		case MemoryType::GPU_ONLY:
			preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			requiredFlags = 0; // Can fallback if no device-local available
			break;

		case MemoryType::CPU_TO_GPU:
			requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
							VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			preferredFlags = requiredFlags | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			break;

		case MemoryType::GPU_TO_CPU:
			requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
							VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
							VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			preferredFlags = requiredFlags;
			break;

		case MemoryType::CPU_ONLY:
			requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
							VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			preferredFlags = requiredFlags;
			break;

		case MemoryType::AUTO:
			preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			requiredFlags = 0;
			break;
		}
	}

	// Image/Texture Memory Allocation
	/// VMA allocation info specifically for images/textures
	inline VmaAllocationCreateInfo ToVmaAllocationCreateInfoForImage(
		MemoryType type,
		bool isRenderTarget = false,
		bool isDynamic = false) {
		VmaAllocationCreateInfo allocInfo = {};

		switch (type) {
		case MemoryType::GPU_ONLY:
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			allocInfo.flags = 0;

			// Render targets should prefer dedicated allocations
			if (isRenderTarget) {
				allocInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
			}
			break;

		case MemoryType::CPU_TO_GPU:
			// For dynamic textures (e.g., streaming video)
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

			if (isDynamic) {
				allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}
			break;

		case MemoryType::GPU_TO_CPU:
			// For readback
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
							  VMA_ALLOCATION_CREATE_MAPPED_BIT;
			allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			break;

		case MemoryType::AUTO:
		default:
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			allocInfo.flags = 0;
			break;
		}

		return allocInfo;
	}


	// Utility Functions
	/// Check if a memory type requires host-visible memory
	inline bool IsHostVisible(MemoryType type) {
		return type == MemoryType::CPU_TO_GPU ||
			   type == MemoryType::GPU_TO_CPU ||
			   type == MemoryType::CPU_ONLY;
	}

	/// Check if a memory type should be persistently mapped
	inline bool ShouldBePersistentlyMapped(MemoryType type, BufferFlags flags) {
		bool isDynamic = Has(flags, BufferFlags::DYNAMIC);
		bool isStreaming = Has(flags, BufferFlags::STREAMING);

		return IsHostVisible(type) && (isDynamic || isStreaming);
	}

	/// Get human-readable name for memory type (useful for debugging)
	inline const char* MemoryTypeToString(MemoryType type) {
		switch (type) {
		case MemoryType::GPU_ONLY: return "GPU_ONLY";
		case MemoryType::CPU_TO_GPU: return "CPU_TO_GPU";
		case MemoryType::GPU_TO_CPU: return "GPU_TO_CPU";
		case MemoryType::CPU_ONLY: return "CPU_ONLY";
		case MemoryType::AUTO: return "AUTO";
		default: return "UNKNOWN";
		}
	}

	/// Get recommended memory type based on usage pattern
	inline MemoryType GetRecommendedMemoryType(BufferFlags flags) {
		if (Has(flags, BufferFlags::DYNAMIC) || Has(flags, BufferFlags::STREAMING)) {
			return MemoryType::CPU_TO_GPU;
		}
		else if (Has(flags, BufferFlags::STATIC)) {
			return MemoryType::GPU_ONLY;
		}

		// Default to AUTO and let VMA decide
		return MemoryType::AUTO;
	}

	// Format Conversion
	inline VkFormat ToVulkanFormat(Format format) {
		switch (format) {
		case Format::UNDEFINED: return VK_FORMAT_UNDEFINED;
		case Format::R8_UNORM: return VK_FORMAT_R8_UNORM;
		case Format::RG8_UNORM: return VK_FORMAT_R8G8_UNORM;
		case Format::RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
		case Format::RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
		case Format::BGRA8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
		case Format::BGRA8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
		case Format::R16_SFLOAT: return VK_FORMAT_R16_SFLOAT;
		case Format::RG16_SFLOAT: return VK_FORMAT_R16G16_SFLOAT;
		case Format::RGBA16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Format::R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
		case Format::RG32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
		case Format::RGB32_SFLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
		case Format::RGBA32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Format::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
		case Format::D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
		case Format::BC1_RGBA_UNORM: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case Format::BC1_RGBA_SRGB: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case Format::BC3_UNORM: return VK_FORMAT_BC3_UNORM_BLOCK;
		case Format::BC3_SRGB: return VK_FORMAT_BC3_SRGB_BLOCK;
		default: return VK_FORMAT_UNDEFINED;
		}
	}

	// Texture/Image Type Conversion
	inline VkImageType ToVulkanImageType(TextureType type) {
		switch (type) {
		case TextureType::TEXTURE_2D:
		case TextureType::TEXTURE_2D_ARRAY:
		case TextureType::TEXTURE_CUBE:
			return VK_IMAGE_TYPE_2D;
		case TextureType::TEXTURE_3D:
			return VK_IMAGE_TYPE_3D;
		default:
			return VK_IMAGE_TYPE_2D;
		}
	}

	inline VkImageViewType ToVulkanImageViewType(TextureType type) {
		switch (type) {
		case TextureType::TEXTURE_2D: return VK_IMAGE_VIEW_TYPE_2D;
		case TextureType::TEXTURE_3D: return VK_IMAGE_VIEW_TYPE_3D;
		case TextureType::TEXTURE_CUBE: return VK_IMAGE_VIEW_TYPE_CUBE;
		case TextureType::TEXTURE_2D_ARRAY: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		default: return VK_IMAGE_VIEW_TYPE_2D;
		}
	}


	// Filter Conversion
	inline VkFilter ToVulkanFilter(Filter filter) {
		switch (filter) {
		case Filter::NEAREST:
		case Filter::NEAREST_MIPMAP_NEAREST:
		case Filter::NEAREST_MIPMAP_LINEAR:
			return VK_FILTER_NEAREST;
		case Filter::LINEAR:
		case Filter::LINEAR_MIPMAP_NEAREST:
		case Filter::LINEAR_MIPMAP_LINEAR:
			return VK_FILTER_LINEAR;
		default:
			return VK_FILTER_LINEAR;
		}
	}

	inline VkSamplerMipmapMode ToVulkanMipmapMode(Filter filter) {
		switch (filter) {
		case Filter::NEAREST_MIPMAP_NEAREST:
		case Filter::LINEAR_MIPMAP_NEAREST:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case Filter::NEAREST_MIPMAP_LINEAR:
		case Filter::LINEAR_MIPMAP_LINEAR:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		default:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		}
	}


	// Texture Wrap/Address Mode Conversion
	inline VkSamplerAddressMode ToVulkanAddressMode(TextureWrap wrap) {
		switch (wrap) {
		case TextureWrap::REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case TextureWrap::MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case TextureWrap::CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case TextureWrap::CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

	// Topology/Primitive Type Conversion
	inline VkPrimitiveTopology ToVulkanTopology(Topology topology) {
		switch (topology) {
		case Topology::POINTS: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case Topology::LINES: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case Topology::LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case Topology::TRIANGLES: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case Topology::TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case Topology::TRIANGLE_FAN: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}


	// Compare Function Conversion
	inline VkCompareOp ToVulkanCompareOp(CompareFunc func) {
		switch (func) {
		case CompareFunc::NEVER: return VK_COMPARE_OP_NEVER;
		case CompareFunc::LESS: return VK_COMPARE_OP_LESS;
		case CompareFunc::EQUAL: return VK_COMPARE_OP_EQUAL;
		case CompareFunc::LESS_EQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CompareFunc::GREATER: return VK_COMPARE_OP_GREATER;
		case CompareFunc::NOT_EQUAL: return VK_COMPARE_OP_NOT_EQUAL;
		case CompareFunc::GREATER_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case CompareFunc::ALWAYS: return VK_COMPARE_OP_ALWAYS;
		default: return VK_COMPARE_OP_LESS;
		}
	}


	// Blend Function Conversion
	inline VkBlendFactor ToVulkanBlendFactor(BlendFunc func) {
		switch (func) {
		case BlendFunc::ZERO: return VK_BLEND_FACTOR_ZERO;
		case BlendFunc::ONE: return VK_BLEND_FACTOR_ONE;
		case BlendFunc::SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendFunc::ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendFunc::DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
		case BlendFunc::ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendFunc::SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendFunc::ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BlendFunc::DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
		case BlendFunc::ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case BlendFunc::CONSTANT_COLOR: return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case BlendFunc::ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		default: return VK_BLEND_FACTOR_ONE;
		}
	}


	// Blend Operation Conversion
	inline VkBlendOp ToVulkanBlendOp(BlendOp op) {
		switch (op) {
		case BlendOp::ADD: return VK_BLEND_OP_ADD;
		case BlendOp::SUBTRACT: return VK_BLEND_OP_SUBTRACT;
		case BlendOp::REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
		case BlendOp::MIN: return VK_BLEND_OP_MIN;
		case BlendOp::MAX: return VK_BLEND_OP_MAX;
		default: return VK_BLEND_OP_ADD;
		}
	}


	// Cull Mode Conversion
	inline VkCullModeFlags ToVulkanCullMode(CullMode mode) {
		switch (mode) {
		case CullMode::NONE: return VK_CULL_MODE_NONE;
		case CullMode::FRONT: return VK_CULL_MODE_FRONT_BIT;
		case CullMode::BACK: return VK_CULL_MODE_BACK_BIT;
		case CullMode::FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
		default: return VK_CULL_MODE_NONE;
		}
	}


	// Fill Mode/Polygon Mode Conversion
	inline VkPolygonMode ToVulkanPolygonMode(FillMode mode) {
		switch (mode) {
		case FillMode::SOLID: return VK_POLYGON_MODE_FILL;
		case FillMode::WIREFRAME: return VK_POLYGON_MODE_LINE;
		case FillMode::POINT: return VK_POLYGON_MODE_POINT;
		default: return VK_POLYGON_MODE_FILL;
		}
	}


	// Load Operation Conversion
	inline VkAttachmentLoadOp ToVulkanLoadOp(LoadOp op) {
		switch (op) {
		case LoadOp::LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
		case LoadOp::CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case LoadOp::DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		default: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		}
	}


	// Store Operation Conversion

	inline VkAttachmentStoreOp ToVulkanStoreOp(StoreOp op) {
		switch (op) {
		case StoreOp::STORE: return VK_ATTACHMENT_STORE_OP_STORE;
		case StoreOp::DONT_CARE: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		default: return VK_ATTACHMENT_STORE_OP_STORE;
		}
	}


	// Shader Stage Conversion
	inline VkShaderStageFlagBits ToVulkanShaderStage(ShaderStage stage) {
		switch (stage) {
		case ShaderStage::VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::GEOMETRY: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderStage::TESS_CONTROL: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case ShaderStage::TESS_EVALUATION: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case ShaderStage::COMPUTE: return VK_SHADER_STAGE_COMPUTE_BIT;
		default: return VK_SHADER_STAGE_VERTEX_BIT;
		}
	}

	inline VkShaderStageFlags ToVulkanShaderStageFlags(ShaderStage stages) {
		VkShaderStageFlags flags = 0;

		if (Has(stages, ShaderStage::VERTEX))
			flags |= VK_SHADER_STAGE_VERTEX_BIT;
		if (Has(stages, ShaderStage::FRAGMENT))
			flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		if (Has(stages, ShaderStage::GEOMETRY))
			flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
		if (Has(stages, ShaderStage::TESS_CONTROL))
			flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		if (Has(stages, ShaderStage::TESS_EVALUATION))
			flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		if (Has(stages, ShaderStage::COMPUTE))
			flags |= VK_SHADER_STAGE_COMPUTE_BIT;

		return flags;
	}


	// Descriptor/Resource Type Conversion
	inline VkDescriptorType ToVulkanDescriptorType(ResourceType type) {
		switch (type) {
		case ResourceType::CONSTANT_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case ResourceType::STORAGE_BUFFER:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case ResourceType::RW_STORAGE_BUFFER:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case ResourceType::TEXTURE_SRV:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case ResourceType::TEXTURE_UAV:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case ResourceType::SAMPLER:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case ResourceType::COMBINED_TEXTURE_SAMPLER:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case ResourceType::ACCELERATION_STRUCTURE:
			return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		default:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		}
	}


	// Buffer Usage Flags Conversion
	inline VkBufferUsageFlags ToVulkanBufferUsage(BufferFlags flags) {
		VkBufferUsageFlags usage = 0;

		if (Has(flags, BufferFlags::VERTEX))
			usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if (Has(flags, BufferFlags::INDEX))
			usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		if (Has(flags, BufferFlags::UNIFORM))
			usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		if (Has(flags, BufferFlags::STORAGE))
			usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if (Has(flags, BufferFlags::INDIRECT))
			usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		if (Has(flags, BufferFlags::TRANSFER_SRC))
			usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		if (Has(flags, BufferFlags::TRANSFER_DST))
			usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		return usage;
	}


	// Memory Type/VMA Allocation Conversion
	inline VmaMemoryUsage ToVmaMemoryUsage(MemoryType type) {
		switch (type) {
		case MemoryType::GPU_ONLY:
			return VMA_MEMORY_USAGE_GPU_ONLY;
		case MemoryType::CPU_TO_GPU:
			return VMA_MEMORY_USAGE_CPU_TO_GPU;
		case MemoryType::GPU_TO_CPU:
			return VMA_MEMORY_USAGE_GPU_TO_CPU;
		case MemoryType::CPU_ONLY:
			return VMA_MEMORY_USAGE_CPU_ONLY;
		case MemoryType::AUTO:
			return VMA_MEMORY_USAGE_AUTO;
		default:
			return VMA_MEMORY_USAGE_AUTO;
		}
	}

	inline VmaAllocationCreateFlags ToVmaAllocationFlags(MemoryType type, BufferFlags bufferFlags) {
		VmaAllocationCreateFlags flags = 0;

		// Memory type specific flags
		switch (type) {
		case MemoryType::CPU_TO_GPU:
			flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;
		case MemoryType::GPU_TO_CPU:
			flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
			flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;
		case MemoryType::CPU_ONLY:
			flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;
		case MemoryType::GPU_ONLY:

			break;
		case MemoryType::AUTO:
			flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
			break;
		}

		// Buffer-specific flags
		if (Has(bufferFlags, BufferFlags::DYNAMIC)) {
			flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}

		return flags;
	}


	// Image Usage Flags Conversion (Helper)
	inline VkImageUsageFlags GetDefaultImageUsageFlags(TextureType type, Format format) {
		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
								  VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
								  VK_IMAGE_USAGE_SAMPLED_BIT;

		// Add depth/stencil usage for depth formats
		if (format == Format::D24_UNORM_S8_UINT || format == Format::D32_SFLOAT) {
			usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		else {
			usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		return usage;
	}

	// Vertex Input Rate Conversion
	inline VkVertexInputRate ToVulkanInputRate(bool perInstance) {
		return perInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
	}


	// Image Layout Helpers
	inline VkImageLayout GetDefaultImageLayout(Format format) {
		if (format == Format::D24_UNORM_S8_UINT || format == Format::D32_SFLOAT) {
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	inline VkImageAspectFlags GetImageAspect(Format format) {
		if (format == Format::D32_SFLOAT) {
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (format == Format::D24_UNORM_S8_UINT) {
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}


	// Pipeline Stage Flags (for synchronization)

	inline VkPipelineStageFlags ShaderStageToPipelineStage(ShaderStage stages) {
		VkPipelineStageFlags flags = 0;

		if (Has(stages, ShaderStage::VERTEX))
			flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		if (Has(stages, ShaderStage::FRAGMENT))
			flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		if (Has(stages, ShaderStage::GEOMETRY))
			flags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
		if (Has(stages, ShaderStage::TESS_CONTROL))
			flags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
		if (Has(stages, ShaderStage::TESS_EVALUATION))
			flags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
		if (Has(stages, ShaderStage::COMPUTE))
			flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

		return flags;
	}

} // namespace VulkanConversion
} // namespace Rx