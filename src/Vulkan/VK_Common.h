#pragma once
#include "ProLog/ProLog.h"
#include "RenderX/RX_Common.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <deque>
#include <limits>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Rx {
namespace RxVK {

// Type Aliases & Global Variables

using Hash64 = uint64_t;

// Utility Functions

inline const char* VkResultToString(VkResult result) {
    switch (result) {
    case 0:
        return "VK_SUCCESS";
    case 1:
        return "VK_NOT_READY";
    case 2:
        return "VK_TIMEOUT";
    case 3:
        return "VK_EVENT_SET";
    case 4:
        return "VK_EVENT_RESET";
    case 5:
        return "VK_INCOMPLETE";
    case -1:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case -2:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case -3:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case -4:
        return "VK_ERROR_DEVICE_LOST";
    case -5:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case -6:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case -7:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case -8:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case -9:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case -10:
        return "VK_ERROR_TOO_MANY_OBJECTS";
    case -11:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case -12:
        return "VK_ERROR_FRAGMENTED_POOL";
    case -13:
        return "VK_ERROR_UNKNOWN";
    case -1000069000:
        return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case -1000072003:
        return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case -1000161000:
        return "VK_ERROR_FRAGMENTATION";
    case -1000257000:
        return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case 1000297000:
        return "VK_PIPELINE_COMPILE_REQUIRED";
    case -1000174001:
        return "VK_ERROR_NOT_PERMITTED";
    case -1000000000:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case -1000000001:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case 1000001003:
        return "VK_SUBOPTIMAL_KHR";
    case -1000001004:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    case -1000003001:
        return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case -1000011001:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
    case -1000012000:
        return "VK_ERROR_INVALID_SHADER_NV";
    case -1000023000:
        return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
    case -1000023001:
        return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
    case -1000023002:
        return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
    case -1000023003:
        return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
    case -1000023004:
        return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
    case -1000023005:
        return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
    case -1000158000:
        return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case -1000255000:
        return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    case 1000268000:
        return "VK_THREAD_IDLE_KHR";
    case 1000268001:
        return "VK_THREAD_DONE_KHR";
    case 1000268002:
        return "VK_OPERATION_DEFERRED_KHR";
    case 1000268003:
        return "VK_OPERATION_NOT_DEFERRED_KHR";
    case -1000299000:
        return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
    case -1000338000:
        return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
    case 1000482000:
        return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
    case 1000483000:
        return "VK_PIPELINE_BINARY_MISSING_KHR";
    case -1000483000:
        return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
    case 0x7FFFFFFF:
        return "VK_RESULT_MAX_ENUM";
    default:
        return "Unknown VKResult";
    }
}

#define VK_CHECK(x)                                                                                                    \
    do {                                                                                                               \
        VkResult err = x;                                                                                              \
        if (err != VK_SUCCESS)                                                                                         \
            RENDERX_ERROR("[Vulkan] {} at {}:{}", VkResultToString(err), __FILE__, __LINE__);                          \
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

#define ALIGNUP(size, alignment) (((((size) + (alignment)) - 1) / (alignment)) * (alignment)) // Constants

constexpr std::array<VkDynamicState, 2> g_DynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
constexpr std::array<const char*, 1>    g_RequestedValidationLayers{"VK_LAYER_KHRONOS_validation"};
constexpr std::array<const char*, 1>    g_RequestedDeviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

enum class DescriptorBindingModel {
    Static,
    DynamicUniform,
    Bindless,
    DescriptorBuffer,
    Dynamic
};

// Data Structures

struct DescriptorPoolSizes {
    uint32_t uniformBufferCount        = 0;
    uint32_t storageBufferCount        = 0;
    uint32_t sampledImageCount         = 0;
    uint32_t storageImageCount         = 0;
    uint32_t samplerCount              = 0;
    uint32_t combinedImageSamplerCount = 0;
    uint32_t maxSets                   = 0;
    DescriptorPoolSizes(uint32_t max) {
        uniformBufferCount        = max;
        storageBufferCount        = max;
        sampledImageCount         = max;
        storageImageCount         = max;
        samplerCount              = max;
        combinedImageSamplerCount = max;
        maxSets                   = max;
    }
};

struct VulkanUploadContext {
    VkBuffer      buffer     = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    uint8_t*      mappedPtr  = nullptr;
    uint32_t      size       = 0;
    uint32_t      offset     = 0;
};

struct VulkanDescriptorBuffer {
    VkBuffer        buffer     = VK_NULL_HANDLE;
    VmaAllocation   allocation = VK_NULL_HANDLE;
    VkDeviceAddress address    = 0;
    VkDeviceSize    stride     = 0;
};

struct VulkanPipelineLayout {

    struct StoredPushRange {
        uint32_t           offset;
        uint32_t           size;
        VkShaderStageFlags stageFlags;
    };

    static constexpr uint32_t          MAX_PUSH_RANGES = 8;
    VkPipelineLayout                   vkLayout;
    std::vector<VkDescriptorSetLayout> setlayouts;
    std::vector<Hash64>                setHashs;
    StoredPushRange                    pushRanges[MAX_PUSH_RANGES] = {};
    uint32_t                           pushRangeCount;
};

struct VulkanPipeline {
    VkPipeline           vkPipeline;
    PipelineLayoutHandle layout;
};

struct VulkanBufferConfig {
    VkBufferUsageFlags       usage;
    VmaMemoryUsage           vmaUsage;
    VmaAllocationCreateFlags vmaFlags;
};

struct VulkanAccessState {
    VkPipelineStageFlags2 stageMask   = 0;
    VkAccessFlags2        accessMask  = 0;
    VkImageLayout         layout      = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t              queueFamily = VK_QUEUE_FAMILY_IGNORED;

    bool operator==(const VulkanAccessState& o) const {
        return stageMask == o.stageMask && accessMask == o.accessMask && layout == o.layout &&
               queueFamily == o.queueFamily;
    }

    bool operator!=(const VulkanAccessState& o) const { return !(*this == o); }
};

struct VulkanSubresourceState {
    VulkanAccessState depth;
    VulkanAccessState stencil;
    VulkanAccessState color; // for non-DS formats
};

struct SparseTextureState {
    VulkanSubresourceState                               global;
    std::unordered_map<uint32_t, VulkanSubresourceState> overrides;
};

struct VulkanBuffer {
    VkBuffer          buffer       = VK_NULL_HANDLE;
    VmaAllocation     allocation   = VK_NULL_HANDLE;
    VmaAllocationInfo allocInfo    = {};
    VkDeviceSize      size         = 0;
    uint32_t          bindingCount = 1;
    BufferFlags       flags;
    const char*       debugName = nullptr;
    VulkanAccessState state;
};

struct VulkanBufferView {
    BufferHandle buffer;
    uint32_t     offset;
    uint32_t     range;
    Hash64       hash;
    bool         isValid   = false;
    const char*  debugName = nullptr;
};

struct VulkanTexture {
    VkImage            image            = VK_NULL_HANDLE;
    VmaAllocation      allocation       = nullptr;
    VkFormat           format           = VK_FORMAT_UNDEFINED;
    uint32_t           width            = 0;
    uint32_t           height           = 0;
    uint32_t           mipLevels        = 1;
    uint32_t           arrayLayers      = 1;
    bool               isSwapchainImage = false;
    const char*        debugName        = nullptr;
    SparseTextureState state;
};

struct VulkanTextureView {
    TextureHandle   texture;
    VkImageView     view            = VK_NULL_HANDLE;
    VkFormat        format          = VK_FORMAT_UNDEFINED;
    VkImageViewType viewType        = VK_IMAGE_VIEW_TYPE_2D;
    uint32_t        baseMipLevel    = 0;
    uint32_t        mipLevelCount   = 1;
    uint32_t        baseArrayLayer  = 0;
    uint32_t        arrayLayerCount = 1;
    const char*     debugName       = nullptr;
};

struct VulkanSampler {
    VkSampler vkSampler;
};
struct VulkanShader {
    std::string    entryPoint;
    PipelineStage  type;
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    const char*    debugName    = nullptr;
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
    const char*                     debugName = nullptr;
};

struct VulkanCommandBuffer {
    VkCommandPool   pool         = VK_NULL_HANDLE;
    VkCommandBuffer buffer       = VK_NULL_HANDLE;
    uint64_t        submissionID = 0;
    const char*     debugName    = nullptr;
};

struct SwapchainCreateInfo {
    uint32_t         width                = 0;
    uint32_t         height               = 0;
    uint32_t         imageCount           = 3;
    uint32_t         maxFramesInFlight    = 3;
    VkFormat         preferredFormat      = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR  preferredColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
};

struct VulkanRenderPass {
    VkRenderPass renderPass = VK_NULL_HANDLE;
    const char*  debugName  = nullptr;
};

struct VulkanFramebuffer {
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    const char*   debugName   = nullptr;
};

// Stored per SetLayoutHandle
struct VulkanSetLayout {
    VkDescriptorSetLayout vkLayout = VK_NULL_HANDLE;

    // Cached binding info — needed when writing descriptors
    // and when computing pool sizes at arena creation
    struct BindingInfo {
        uint32_t           slot;
        VkDescriptorType   vkType;
        uint32_t           count;
        VkShaderStageFlags stages;
        bool               updateAfterBind;
        bool               partiallyBound;
    };

    static constexpr uint32_t MAX_BINDINGS = 32;
    BindingInfo               bindings[MAX_BINDINGS];
    uint32_t                  bindingCount       = 0;
    bool                      hasUpdateAfterBind = false;

    // Total descriptor count across all bindings
    // Used when computing how many slots an allocation takes
    uint32_t totalDescriptorCount = 0;

    const char* debugName = nullptr;
};

// Stored per DescriptorPoolHandle
// One pool can back many sets (LINEAR) or individual sets (POOL)
struct VulkanDescriptorPool {

    DescriptorPoolFlags   flags;
    VkDescriptorPool      vkPool = VK_NULL_HANDLE;
    DescriptorHeapHandle  heapHandle;
    uint64_t              heapBaseOffset = 0; // byte offset into the heap where this pool starts
    uint64_t              writePtr       = 0; // current allocation position in bytes (LINEAR)
    std::vector<uint32_t> freeSlots;
    SetLayoutHandle       layout;
    uint64_t              stridePerSet    = 0;
    uint32_t              capacity        = 0; // max sets
    bool                  updateAfterBind = false;
    const char*           debugName       = nullptr;
};

// Stored per SetHandle
struct VulkanSet {
    DescriptorPoolFlags poolFlags; // which path this set came from

    //---- DESCRIPTOR_SETS path -------------------------------------------
    VkDescriptorSet vkSet = VK_NULL_HANDLE;

    //---- DESCRIPTOR_BUFFER path -----------------------------------------
    // The set is just a location in a heap buffer.
    // No VkDescriptorSet exists.
    DescriptorHeapHandle heapHandle;
    uint64_t             byteOffset = 0; // offset into the heap's VkBuffer

    // Back-reference to the pool this was allocated from
    // Needed for FreeSet on POOL policy
    DescriptorPoolHandle poolHandle;

    // Layout — needed at bind time to know descriptor count
    SetLayoutHandle layoutHandle;

    const char* debugName = nullptr;
};

// Stored per DescriptorHeapHandle
// Only used for the DESCRIPTOR_BUFFER path.
// On the DESCRIPTOR_SETS path the user never creates a heap —
// the backend creates VkDescriptorPools internally inside VulkanDescriptorPool.
struct VulkanDescriptorHeap {
    // The actual GPU memory — a VkBuffer mapped persistently
    VkBuffer      buffer     = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    // Persistently mapped CPU pointer — valid for the lifetime of the heap
    // Null if MemoryType::GPU_ONLY (staging required)
    uint8_t* mappedPtr = nullptr;

    // GPU virtual address — used in vkCmdBindDescriptorBuffersEXT
    VkDeviceAddress gpuAddress = 0;

    uint32_t capacity       = 0; // number of descriptors
    uint32_t descriptorSize = 0; // hardware size per descriptor for this heap type

    DescriptorHeapType type;
    bool               shaderVisible = true;

    const char* debugName = nullptr;
};

// Convert ResourceType → VkDescriptorType
// Already exists as ToVulkanDescriptorType in RX_VK_Common.h — reuse that.

// Compute descriptor type counts from a layout
// Used when creating a VkDescriptorPool to size it correctly
struct VkDescriptorPoolSizeList {
    VkDescriptorPoolSize sizes[16];
    uint32_t             count = 0;
};

inline VkDescriptorPoolSizeList ComputePoolSizes(const VulkanSetLayout& layout, uint32_t maxSets) {
    VkDescriptorPoolSizeList result;

    // Accumulate counts per descriptor type
    // Use a small fixed array indexed by VkDescriptorType
    // Max VkDescriptorType value we care about is ~13
    uint32_t counts[16] = {};

    for (uint32_t i = 0; i < layout.bindingCount; i++) {
        const auto& b       = layout.bindings[i];
        uint32_t    typeIdx = static_cast<uint32_t>(b.vkType);
        if (typeIdx < 16) {
            counts[typeIdx] += b.count * maxSets;
        }
    }

    for (uint32_t i = 0; i < 16; i++) {
        if (counts[i] > 0) {
            result.sizes[result.count++] = {static_cast<VkDescriptorType>(i), counts[i]};
        }
    }

    return result;
}

// Write a single descriptor directly into CPU-mapped heap memory
// Used for the DESCRIPTOR_BUFFER path
void WriteRawDescriptorToMemory(uint8_t* destCpuPtr, VkDevice device, ResourceType type, uint64_t resourceHandle);

// Write a descriptor into a VkDescriptorSet
// Used for the DESCRIPTOR_SETS path
void WriteVkDescriptor(VkDevice        device,
                       VkDescriptorSet dstSet,
                       uint32_t        dstBinding,
                       uint32_t        dstArrayElement,
                       uint32_t        count,
                       ResourceType    type,
                       uint64_t        resourceHandle);

// Resource Pool Template

template <typename ResourceType, typename Tag> class ResourcePool {
public:
    using ValueType = uint32_t;

    ResourcePool() {
        _My_resource.emplace_back();
        _My_generation.emplace_back(1);
        _My_key = GenerateKey();
    }

    Tag allocate(ResourceType resource) {
        ValueType index;
        Tag       handle;

        if (_My_freelist.empty()) {
            index = static_cast<ValueType>(_My_resource.size());
            _My_resource.push_back(resource);
            _My_generation.push_back(1);
        } else {
            index = _My_freelist.back();
            _My_freelist.pop_back();
            _My_resource[index] = resource;
            ++_My_generation[index];
        }

        uint64_t raw = (static_cast<uint64_t>(_My_generation[index]) << 32) | static_cast<uint64_t>(index);

        handle.id = Encrypt(raw);
        return handle;
    }

    void free(Tag& handle) {
        RENDERX_ASSERT_MSG(handle.isValid(), "ResourcePool::free: trying to free an invalid handle");

        uint64_t raw   = Decrypt(handle.id);
        auto     index = static_cast<ValueType>(raw & 0xFFFFFFFF);
        auto     gen   = static_cast<ValueType>(raw >> 32);

        RENDERX_ASSERT_MSG(index < _My_resource.size() && _My_generation[index] == gen,
                           "ResourcePool::free: stale or foreign handle detected");

        handle.id           = 0;
        _My_resource[index] = ResourceType{};
        _My_freelist.push_back(index);
    }

    ResourceType* get(const Tag& handle) {
        if (!handle.isValid()) {
            RENDERX_WARN("ResourcePool::get : invalid handle");
            return nullptr;
        }

        uint64_t raw   = Decrypt(handle.id);
        auto     index = static_cast<ValueType>(raw & 0xFFFFFFFF);
        auto     gen   = static_cast<ValueType>(raw >> 32);

        if (!(index < _My_resource.size() && _My_generation[index] == gen)) {
            RENDERX_WARN("stale or foreign handle detected");
            return nullptr;
        }

        return &_My_resource[index];
    }

    template <typename Fn> void ForEach(Fn&& fn) {
        for (size_t i = 1; i < _My_resource.size(); ++i) {
            if (_My_generation[i] != 0) {
                fn(_My_resource[i]);
            }
        }
    }

    template <typename Fn> void ForEachAlive(Fn&& fn) {
        for (ValueType i = 1; i < _My_resource.size(); ++i) {
            // skip freed slots
            if (std::find(_My_freelist.begin(), _My_freelist.end(), i) != _My_freelist.end())
                continue;

            uint32_t gen = _My_generation[i];

            uint64_t raw = (static_cast<uint64_t>(gen) << 32) | static_cast<uint64_t>(i);

            Tag handle;
            handle.id = Encrypt(raw);

            fn(_My_resource[i], handle);
        }
    }
    
    bool IsAlive(const Tag& handle) const {
        if (!handle.isValid())
            return false;

        uint64_t raw   = Decrypt(handle.id);
        auto     index = static_cast<ValueType>(raw & 0xFFFFFFFF);
        auto     gen   = static_cast<ValueType>(raw >> 32);

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
    static uint64_t RotateLeft(uint64_t x, int r) { return (x << r) | (x >> (64 - r)); }
    static uint64_t RotateRight(uint64_t x, int r) { return (x >> r) | (x << (64 - r)); }

    uint64_t Encrypt(uint64_t value) const {
        value ^= _My_key;
        value  = RotateLeft(value, 17);
        return value;
    }

    uint64_t Decrypt(uint64_t value) const {
        value  = RotateRight(value, 17);
        value ^= _My_key;
        return value;
    }

    static uint64_t GenerateKey() {
        static std::atomic<uint64_t> counter{0xA5B35705F00DBAAD};
        return counter.fetch_add(0x9E3779B97F4A7C15ull);
    }

private:
    std::vector<ResourceType> _My_resource;
    std::vector<ValueType>    _My_generation;
    std::vector<ValueType>    _My_freelist;
    uint64_t                  _My_key = 0;
};

// Class Declarations
struct VulkanContext;
class VulkanInstance;
class VulkanDevice;
class VulkanSwapchain;
class VulkanAllocator;
class VulkanCommandQueue;
class VulkanDescriptorPoolManager;
class VulkanDescriptorManager;
class VulkanStagingAllocator;

class VulkanSwapchain : public Swapchain {
public:
    VulkanSwapchain();
    ~VulkanSwapchain();

    // backend only
    bool create(const SwapchainCreateInfo& info);
    void destroy();
    void recreate(uint32_t width, uint32_t height);

    // renderx public api functions
    TextureHandle     GetImage(uint32_t imageIndex) const override { return m_ImageHandles[imageIndex]; };
    TextureHandle     GetDepth(uint32_t imageindex) const override { return m_DepthHandles[imageindex]; };
    TextureViewHandle GetImageView(uint32_t imageindex) const override { return m_ImageViewsHandles[imageindex]; };
    TextureViewHandle GetDepthView(uint32_t imageindex) const override { return m_DepthViewHandles[imageindex]; };
    Format            GetFormat() const override;
    uint32_t          GetWidth() const override { return m_Extent.width; }
    uint32_t          GetHeight() const override { return m_Extent.height; };
    uint32_t          GetImageCount() const override { return m_ImageCount; };
    uint32_t          AcquireNextImage() override;
    void              Present(uint32_t imageIndex) override;
    void              Resize(uint32_t width, uint32_t height) override;

private:
    friend class VulkanCommandQueue;

    VkSemaphore renderComplete() { return m_WaitSemaphores[m_CurrentImageIndex]; }
    VkSemaphore imageAvail() { return m_SignalSemaphores[m_currentSemaphoreIndex]; }

    void               createSwapchain(uint32_t width, uint32_t height);
    void               destroySwapchain();
    VkSurfaceFormatKHR chooseSurfaceFormat() const;
    VkPresentModeKHR   choosePresentMode() const;
    VkExtent2D         chooseExtent(uint32_t width, uint32_t height) const;

private:
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkFormat       m_Format    = VK_FORMAT_UNDEFINED;
    VkExtent2D     m_Extent{};
    uint32_t       m_ImageCount            = 0;
    uint32_t       m_CurrentImageIndex     = 0;
    uint32_t       m_currentSemaphoreIndex = 0;

    std::vector<VkImage>           m_Images;
    std::vector<TextureHandle>     m_ImageHandles;
    std::vector<TextureHandle>     m_DepthHandles;
    std::vector<TextureViewHandle> m_ImageViewsHandles;
    std::vector<TextureViewHandle> m_DepthViewHandles;
    std::vector<VkSemaphore>       m_WaitSemaphores;
    std::vector<VkSemaphore>       m_SignalSemaphores;
    SwapchainCreateInfo            m_Info{};
};

class VulkanInstance {
public:
    VulkanInstance(const InitDesc& window);
    ~VulkanInstance();

    VkInstance   getInstance() const { return m_Instance; }
    VkSurfaceKHR getSurface() const { return m_Surface; }

private:
    void createInstance(const InitDesc& window);
    void createSurface(const InitDesc& window);

private:
    VkInstance   m_Instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface  = VK_NULL_HANDLE;
};

struct DeviceInfo {
    VkPhysicalDevice                     device;
    VkPhysicalDeviceProperties           properties;
    VkPhysicalDeviceFeatures             features;
    VkPhysicalDeviceMemoryProperties     memoryProperties;
    std::vector<VkQueueFamilyProperties> queueFamilies;
    std::vector<VkExtensionProperties>   extensions;
    uint32_t                             score = 0;
    int                                  index;
};

class VulkanDevice {
public:
    VulkanDevice(VkInstance                      instance,
                 VkSurfaceKHR                    surface,
                 const std::vector<const char*>& requiredExtensions = {},
                 const std::vector<const char*>& requiredLayers     = {});
    ~VulkanDevice();

    VkPhysicalDevice                  physical() const { return m_PhysicalDevice; }
    VkDevice                          logical() const { return m_Device; }
    VkQueue                           graphicsQueue() const { return m_GraphicsQueue; }
    VkQueue                           computeQueue() const { return m_ComputeQueue; }
    VkQueue                           transferQueue() const { return m_TransferQueue; }
    uint32_t                          graphicsFamily() const { return m_GraphicsFamily; }
    uint32_t                          computeFamily() const { return m_ComputeFamily; }
    uint32_t                          transferFamily() const { return m_TransferFamily; }
    const VkPhysicalDeviceLimits&     limits() const { return m_VkLimits; }
    const VkPhysicalDeviceProperties& VkProperties() { return m_VkProperties; }

private:
    DeviceInfo gatherDeviceInfo(VkPhysicalDevice device) const;
    void       logDeviceInfo(uint32_t index, const DeviceInfo& info) const;
    uint32_t   scoreDevice(const DeviceInfo& info) const;
    int        selectDevice(const std::vector<DeviceInfo>& devices) const;
    bool       isDeviceSuitable(VkPhysicalDevice device) const;
    void       createLogicalDevice(const std::vector<const char*>& requiredExtensions,
                                   const std::vector<const char*>& requiredLayers);
    void       queryLimits();

private:
    VkInstance                 m_Instance       = VK_NULL_HANDLE;
    VkSurfaceKHR               m_Surface        = VK_NULL_HANDLE;
    VkPhysicalDevice           m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice                   m_Device         = VK_NULL_HANDLE;
    VkQueue                    m_GraphicsQueue  = VK_NULL_HANDLE;
    VkQueue                    m_ComputeQueue   = VK_NULL_HANDLE;
    VkQueue                    m_TransferQueue  = VK_NULL_HANDLE;
    uint32_t                   m_GraphicsFamily = UINT32_MAX;
    uint32_t                   m_ComputeFamily  = UINT32_MAX;
    uint32_t                   m_TransferFamily = UINT32_MAX;
    VkPhysicalDeviceLimits     m_VkLimits{};
    VkPhysicalDeviceProperties m_VkProperties{};
};

class VulkanAllocator {
public:
    VulkanAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
    ~VulkanAllocator();

    bool createBuffer(VkDeviceSize             size,
                      VkBufferUsageFlags       usage,
                      VmaMemoryUsage           memoryUsage,
                      VmaAllocationCreateFlags flags,
                      VkBuffer&                outBuffer,
                      VmaAllocation&           outAllocation,
                      VmaAllocationInfo*       outInfo = nullptr);

    void destroyBuffer(VkBuffer buffer, VmaAllocation allocation);

    bool createImage(const VkImageCreateInfo& imageInfo,
                     VmaMemoryUsage           memoryUsage,
                     VmaAllocationCreateFlags flags,
                     VkImage&                 outImage,
                     VmaAllocation&           outAllocation,
                     VmaAllocationInfo*       outInfo = nullptr);

    void         destroyImage(VkImage image, VmaAllocation allocation);
    void*        map(VmaAllocation allocation);
    void         unmap(VmaAllocation allocation);
    VmaAllocator handle() const { return m_Allocator; }

private:
    VmaAllocator m_Allocator = VK_NULL_HANDLE;
};

// Per-frame staging allocation
struct StagingAllocation {
    VkBuffer buffer       = VK_NULL_HANDLE;
    uint32_t offset       = 0;
    uint32_t size         = 0;
    uint8_t* mappedPtr    = nullptr;
    uint64_t submissionID = 0; // Track when it was submitted
};

// Large staging buffer chunk
struct StagingChunk {
    VkBuffer      buffer           = VK_NULL_HANDLE;
    VmaAllocation allocation       = VK_NULL_HANDLE;
    uint8_t*      mappedPtr        = nullptr;
    uint32_t      size             = 0;
    uint32_t      currentOffset    = 0;
    uint64_t      lastSubmissionID = 0; // Last submission that used this chunk

    // Reset for reuse
    void reset() {
        currentOffset    = 0;
        lastSubmissionID = 0;
    }

    // Check if there's enough space for an allocation
    bool canAllocate(uint32_t requestedSize, uint32_t alignment = 256) const {
        uint32_t alignedOffset = (currentOffset + alignment - 1) & ~(alignment - 1);
        return (alignedOffset + requestedSize) <= size;
    }

    // Suballocate from this chunk
    StagingAllocation allocate(uint32_t requestedSize, uint32_t alignment = 256) {
        StagingAllocation alloc{};
        uint32_t          alignedOffset = (currentOffset + alignment - 1) & ~(alignment - 1);
        if (!(alignedOffset + requestedSize <= size)) {
            RENDERX_ERROR("Staging allocation exceeds chunk size requested size {}, size {}",
                          alignedOffset + requestedSize,
                          size);
        }

        alloc.buffer    = buffer;
        alloc.offset    = alignedOffset;
        alloc.size      = requestedSize;
        alloc.mappedPtr = mappedPtr + alignedOffset;
        currentOffset   = alignedOffset + requestedSize;
        return alloc;
    }
};

class VulkanStagingAllocator {
public:
    VulkanStagingAllocator(VulkanContext& ctx, uint32_t chunkSize = 64 * 1024 * 1024); // 64MB default
    ~VulkanStagingAllocator();
    StagingAllocation allocate(uint32_t size, uint32_t alignment = 256);
    void              submit(uint64_t submissionID);
    void              retire(uint64_t completedSubmissionID);
    void              cleanup();

private:
    StagingChunk* getOrCreateChunk(uint32_t requiredSize);
    StagingChunk  createChunk(uint32_t size);
    void          destroyChunk(StagingChunk& chunk);

private:
    VulkanContext&           m_Ctx;
    uint32_t                 m_ChunkSize;
    StagingChunk*            m_CurrentChunk = nullptr;
    std::deque<StagingChunk> m_InFlightChunks;
    std::deque<StagingChunk> m_FreeChunks;
    std::deque<StagingChunk> m_AllChunks;
    std::mutex               m_Mutex;
};

struct ImmediateUploadContext {
    VkCommandPool   commandPool   = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence         fence         = VK_NULL_HANDLE;
    bool            isRecording   = false;
};

class VulkanImmediateUploader {
public:
    VulkanImmediateUploader(VulkanContext& ctx);
    ~VulkanImmediateUploader();
    bool
    uploadBuffer(VkBuffer dstBuffer, const void* data, uint32_t size, uint32_t dstOffset = 0, uint32_t alignment = 256);
    bool uploadTexture(VkImage dstTexture, const void* data, uint32_t size, const TextureCopy& region);
    void beginBatch();
    void uploadBufferBatched(VkBuffer dstBuffer, const void* data, uint32_t size, uint32_t dstOffset = 0);
    void uploadTextureBatched(VkImage dstTexture, const void* data, uint32_t size, const TextureCopy& region);
    bool endBatch(); // Returns true if successful
private:
    void            ensureContext();
    void            resetContext();
    VkCommandBuffer beginSingleTimeCommands();
    void            endSingleTimeCommands(VkCommandBuffer cmd);

private:
    VulkanContext&         m_Ctx;
    ImmediateUploadContext m_ImmediateCtx;
    std::mutex             m_Mutex; // Protect immediate uploads
};

class VulkanLoadTimeStagingUploader {
public:
    explicit VulkanLoadTimeStagingUploader(VulkanContext& ctx);
    ~VulkanLoadTimeStagingUploader();

    // Non-copyable, non-movable
    VulkanLoadTimeStagingUploader(const VulkanLoadTimeStagingUploader&)            = delete;
    VulkanLoadTimeStagingUploader& operator=(const VulkanLoadTimeStagingUploader&) = delete;
    VulkanLoadTimeStagingUploader(VulkanLoadTimeStagingUploader&&)                 = delete;
    VulkanLoadTimeStagingUploader& operator=(VulkanLoadTimeStagingUploader&&)      = delete;

    // Queue a buffer upload — data is copied to CPU-side staging immediately
    void uploadBuffer(VkBuffer dst, const void* data, uint32_t size, uint32_t dstOffset = 0);

    // Queue a texture upload — data is copied to CPU-side staging immediately
    void uploadTexture(VkImage dst, const void* data, uint32_t size, const TextureCopy& region);

    // Execute all queued uploads in a single GPU submission, then free staging memory.
    // Call once after all assets are queued. Safe to call on empty queue (no-op).
    void flush();

    // Returns total bytes queued so far (before flush)
    uint32_t pendingBytes() const { return m_TotalSize; }

private:
    uint32_t pushData(const void* data, uint32_t size, uint32_t alignment = 256);

    struct BufferUpload {
        VkBuffer dst;
        uint32_t stagingOffset;
        uint32_t dstOffset;
        uint32_t size;
    };

    struct TextureUpload {
        VkImage     dst;
        uint32_t    stagingOffset;
        uint32_t    size;
        TextureCopy region;
    };

    VulkanContext& m_Ctx;

    VkCommandPool   m_Pool  = VK_NULL_HANDLE;
    VkCommandBuffer m_Cmd   = VK_NULL_HANDLE;
    VkFence         m_Fence = VK_NULL_HANDLE;

    std::vector<uint8_t>       m_CPUData; // all upload data packed linearly
    std::vector<BufferUpload>  m_BufferUploads;
    std::vector<TextureUpload> m_TextureUploads;
    uint32_t                   m_TotalSize = 0;
};

struct DeferredUpload {
    StagingAllocation staging;
    BufferHandle      dstBuffer;
    uint32_t          dstOffset;
    uint32_t          size;
    uint64_t          submissionID;
};

struct DeferredTextureUpload {
    StagingAllocation staging;
    TextureHandle     dstTexture;
    TextureCopy       region;
    uint64_t          submissionID;
};

class VulkanDeferredUploader {
public:
    VulkanDeferredUploader(VulkanContext& ctx);
    ~VulkanDeferredUploader();
    void     uploadBuffer(BufferHandle dstBuffer, const void* data, uint32_t size, uint32_t dstOffset = 0);
    void     uploadTexture(TextureHandle dstTexture, const void* data, uint32_t size, const TextureCopy& region);
    Timeline flush();
    void     retire(uint64_t completedSubmission);

private:
    VulkanContext&                     m_Ctx;
    std::vector<DeferredUpload>        m_PendingBufferUploads;
    std::vector<DeferredTextureUpload> m_PendingTextureUploads;
    std::mutex                         m_Mutex;
};

class VulkanCommandList : public CommandList {
public:
    VulkanCommandList(VkCommandBuffer cmdBuffer, QueueType queueType)
        : m_CommandBuffer(cmdBuffer),
          m_QueueType(queueType) {}
    void open() override;
    void close() override;
    void setPipeline(const PipelineHandle& pipeline) override;
    void setVertexBuffer(const BufferHandle& buffer, uint64_t offset = 0) override;
    void setIndexBuffer(const BufferHandle& buffer, uint64_t offset = 0, Format indextype = Format::UINT32) override;

    void draw(uint32_t vertexCount,
              uint32_t instanceCount = 1,
              uint32_t firstVertex   = 0,
              uint32_t firstInstance = 0) override;

    void drawIndexed(uint32_t indexCount,
                     int32_t  vertexOffset  = 0,
                     uint32_t instanceCount = 1,
                     uint32_t firstIndex    = 0,
                     uint32_t firstInstance = 0) override;
    void beginRenderPass(RenderPassHandle pass, const void* clearValues, uint32_t clearCount) override;
    void endRenderPass() override;
    void beginRendering(const RenderingDesc& desc) override;
    void endRendering() override;
    void writeBuffer(BufferHandle handle, const void* data, uint32_t offset, uint32_t size) override;
    void setFramebuffer(FramebufferHandle handle) override;
    void copyBufferToTexture(BufferHandle srcBuffer, TextureHandle dstTexture, const TextureCopy& region) override;
    void copyTexture(TextureHandle srcTexture, TextureHandle dstTexture, const TextureCopy& region) override;
    void copyBuffer(BufferHandle src, BufferHandle dst, const BufferCopy& region) override;
    void copyTextureToBuffer(TextureHandle srcTexture, BufferHandle dstBuffer, const TextureCopy& region) override;
    void Barrier(const Memory_Barrier* memoryBarriers,
                 uint32_t              memoryCount,
                 const BufferBarrier*  bufferBarriers,
                 uint32_t              bufferCount,
                 const TextureBarrier* imageBarriers,
                 uint32_t              imageCount) override;

    void setDescriptorSet(uint32_t slot, SetHandle set) override;
    void setDescriptorSets(uint32_t firstSlot, const SetHandle* sets, uint32_t count) override;
    void setBindlessTable(BindlessTableHandle table) override;
    void
    pushConstants(uint32_t slot, const void* data, uint32_t sizeIn32BitWords, uint32_t offsetIn32BitWords = 0) override;
    void setDescriptorHeaps(DescriptorHeapHandle* heaps, uint32_t count) override;
    void setInlineCBV(uint32_t slot, BufferHandle buf, uint64_t offset = 0) override;
    void setInlineSRV(uint32_t slot, BufferHandle buf, uint64_t offset = 0) override;
    void setInlineUAV(uint32_t slot, BufferHandle buf, uint64_t offset = 0) override;
    void setDescriptorBufferOffset(uint32_t slot, uint32_t bufferIndex, uint64_t byteOffset) override;
    void setDynamicOffset(uint32_t slot, uint32_t byteOffset) override;
    void pushDescriptor(uint32_t slot, const DescriptorWrite* writes, uint32_t count) override;
    void setViewport(const Viewport& viewport) override;
    void setScissor(const Scissor& scissor) override;
    // friends
    friend class VulkanCommandAllocator;
    friend class VulkanCommandQueue;

private:
    // TODO----------------------------------------
    //  void TransitionTexture(
    //  	TextureHandle handle,
    //  	uint32_t subresource,
    //  	const VulkanAccessState& newState);
    // void TransitionBuffer(
    // 	BufferHandle handle,
    // 	const VulkanAccessState& newState);
    // void FlushBarriers();
    //-----------------------------------------------

    const char*     m_DebugName;
    VkCommandBuffer m_CommandBuffer;
    QueueType       m_QueueType;

    // Track currently bound pipeline / layout so descriptor sets can be bound
    PipelineHandle       m_CurrentPipelineHandle;
    PipelineLayoutHandle m_CurrentPipelineLayoutHandle;
    VulkanPipeline       m_CurrentPipeline;
    VkPipelineLayout     m_BoundPipelineLayout;
    uint32_t             m_PushRangeCount;
    bool                 m_HasMultiplePushRanges;
    VkShaderStageFlags   m_PushConstantStages;

    VulkanPipelineLayout::StoredPushRange m_PushRanges[VulkanPipelineLayout::MAX_PUSH_RANGES];

    struct LocalTextureState {
        TextureHandle     handle;
        uint32_t          subresource;
        VulkanAccessState state;
    };

    struct LocalBufferState {
        BufferHandle      handle;
        VulkanAccessState state;
    };

    std::vector<LocalTextureState> m_LocalTextures;
    std::vector<LocalBufferState>  m_LocalBuffers;

    std::vector<VkImageMemoryBarrier2>  m_ImageBarriers;
    std::vector<VkBufferMemoryBarrier2> m_BufferBarriers;

    // Currently bound vertex/index buffers (handles) and offsets
    BufferHandle m_VertexBuffer;
    uint64_t     m_VertexBufferOffset = 0;
    BufferHandle m_IndexBuffer;
    uint64_t     m_IndexBufferOffset = 0;
};

class VulkanCommandAllocator : public CommandAllocator {
public:
    VulkanCommandAllocator(VkCommandPool pool, VkDevice device, const char* debugName = nullptr)
        : m_device(device),
          m_Pool(pool) {};

    ~VulkanCommandAllocator() = default;
    CommandList* Allocate() override;
    void         Free(CommandList* list) override;
    void         Reset(CommandList* list) override;
    void         Reset() override;
    friend class VulkanCommandQueue;

private:
    const char*   m_DebugName;
    QueueType     m_QueueType;
    VkCommandPool m_Pool;
    VkDevice      m_device;
};

class VulkanCommandQueue : public CommandQueue {
public:
    VulkanCommandQueue(VkDevice device, VkQueue queue, uint32_t family, QueueType type);
    ~VulkanCommandQueue();

private:
    void        addWait(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags stage);
    void        addWait2(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);
    void        addSignal2(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);
    VkQueue     Queue();
    VkSemaphore Semaphore() { return m_TimelineSemaphore; }

    VkDevice  m_Device = VK_NULL_HANDLE;
    VkQueue   m_Queue  = VK_NULL_HANDLE;
    uint32_t  m_Family = UINT32_MAX;
    QueueType m_Type   = QueueType::GRAPHICS;

    VkSemaphore m_TimelineSemaphore = VK_NULL_HANDLE;

    uint64_t m_Submitted = 0;
    uint64_t m_Completed = 0;

    uint32_t              m_WaitCount = 0;
    uint64_t              m_WaitValues[4];
    VkSemaphore           m_WaitSemaphores[4];
    VkPipelineStageFlags  m_WaitStages[4];
    VkSemaphoreSubmitInfo m_WaitInfos[4];

    VkSemaphore           m_SignalSemaphores[2];
    uint64_t              m_SignalValues[2];
    VkSemaphoreSubmitInfo m_SignalInfos[2];
    uint32_t              m_SignalCount = 0;
    friend class VulkanSwapchain;

public:
    CommandAllocator* CreateCommandAllocator(const char* debugName = nullptr) override;
    void              DestroyCommandAllocator(CommandAllocator* allocator) override;
    Timeline          Submit(CommandList* commandList) override;
    Timeline          Submit(const SubmitInfo& submitInfo) override;
    bool              Wait(Timeline value, uint64_t timeout = UINT64_MAX) override;
    void              WaitIdle() override;
    bool              Poll(Timeline value) override;
    Timeline          Completed() override;
    Timeline          Submitted() const override;
    float             TimestampFrequency() const override;
};

struct VulkanContext {
    void*                          window = nullptr;
    VulkanInstance*                instance;
    VulkanDevice*                  device;
    VulkanSwapchain*               swapchain;
    VulkanCommandQueue*            graphicsQueue;
    VulkanCommandQueue*            computeQueue;
    VulkanCommandQueue*            transferQueue;
    VulkanAllocator*               allocator;
    VulkanDescriptorManager*       descriptorSetManager;
    VulkanDescriptorPoolManager*   descriptorPoolManager;
    VulkanStagingAllocator*        stagingAllocator;
    VulkanImmediateUploader*       immediateUploader;
    VulkanDeferredUploader*        deferredUploader;
    VulkanLoadTimeStagingUploader* loadTimeStagingUploader;
};

// Global Resource Pools
extern ResourcePool<VulkanBuffer, BufferHandle>                 g_BufferPool;
extern ResourcePool<VulkanBufferView, BufferViewHandle>         g_BufferViewPool;
extern ResourcePool<VulkanTexture, TextureHandle>               g_TexturePool;
extern ResourcePool<VulkanTextureView, TextureViewHandle>       g_TextureViewPool;
extern ResourcePool<VulkanShader, ShaderHandle>                 g_ShaderPool;
extern ResourcePool<VulkanPipeline, PipelineHandle>             g_PipelinePool;
extern ResourcePool<VulkanPipelineLayout, PipelineLayoutHandle> g_PipelineLayoutPool;
extern ResourcePool<VulkanRenderPass, RenderPassHandle>         g_RenderPassPool;
extern ResourcePool<VulkanFramebuffer, FramebufferHandle>       g_FramebufferPool;
extern ResourcePool<VulkanSet, SetHandle>                       g_SetPool;
extern ResourcePool<VulkanSetLayout, SetLayoutHandle>           g_SetLayoutPool;
extern ResourcePool<VulkanDescriptorPool, DescriptorPoolHandle> g_DescriptorPoolPool;
extern ResourcePool<VulkanDescriptorHeap, DescriptorHeapHandle> g_DescriptorHeapPool;
extern ResourcePool<VulkanSampler, SamplerHandle>               g_SamplerPool;

// Global Hash Storage
// TODO implement better cache managenet
extern std::unordered_map<Hash64, BufferViewHandle> g_BufferViewCache;

// Context & Cleanup Functions
VulkanContext& GetVulkanContext();
void           freeAllVulkanResources();
void           VKShutdownCommon();

// Create a Descriptor Pool
inline VkDescriptorPool CreateDescriptorPool(const DescriptorPoolSizes& sizes, bool allowFree, bool updateAfterBind) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    auto&                             ctx = GetVulkanContext();

    if (sizes.uniformBufferCount > 0)
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizes.uniformBufferCount});
    if (sizes.storageBufferCount > 0)
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sizes.storageBufferCount});
    if (sizes.sampledImageCount > 0)
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, sizes.sampledImageCount});
    if (sizes.storageImageCount > 0)
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, sizes.storageImageCount});
    if (sizes.samplerCount > 0)
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLER, sizes.samplerCount});
    if (sizes.combinedImageSamplerCount > 0)
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, sizes.combinedImageSamplerCount});

    VkDescriptorPoolCreateFlags flags = 0;
    if (allowFree)
        flags |= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    if (updateAfterBind)
        flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags                      = flags;
    poolInfo.maxSets                    = sizes.maxSets;
    poolInfo.poolSizeCount              = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes                 = poolSizes.data();

    VkDescriptorPool pool;
    VkResult         result = vkCreateDescriptorPool(ctx.device->logical(), &poolInfo, nullptr, &pool);

    return (result == VK_SUCCESS) ? pool : VK_NULL_HANDLE;
}

// VMA Memory Allocation Conversion
/// Convert RenderX MemoryType to VMA allocation info
inline VmaAllocationCreateInfo ToVmaAllocationCreateInfo(MemoryType type, BufferFlags flags) {
    VmaAllocationCreateInfo allocInfo   = {};
    bool                    isDynamic   = Has(flags, BufferFlags::DYNAMIC);
    bool                    isStreaming = Has(flags, BufferFlags::STREAMING);

    switch (type) {
    case MemoryType::GPU_ONLY:
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInfo.flags = 0;
        break;

    case MemoryType::CPU_TO_GPU:
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        if (isDynamic || isStreaming) {
            allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }

        allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        break;

    case MemoryType::GPU_TO_CPU:
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        break;

    case MemoryType::CPU_ONLY:
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        break;

    case MemoryType::AUTO:
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;

        if (isDynamic || isStreaming) {
            allocInfo.flags |=
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
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
        return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    case MemoryType::GPU_TO_CPU:
        return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
               VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

    case MemoryType::CPU_ONLY:
        return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    case MemoryType::AUTO:
        return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    default:
        return 0;
    }
}

/// Extended allocation info with priority and dedicated allocation support
inline VmaAllocationCreateInfo ToVmaAllocationCreateInfoEx(MemoryType  type,
                                                           BufferFlags flags,
                                                           float       priority            = 0.5f,
                                                           bool        dedicatedAllocation = false) {
    VmaAllocationCreateInfo allocInfo = ToVmaAllocationCreateInfo(type, flags);
    allocInfo.priority                = priority;
    if (dedicatedAllocation) {
        allocInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    }

    return allocInfo;
}

/// Get preferred flags vs required flags
inline void
GetMemoryRequirements(MemoryType type, VkMemoryPropertyFlags& preferredFlags, VkMemoryPropertyFlags& requiredFlags) {
    requiredFlags  = 0;
    preferredFlags = 0;

    switch (type) {
    case MemoryType::GPU_ONLY:
        preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        requiredFlags  = 0; // Can fallback if no device-local available
        break;

    case MemoryType::CPU_TO_GPU:
        requiredFlags  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        preferredFlags = requiredFlags | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;

    case MemoryType::GPU_TO_CPU:
        requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                        VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        preferredFlags = requiredFlags;
        break;

    case MemoryType::CPU_ONLY:
        requiredFlags  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        preferredFlags = requiredFlags;
        break;

    case MemoryType::AUTO:
        preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        requiredFlags  = 0;
        break;
    }
}

// Image/Texture Memory Allocation
/// VMA allocation info specifically for images/textures
inline VmaAllocationCreateInfo
ToVmaAllocationCreateInfoForImage(MemoryType type, bool isRenderTarget = false, bool isDynamic = false) {
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
        allocInfo.usage         = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
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
inline bool IsHostVisible(MemoryType type) {
    return type == MemoryType::CPU_TO_GPU || type == MemoryType::GPU_TO_CPU || type == MemoryType::CPU_ONLY;
}

inline bool ShouldBePersistentlyMapped(MemoryType type, BufferFlags flags) {
    bool isDynamic   = Has(flags, BufferFlags::DYNAMIC);
    bool isStreaming = Has(flags, BufferFlags::STREAMING);

    return IsHostVisible(type) && (isDynamic || isStreaming);
}

inline const char* MemoryTypeToString(MemoryType type) {
    switch (type) {
    case MemoryType::GPU_ONLY:
        return "GPU_ONLY";
    case MemoryType::CPU_TO_GPU:
        return "CPU_TO_GPU";
    case MemoryType::GPU_TO_CPU:
        return "GPU_TO_CPU";
    case MemoryType::CPU_ONLY:
        return "CPU_ONLY";
    case MemoryType::AUTO:
        return "AUTO";
    default:
        return "UNKNOWN";
    }
}

inline MemoryType GetRecommendedMemoryType(BufferFlags flags) {
    if (Has(flags, BufferFlags::DYNAMIC) || Has(flags, BufferFlags::STREAMING)) {
        return MemoryType::CPU_TO_GPU;
    } else if (Has(flags, BufferFlags::STATIC)) {
        return MemoryType::GPU_ONLY;
    }

    return MemoryType::AUTO;
}

inline VkFormat ToVulkanFormat(Format format) {
    switch (format) {
    case Format::UNDEFINED:
        return VK_FORMAT_UNDEFINED;
    case Format::R8_UNORM:
        return VK_FORMAT_R8_UNORM;
    case Format::RG8_UNORM:
        return VK_FORMAT_R8G8_UNORM;
    case Format::RGBA8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case Format::RGBA8_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case Format::BGRA8_UNORM:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case Format::BGRA8_SRGB:
        return VK_FORMAT_B8G8R8A8_SRGB;
    case Format::R16_SFLOAT:
        return VK_FORMAT_R16_SFLOAT;
    case Format::RG16_SFLOAT:
        return VK_FORMAT_R16G16_SFLOAT;
    case Format::RGBA16_SFLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case Format::R32_SFLOAT:
        return VK_FORMAT_R32_SFLOAT;
    case Format::RG32_SFLOAT:
        return VK_FORMAT_R32G32_SFLOAT;
    case Format::RGB32_SFLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case Format::RGBA32_SFLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case Format::D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case Format::D32_SFLOAT:
        return VK_FORMAT_D32_SFLOAT;
    case Format::BC1_RGBA_UNORM:
        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case Format::BC1_RGBA_SRGB:
        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case Format::BC3_UNORM:
        return VK_FORMAT_BC3_UNORM_BLOCK;
    case Format::BC3_SRGB:
        return VK_FORMAT_BC3_SRGB_BLOCK;

    case Format::UINT32: {
        RENDERX_ERROR("Using indesx type as the buffer format");
        return VK_FORMAT_UNDEFINED;
    }
    case Format::UINT16: {
        RENDERX_ERROR("Using index type as the buffer format");
        return VK_FORMAT_UNDEFINED;
    }
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

inline VkIndexType ToVulkanIndexType(Format fmt) {
    switch (fmt) {
    case Format::UINT32:
        return VK_INDEX_TYPE_UINT32;
    case Format::UINT16:
        return VK_INDEX_TYPE_UINT16;
    default:
        return VK_INDEX_TYPE_UINT32;
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
    case TextureType::TEXTURE_2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case TextureType::TEXTURE_3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    case TextureType::TEXTURE_CUBE:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case TextureType::TEXTURE_2D_ARRAY:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    default:
        return VK_IMAGE_VIEW_TYPE_2D;
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
inline VkSamplerAddressMode ToVulkanAddressMode(AddressMode wrap) {
    switch (wrap) {
    case AddressMode::REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case AddressMode::MIRRORED_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case AddressMode::CLAMP_TO_EDGE:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case AddressMode::CLAMP_TO_BORDER:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    default:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

// Topology/Primitive Type Conversion
inline VkPrimitiveTopology ToVulkanTopology(Topology topology) {
    switch (topology) {
    case Topology::POINTS:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case Topology::LINES:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case Topology::LINE_STRIP:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case Topology::TRIANGLES:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case Topology::TRIANGLE_STRIP:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case Topology::TRIANGLE_FAN:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    default:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

// Compare Function Conversion
inline VkCompareOp ToVulkanCompareOp(CompareOp func) {
    switch (func) {
    case CompareOp::NEVER:
        return VK_COMPARE_OP_NEVER;
    case CompareOp::LESS:
        return VK_COMPARE_OP_LESS;
    case CompareOp::EQUAL:
        return VK_COMPARE_OP_EQUAL;
    case CompareOp::LESS_EQUAL:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOp::GREATER:
        return VK_COMPARE_OP_GREATER;
    case CompareOp::NOT_EQUAL:
        return VK_COMPARE_OP_NOT_EQUAL;
    case CompareOp::GREATER_EQUAL:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case CompareOp::ALWAYS:
        return VK_COMPARE_OP_ALWAYS;
    default:
        return VK_COMPARE_OP_LESS;
    }
}

// Blend Function Conversion
inline VkBlendFactor ToVulkanBlendFactor(BlendFunc func) {
    switch (func) {
    case BlendFunc::ZERO:
        return VK_BLEND_FACTOR_ZERO;
    case BlendFunc::ONE:
        return VK_BLEND_FACTOR_ONE;
    case BlendFunc::SRC_COLOR:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case BlendFunc::ONE_MINUS_SRC_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case BlendFunc::DST_COLOR:
        return VK_BLEND_FACTOR_DST_COLOR;
    case BlendFunc::ONE_MINUS_DST_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case BlendFunc::SRC_ALPHA:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case BlendFunc::ONE_MINUS_SRC_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case BlendFunc::DST_ALPHA:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case BlendFunc::ONE_MINUS_DST_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case BlendFunc::CONSTANT_COLOR:
        return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case BlendFunc::ONE_MINUS_CONSTANT_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    default:
        return VK_BLEND_FACTOR_ONE;
    }
}

// Blend Operation Conversion
inline VkBlendOp ToVulkanBlendOp(BlendOp op) {
    switch (op) {
    case BlendOp::ADD:
        return VK_BLEND_OP_ADD;
    case BlendOp::SUBTRACT:
        return VK_BLEND_OP_SUBTRACT;
    case BlendOp::REVERSE_SUBTRACT:
        return VK_BLEND_OP_REVERSE_SUBTRACT;
    case BlendOp::MIN:
        return VK_BLEND_OP_MIN;
    case BlendOp::MAX:
        return VK_BLEND_OP_MAX;
    default:
        return VK_BLEND_OP_ADD;
    }
}

// Cull Mode Conversion
inline VkCullModeFlags ToVulkanCullMode(CullMode mode) {
    switch (mode) {
    case CullMode::NONE:
        return VK_CULL_MODE_NONE;
    case CullMode::FRONT:
        return VK_CULL_MODE_FRONT_BIT;
    case CullMode::BACK:
        return VK_CULL_MODE_BACK_BIT;
    case CullMode::FRONT_AND_BACK:
        return VK_CULL_MODE_FRONT_AND_BACK;
    default:
        return VK_CULL_MODE_NONE;
    }
}

// Fill Mode/Polygon Mode Conversion
inline VkPolygonMode ToVulkanPolygonMode(FillMode mode) {
    switch (mode) {
    case FillMode::SOLID:
        return VK_POLYGON_MODE_FILL;
    case FillMode::WIREFRAME:
        return VK_POLYGON_MODE_LINE;
    case FillMode::POINT:
        return VK_POLYGON_MODE_POINT;
    default:
        return VK_POLYGON_MODE_FILL;
    }
}

// Load Operation Conversion
inline VkAttachmentLoadOp ToVulkanLoadOp(LoadOp op) {
    switch (op) {
    case LoadOp::LOAD:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case LoadOp::CLEAR:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case LoadOp::DONT_CARE:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    default:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
}

// Store Operation Conversion

inline VkAttachmentStoreOp ToVulkanStoreOp(StoreOp op) {
    switch (op) {
    case StoreOp::STORE:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case StoreOp::DONT_CARE:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    default:
        return VK_ATTACHMENT_STORE_OP_STORE;
    }
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
    VkImageUsageFlags usage =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    // Add depth/stencil usage for depth formats
    if (format == Format::D24_UNORM_S8_UINT || format == Format::D32_SFLOAT) {
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    } else {
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
    } else if (format == Format::D24_UNORM_S8_UINT) {
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    return VK_IMAGE_ASPECT_COLOR_BIT;
}

inline VkImageViewType ToVulkanViewType(TextureType type) {
    switch (type) {
    case TextureType::TEXTURE_1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case TextureType::TEXTURE_2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case TextureType::TEXTURE_3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    case TextureType::TEXTURE_CUBE:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case TextureType::TEXTURE_1D_ARRAY:
        return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    case TextureType::TEXTURE_2D_ARRAY:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    // case TextureType::T: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    default:
        return VK_IMAGE_VIEW_TYPE_2D;
    }
}

inline uint32_t GetMinVulkanAlignment(BufferFlags usages) {
    auto&       ctx       = GetVulkanContext();
    const auto& limits    = ctx.device->limits();
    uint32_t    alignment = 1; // default: no special requirement
    if (Has(usages, BufferFlags::UNIFORM)) {
        alignment = std::max(alignment, static_cast<uint32_t>(limits.minUniformBufferOffsetAlignment));
    }
    if (Has(usages, BufferFlags::STORAGE)) {
        alignment = std::max(alignment, static_cast<uint32_t>(limits.minStorageBufferOffsetAlignment));
    }
    // VERTEX / INDEX / INDIRECT / TRANSFER*
    // no Vulkan-mandated offset alignment
    // allocator handles natural alignment
    return alignment;
}

inline const char* FormatToString(Format format) {
    switch (format) {
    case Format::UNDEFINED:
        return "UNDEFINED";
    case Format::R8_UNORM:
        return "R8_UNORM";
    case Format::RG8_UNORM:
        return "RG8_UNORM";
    case Format::RGBA8_UNORM:
        return "RGBA8_UNORM";
    case Format::RGBA8_SRGB:
        return "RGBA8_SRGB";
    case Format::BGRA8_UNORM:
        return "BGRA8_UNORM";
    case Format::BGRA8_SRGB:
        return "BGRA8_SRGB";
    case Format::R16_SFLOAT:
        return "R16_SFLOAT";
    case Format::RG16_SFLOAT:
        return "RG16_SFLOAT";
    case Format::RGBA16_SFLOAT:
        return "RGBA16_SFLOAT";
    case Format::R32_SFLOAT:
        return "R32_SFLOAT";
    case Format::RG32_SFLOAT:
        return "RG32_SFLOAT";
    case Format::RGB32_SFLOAT:
        return "RGB32_SFLOAT";
    case Format::RGBA32_SFLOAT:
        return "RGBA32_SFLOAT";
    case Format::D24_UNORM_S8_UINT:
        return "D24_UNORM_S8_UINT";
    case Format::D32_SFLOAT:
        return "D32_SFLOAT";
    case Format::BC1_RGBA_UNORM:
        return "BC1_RGBA_UNORM";
    case Format::BC1_RGBA_SRGB:
        return "BC1_RGBA_SRGB";
    case Format::BC3_UNORM:
        return "BC3_UNORM";
    case Format::BC3_SRGB:
        return "BC3_SRGB";
    }

    return "UNKNOWN_FORMAT";
}

inline Format VkFormatToFormat(VkFormat format) {
    switch (format) {
    case VK_FORMAT_UNDEFINED:
        return Format::UNDEFINED;
    case VK_FORMAT_R8_UNORM:
        return Format::R8_UNORM;
    case VK_FORMAT_R8G8_UNORM:
        return Format::RG8_UNORM;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return Format::RGBA8_UNORM;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return Format::RGBA8_SRGB;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return Format::BGRA8_UNORM;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return Format::BGRA8_SRGB;
    case VK_FORMAT_R16_SFLOAT:
        return Format::R16_SFLOAT;
    case VK_FORMAT_R16G16_SFLOAT:
        return Format::RG16_SFLOAT;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return Format::RGBA16_SFLOAT;
    case VK_FORMAT_R32_SFLOAT:
        return Format::R32_SFLOAT;
    case VK_FORMAT_R32G32_SFLOAT:
        return Format::RG32_SFLOAT;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return Format::RGB32_SFLOAT;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return Format::RGBA32_SFLOAT;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return Format::D24_UNORM_S8_UINT;
    case VK_FORMAT_D32_SFLOAT:
        return Format::D32_SFLOAT;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        return Format::BC1_RGBA_UNORM;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        return Format::BC1_RGBA_SRGB;
    case VK_FORMAT_BC3_UNORM_BLOCK:
        return Format::BC3_UNORM;
    case VK_FORMAT_BC3_SRGB_BLOCK:
        return Format::BC3_SRGB;
    }

    return Format::UNDEFINED;
}

inline const char* VkColorSpaceToString(VkColorSpaceKHR cs) {
    switch (cs) {
    case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
        return "SRGB_NONLINEAR";
    case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
        return "DISPLAY_P3";
    case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
        return "EXTENDED_SRGB_LINEAR";
    case VK_COLOR_SPACE_HDR10_ST2084_EXT:
        return "HDR10_ST2084";
    case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
        return "BT2020_LINEAR";
    default:
        return "UNKNOWN_COLOR_SPACE";
    }
}

inline VkShaderStageFlagBits MapShaderStage(PipelineStage stage) {

    if (Has(stage, PipelineStage::VERTEX))
        return VK_SHADER_STAGE_VERTEX_BIT;

    if (Has(stage, PipelineStage::FRAGMENT))
        return VK_SHADER_STAGE_FRAGMENT_BIT;

    if (Has(stage, PipelineStage::COMPUTE))
        return VK_SHADER_STAGE_COMPUTE_BIT;

    if (Has(stage, PipelineStage::GEOMETRY))
        return VK_SHADER_STAGE_GEOMETRY_BIT;

    if (Has(stage, PipelineStage::TESS_CONTROL))
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

    if (Has(stage, PipelineStage::TESS_EVALUATION))
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

    return VK_SHADER_STAGE_ALL;
}

inline VkShaderStageFlags MapShaderStageFlags(PipelineStage stage) {
    VkShaderStageFlags result = 0;

    if (Has(stage, PipelineStage::VERTEX))
        result |= VK_SHADER_STAGE_VERTEX_BIT;

    if (Has(stage, PipelineStage::FRAGMENT))
        result |= VK_SHADER_STAGE_FRAGMENT_BIT;

    if (Has(stage, PipelineStage::COMPUTE))
        result |= VK_SHADER_STAGE_COMPUTE_BIT;

    if (Has(stage, PipelineStage::GEOMETRY))
        result |= VK_SHADER_STAGE_GEOMETRY_BIT;

    if (Has(stage, PipelineStage::TESS_CONTROL))
        result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

    if (Has(stage, PipelineStage::TESS_EVALUATION))
        result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

    return result;
}

static inline VkPipelineStageFlags2 MapPipelineStage(PipelineStage stage) {
    VkPipelineStageFlags2 result = 0;

    // Shader stages
    if (Has(stage, PipelineStage::VERTEX))
        result |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

    if (Has(stage, PipelineStage::FRAGMENT))
        result |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

    if (Has(stage, PipelineStage::COMPUTE))
        result |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

    if (Has(stage, PipelineStage::GEOMETRY))
        result |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;

    if (Has(stage, PipelineStage::TESS_CONTROL))
        result |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT;

    if (Has(stage, PipelineStage::TESS_EVALUATION))
        result |= VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;

    // Fixed-function / non-shader
    if (Has(stage, PipelineStage::DRAW_INDIRECT))
        result |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;

    if (Has(stage, PipelineStage::TRANSFER))
        result |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    if (Has(stage, PipelineStage::COLOR_ATTACHMENT_OUTPUT))
        result |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    if (Has(stage, PipelineStage::EARLY_FRAGMENT_TESTS))
        result |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;

    if (Has(stage, PipelineStage::LATE_FRAGMENT_TESTS))
        result |= VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

    if (Has(stage, PipelineStage::TOP_OF_PIPE))
        result |= VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;

    if (Has(stage, PipelineStage::BOTTOM_OF_PIPE))
        result |= VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

    if (Has(stage, PipelineStage::HOST))
        result |= VK_PIPELINE_STAGE_2_HOST_BIT;

    return result;
}

// resource tracking helpers
inline VulkanAccessState ConvertState(ResourceState state, PipelineStage stages, QueueType queue) {
    VulkanAccessState out{};

    VkPipelineStageFlags2 stageMask  = 0;
    VkAccessFlags2        accessMask = 0;
    VkImageLayout         layout     = VK_IMAGE_LAYOUT_UNDEFINED;

    if (Has(state, ResourceState::SHADER_RESOURCE)) {
        stageMask  |= MapPipelineStage(stages);
        accessMask |= VK_ACCESS_2_SHADER_READ_BIT;
        layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    if (Has(state, ResourceState::UNORDERED_ACCESS)) {
        stageMask  |= MapPipelineStage(stages);
        accessMask |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
        layout      = VK_IMAGE_LAYOUT_GENERAL;
    }

    if (Has(state, ResourceState::RENDER_TARGET)) {
        stageMask  |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        accessMask |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    if (Has(state, ResourceState::TRANSFER_SRC)) {
        stageMask  |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        accessMask |= VK_ACCESS_2_TRANSFER_READ_BIT;
        layout      = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    }

    if (Has(state, ResourceState::TRANSFER_DST)) {
        stageMask  |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        accessMask |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
        layout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }

    out.stageMask  = stageMask;
    out.accessMask = accessMask;
    out.layout     = layout;
    return out;
}

inline bool HasWriteAccess(VkAccessFlags2 access) {
    return access & (VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT |
                     VK_ACCESS_2_HOST_WRITE_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT);
}

inline bool IsReadOnly(VkAccessFlags2 access) {
    return !HasWriteAccess(access);
}

inline bool NeedsBarrier(const VulkanAccessState& oldState, const VulkanAccessState& newState) {
    bool writeHazard = HasWriteAccess(oldState.accessMask) || HasWriteAccess(newState.accessMask);

    bool layoutChange = oldState.layout != newState.layout;
    bool queueTransfer =
        oldState.queueFamily != newState.queueFamily && oldState.queueFamily != VK_QUEUE_FAMILY_IGNORED;

    bool stageOnlyChange = oldState.stageMask != newState.stageMask && IsReadOnly(oldState.accessMask) &&
                           IsReadOnly(newState.accessMask) && !layoutChange && !queueTransfer;

    // Skip barrier entirely
    if (stageOnlyChange)
        return false;

    return writeHazard || layoutChange || queueTransfer;
}

inline VulkanSubresourceState& GetSubresourceState(SparseTextureState& sparse, uint32_t subresource) {
    auto it = sparse.overrides.find(subresource);
    if (it != sparse.overrides.end())
        return it->second;

    return sparse.global;
}

// TODO----------------------------------------------------
//  inline void SetSubresourceState(
//  	SparseTextureState& sparse,
//  	uint32_t subresource,
//  	const VulkanAccessState& newState) {
//  	sparse.overrides[subresource] = newState;
//  }
//---------------------------------------------------------

inline VkAccessFlags2 MapAccess(AccessFlags access) {
    VkAccessFlags2 result = 0;

    if (Has(access, AccessFlags::INDIRECT_COMMAND_READ))
        result |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;

    if (Has(access, AccessFlags::INDEX_READ))
        result |= VK_ACCESS_2_INDEX_READ_BIT;

    if (Has(access, AccessFlags::VERTEX_ATTRIBUTE_READ))
        result |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;

    if (Has(access, AccessFlags::UNIFORM_READ))
        result |= VK_ACCESS_2_UNIFORM_READ_BIT;

    if (Has(access, AccessFlags::SHADER_READ))
        result |= VK_ACCESS_2_SHADER_READ_BIT;

    if (Has(access, AccessFlags::SHADER_WRITE))
        result |= VK_ACCESS_2_SHADER_WRITE_BIT;

    if (Has(access, AccessFlags::COLOR_ATTACHMENT_READ))
        result |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;

    if (Has(access, AccessFlags::COLOR_ATTACHMENT_WRITE))
        result |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    if (Has(access, AccessFlags::DEPTH_STENCIL_READ))
        result |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    if (Has(access, AccessFlags::DEPTH_STENCIL_WRITE))
        result |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    if (Has(access, AccessFlags::TRANSFER_READ))
        result |= VK_ACCESS_2_TRANSFER_READ_BIT;

    if (Has(access, AccessFlags::TRANSFER_WRITE))
        result |= VK_ACCESS_2_TRANSFER_WRITE_BIT;

    if (Has(access, AccessFlags::HOST_READ))
        result |= VK_ACCESS_2_HOST_READ_BIT;

    if (Has(access, AccessFlags::HOST_WRITE))
        result |= VK_ACCESS_2_HOST_WRITE_BIT;

    if (Has(access, AccessFlags::MEMORY_READ))
        result |= VK_ACCESS_2_MEMORY_READ_BIT;

    if (Has(access, AccessFlags::MEMORY_WRITE))
        result |= VK_ACCESS_2_MEMORY_WRITE_BIT;

    return result;
}

inline VkImageLayout MapLayout(TextureLayout layout) {
    switch (layout) {
    case TextureLayout::UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;

    case TextureLayout::GENERAL:
        return VK_IMAGE_LAYOUT_GENERAL;

    case TextureLayout::COLOR_ATTACHMENT:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    case TextureLayout::DEPTH_STENCIL_ATTACHMENT:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    case TextureLayout::SHADER_READ_ONLY:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    case TextureLayout::TRANSFER_SRC:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    case TextureLayout::TRANSFER_DST:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    case TextureLayout::PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    default:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

inline VkImageAspectFlags MapAspect(TextureAspect aspect) {
    VkImageAspectFlags result = 0;

    if (Has(aspect, TextureAspect::IMAGE_ASPECT_COLOR))
        result |= VK_IMAGE_ASPECT_COLOR_BIT;

    if (Has(aspect, TextureAspect::IMAGE_ASPECT_DEPTH))
        result |= VK_IMAGE_ASPECT_DEPTH_BIT;

    if (Has(aspect, TextureAspect::IMAGE_ASPECT_STENCIL))
        result |= VK_IMAGE_ASPECT_STENCIL_BIT;

    return result;
}

} // namespace RxVK
} // namespace Rx