#pragma once
#include "RX_Core.h"
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace Rx {

namespace Validation {

// Validation severity levels
enum class ValidationSeverity {
    INFO,
    WARNING,
    _ERROR,
    FATAL
};

// Validation configuration
struct ValidationConfig {
    ValidationCategory enabledCategories = ValidationCategory::ALL;
    bool               breakOnError      = false;
    bool               breakOnWarning    = false;
    bool               logToConsole      = true;
    bool               logToFile         = false;
    std::string        logFilePath       = "validation.log";
};

// Resource tracking info
enum class ResourceState : uint8_t {
    CREATED,
    BOUND,
    IN_USE,
    DESTROYED
};

struct ResourceInfo {
    uint64_t      handleId;
    ResourceState state;
    std::string   debugName;
    uint64_t      creationFrame;
    uint64_t      lastUsedFrame;

    ResourceInfo()
        : handleId(0),
          state(ResourceState::CREATED),
          creationFrame(0),
          lastUsedFrame(0) {}
};

// Buffer tracking
struct BufferInfo : ResourceInfo {
    BufferDesc desc;
    bool       isMapped      = false;
    void*      mappedPointer = nullptr;
};

// Texture tracking
struct TextureInfo : ResourceInfo {
    TextureDesc           desc;
    std::vector<uint64_t> viewHandles;
};

// Pipeline tracking
struct PipelineInfo : ResourceInfo {
    PipelineDesc desc;
    bool         isCompute = false;
};

// Command list tracking
struct CommandListInfo {
    CommandListState          state;
    bool                      isInsideRenderPass = false;
    bool                      isInsideRendering  = false;
    PipelineHandle            boundPipeline;
    std::vector<BufferHandle> boundVertexBuffers;
    BufferHandle              boundIndexBuffer;
    FramebufferHandle         boundFramebuffer;
    RenderPassHandle          activeRenderPass;
    uint64_t                  recordingFrame;

    CommandListInfo()
        : state(CommandListState::INITIAL),
          isInsideRenderPass(false),
          isInsideRendering(false),
          boundPipeline(0),
          boundIndexBuffer(0),
          boundFramebuffer(0),
          activeRenderPass(0),
          recordingFrame(0) {}
};

// Validation message
struct ValidationMessage {
    ValidationSeverity severity;
    ValidationCategory category;
    std::string        message;
    std::string        file;
    int                line;
    uint64_t           frame;

    ValidationMessage(
        ValidationSeverity sev, ValidationCategory cat, const std::string& msg, const std::string& f = "", int l = 0)
        : severity(sev),
          category(cat),
          message(msg),
          file(f),
          line(l),
          frame(0) {}
};

// Main validation layer class
class RENDERX_EXPORT ValidationLayer {
public:
    static ValidationLayer& Get();

    void Initialize(const ValidationConfig& config = ValidationConfig());
    void Shutdown();

    // Configuration
    void                    SetConfig(const ValidationConfig& config);
    const ValidationConfig& GetConfig() const { return config_; }
    void                    EnableCategory(ValidationCategory category);
    void                    DisableCategory(ValidationCategory category);

    // Frame tracking
    void     BeginFrame();
    void     EndFrame();
    uint64_t GetCurrentFrame() const { return currentFrame_; }

    // Message reporting
    void Report(ValidationSeverity severity,
                ValidationCategory category,
                const std::string& message,
                const char*        file = nullptr,
                int                line = 0);

    const std::vector<ValidationMessage>& GetMessages() const { return messages_; }
    void                                  ClearMessages();

    // Buffer validation
    void RegisterBuffer(BufferHandle handle, const BufferDesc& desc, const char* debugName = nullptr);
    void UnregisterBuffer(BufferHandle handle);
    bool ValidateBuffer(BufferHandle handle, const char* context = nullptr);
    void ValidateBufferDesc(const BufferDesc& desc);
    void OnBufferMap(BufferHandle handle, void* ptr);
    void OnBufferUnmap(BufferHandle handle);

    // Texture validation
    void RegisterTexture(TextureHandle handle, const TextureDesc& desc, const char* debugName = nullptr);
    void UnregisterTexture(TextureHandle handle);
    bool ValidateTexture(TextureHandle handle, const char* context = nullptr);
    void ValidateTextureDesc(const TextureDesc& desc);
    void RegisterTextureView(TextureViewHandle handle, TextureHandle parent);
    void UnregisterTextureView(TextureViewHandle handle);
    bool ValidateTextureView(TextureViewHandle handle, const char* context = nullptr);

    // Pipeline validation
    void RegisterPipeline(PipelineHandle handle, const PipelineDesc& desc, const char* debugName = nullptr);
    void UnregisterPipeline(PipelineHandle handle);
    bool ValidatePipeline(PipelineHandle handle, const char* context = nullptr);
    void ValidatePipelineDesc(const PipelineDesc& desc);

    // Command list validation
    void RegisterCommandList(CommandList* cmdList);
    void UnregisterCommandList(CommandList* cmdList);
    void OnCommandListBegin(CommandList* cmdList);
    void OnCommandListEnd(CommandList* cmdList);
    void OnCommandListSubmit(CommandList* cmdList);
    void OnCommandListReset(CommandList* cmdList);

    // Command recording validation
    void ValidateDrawCall(CommandList* cmdList, uint32_t vertexCount, uint32_t instanceCount);
    void ValidateDrawIndexed(CommandList* cmdList, uint32_t indexCount, uint32_t instanceCount);
    void ValidateBeginRenderPass(CommandList* cmdList, RenderPassHandle pass);
    void ValidateEndRenderPass(CommandList* cmdList);
    void ValidateBeginRendering(CommandList* cmdList, const RenderingDesc& desc);
    void ValidateEndRendering(CommandList* cmdList);
    void ValidateSetPipeline(CommandList* cmdList, PipelineHandle pipeline);
    void ValidateSetVertexBuffer(CommandList* cmdList, BufferHandle buffer);
    void ValidateSetIndexBuffer(CommandList* cmdList, BufferHandle buffer);

    // Buffer operation validation
    void ValidateBufferCopy(BufferHandle src, BufferHandle dst, const BufferCopyRegion& region);
    void ValidateBufferWrite(BufferHandle buffer, uint32_t offset, uint32_t size);

    // Texture operation validation
    void ValidateTextureCopy(TextureHandle src, TextureHandle dst, const TextureCopyRegion& region);
    void ValidateBufferToTextureCopy(BufferHandle src, TextureHandle dst, const TextureCopyRegion& region);

    // Render pass validation
    void ValidateRenderPassDesc(const RenderPassDesc& desc);
    void ValidateFramebufferDesc(const FramebufferDesc& desc);
    void ValidateRenderingDesc(const RenderingDesc& desc);

    // Synchronization validation
    void ValidateQueueSubmit(QueueType queue, const SubmitInfo& info);
    void ValidateQueueWait(QueueType queue, Timeline value);

    // Statistics
    uint32_t GetErrorCount() const { return errorCount_; }
    uint32_t GetWarningCount() const { return warningCount_; }
    void     ResetStatistics();

private:
    ValidationLayer()                                  = default;
    ~ValidationLayer()                                 = default;
    ValidationLayer(const ValidationLayer&)            = delete;
    ValidationLayer& operator=(const ValidationLayer&) = delete;

    bool             IsCategoryEnabled(ValidationCategory category) const;
    void             LogMessage(const ValidationMessage& msg);
    CommandListInfo* GetCommandListInfo(CommandList* cmdList);

    ValidationConfig config_;
    std::mutex       mutex_;

    // Resource tracking
    std::unordered_map<uint64_t, BufferInfo>          buffers_;
    std::unordered_map<uint64_t, TextureInfo>         textures_;
    std::unordered_map<uint64_t, ResourceInfo>        textureViews_;
    std::unordered_map<uint64_t, PipelineInfo>        pipelines_;
    std::unordered_map<uint64_t, ResourceInfo>        resourceGroups_;
    std::unordered_map<CommandList*, CommandListInfo> commandLists_;

    // Messages and statistics
    std::vector<ValidationMessage> messages_;
    uint32_t                       errorCount_   = 0;
    uint32_t                       warningCount_ = 0;
    uint64_t                       currentFrame_ = 0;

    bool initialized_ = false;
};

#ifdef RX_ENABLE_VALIDATION

DefaultValidationConfig {
    static constexpr bool ENABLE_HANDLE_VALIDATION       = true;
    static constexpr bool ENABLE_STATE_VALIDATION        = true;
    static constexpr bool ENABLE_RESOURCE_VALIDATION     = true;
    static constexpr bool ENABLE_SYNC_VALIDATION         = true;
    static constexpr bool ENABLE_MEMORY_VALIDATION       = true;
    static constexpr bool ENABLE_PIPELINE_VALIDATION     = true;
    static constexpr bool ENABLE_DESCRIPTOR_VALIDATION   = true;
    static constexpr bool ENABLE_COMMAND_LIST_VALIDATION = true;
    static constexpr bool ENABLE_RENDER_PASS_VALIDATION  = true;

    // Debug break behavior
    static constexpr bool BREAK_ON_ERROR   = true;  // Break debugger on errors
    static constexpr bool BREAK_ON_WARNING = false; // Don't break on warnings

    // Logging behavior
    static constexpr bool LOG_TO_CONSOLE  = true;
    static constexpr bool LOG_TO_FILE     = false;
    static constexpr char LOG_FILE_PATH[] = "validation.log";

    // Performance options
    static constexpr bool TRACK_FRAME_STATISTICS = true;
    static constexpr bool DETECT_RESOURCE_LEAKS  = true;
    static constexpr bool VALIDATE_EVERY_CALL    = true; // Set false to reduce overhead
}

#define RX_VALIDATE_INIT(config)  ::Rx::Validation::ValidationLayer::Get().Initialize(config)
#define RX_VALIDATE_SHUTDOWN()    ::Rx::Validation::ValidationLayer::Get().Shutdown()
#define RX_VALIDATE_BEGIN_FRAME() ::Rx::Validation::ValidationLayer::Get().BeginFrame()
#define RX_VALIDATE_END_FRAME()   ::Rx::Validation::ValidationLayer::Get().EndFrame()

#define RX_VALIDATE_ERROR(category, msg)                                                                               \
    ::Rx::Validation::ValidationLayer::Get().Report(                                                                   \
        ::Rx::Validation::ValidationSeverity::ERROR, category, msg, __FILE__, __LINE__)

#define RX_VALIDATE_WARNING(category, msg)                                                                             \
    ::Rx::Validation::ValidationLayer::Get().Report(                                                                   \
        ::Rx::Validation::ValidationSeverity::WARNING, category, msg, __FILE__, __LINE__)

#define RX_VALIDATE_INFO(category, msg)                                                                                \
    ::Rx::Validation::ValidationLayer::Get().Report(                                                                   \
        ::Rx::Validation::ValidationSeverity::INFO, category, msg, __FILE__, __LINE__)

// Buffer validation macros
#define RX_VALIDATE_BUFFER_REGISTER(handle, desc, name)                                                                \
    ::Rx::Validation::ValidationLayer::Get().RegisterBuffer(handle, desc, name)

#define RX_VALIDATE_BUFFER_UNREGISTER(handle) ::Rx::Validation::ValidationLayer::Get().UnregisterBuffer(handle)

#define RX_VALIDATE_BUFFER(handle, context) ::Rx::Validation::ValidationLayer::Get().ValidateBuffer(handle, context)

#define RX_VALIDATE_BUFFER_DESC(desc) ::Rx::Validation::ValidationLayer::Get().ValidateBufferDesc(desc)

#define RX_VALIDATE_BUFFER_MAP(handle, ptr) ::Rx::Validation::ValidationLayer::Get().OnBufferMap(handle, ptr)

#define RX_VALIDATE_BUFFER_UNMAP(handle) ::Rx::Validation::ValidationLayer::Get().OnBufferUnmap(handle)

#define RX_VALIDATE_BUFFER_COPY(src, dst, region)                                                                      \
    ::Rx::Validation::ValidationLayer::Get().ValidateBufferCopy(src, dst, region)

#define RX_VALIDATE_BUFFER_WRITE(handle, offset, size)                                                                 \
    ::Rx::Validation::ValidationLayer::Get().ValidateBufferWrite(handle, offset, size)

// Texture validation macros
#define RX_VALIDATE_TEXTURE_REGISTER(handle, desc, name)                                                               \
    ::Rx::Validation::ValidationLayer::Get().RegisterTexture(handle, desc, name)

#define RX_VALIDATE_TEXTURE_UNREGISTER(handle) ::Rx::Validation::ValidationLayer::Get().UnregisterTexture(handle)

#define RX_VALIDATE_TEXTURE(handle, context) ::Rx::Validation::ValidationLayer::Get().ValidateTexture(handle, context)

#define RX_VALIDATE_TEXTURE_DESC(desc) ::Rx::Validation::ValidationLayer::Get().ValidateTextureDesc(desc)

#define RX_VALIDATE_TEXTURE_VIEW_REGISTER(handle, parent)                                                              \
    ::Rx::Validation::ValidationLayer::Get().RegisterTextureView(handle, parent)

#define RX_VALIDATE_TEXTURE_VIEW_UNREGISTER(handle)                                                                    \
    ::Rx::Validation::ValidationLayer::Get().UnregisterTextureView(handle)

#define RX_VALIDATE_TEXTURE_VIEW(handle, context)                                                                      \
    ::Rx::Validation::ValidationLayer::Get().ValidateTextureView(handle, context)

#define RX_VALIDATE_TEXTURE_COPY(src, dst, region)                                                                     \
    ::Rx::Validation::ValidationLayer::Get().ValidateTextureCopy(src, dst, region)

// Pipeline validation macros
#define RX_VALIDATE_PIPELINE_REGISTER(handle, desc, name)                                                              \
    ::Rx::Validation::ValidationLayer::Get().RegisterPipeline(handle, desc, name)

#define RX_VALIDATE_PIPELINE_UNREGISTER(handle) ::Rx::Validation::ValidationLayer::Get().UnregisterPipeline(handle)

#define RX_VALIDATE_PIPELINE(handle, context) ::Rx::Validation::ValidationLayer::Get().ValidatePipeline(handle, context)

#define RX_VALIDATE_PIPELINE_DESC(desc) ::Rx::Validation::ValidationLayer::Get().ValidatePipelineDesc(desc)

// Resource group validation macros
#define RX_VALIDATE_RESOURCE_GROUP_REGISTER(handle, desc)                                                              \
    ::Rx::Validation::ValidationLayer::Get().RegisterResourceGroup(handle, desc)

#define RX_VALIDATE_RESOURCE_GROUP_UNREGISTER(handle)                                                                  \
    ::Rx::Validation::ValidationLayer::Get().UnregisterResourceGroup(handle)

#define RX_VALIDATE_RESOURCE_GROUP(handle, context)                                                                    \
    ::Rx::Validation::ValidationLayer::Get().ValidateResourceGroup(handle, context)

#define RX_VALIDATE_RESOURCE_GROUP_DESC(desc) ::Rx::Validation::ValidationLayer::Get().ValidateResourceGroupDesc(desc)

// Command list validation macros
#define RX_VALIDATE_CMD_REGISTER(cmdList) ::Rx::Validation::ValidationLayer::Get().RegisterCommandList(cmdList)

#define RX_VALIDATE_CMD_UNREGISTER(cmdList) ::Rx::Validation::ValidationLayer::Get().UnregisterCommandList(cmdList)

#define RX_VALIDATE_CMD_BEGIN(cmdList) ::Rx::Validation::ValidationLayer::Get().OnCommandListBegin(cmdList)

#define RX_VALIDATE_CMD_END(cmdList) ::Rx::Validation::ValidationLayer::Get().OnCommandListEnd(cmdList)

#define RX_VALIDATE_CMD_SUBMIT(cmdList) ::Rx::Validation::ValidationLayer::Get().OnCommandListSubmit(cmdList)

#define RX_VALIDATE_CMD_RESET(cmdList) ::Rx::Validation::ValidationLayer::Get().OnCommandListReset(cmdList)

#define RX_VALIDATE_DRAW(cmdList, vertexCount, instanceCount)                                                          \
    ::Rx::Validation::ValidationLayer::Get().ValidateDrawCall(cmdList, vertexCount, instanceCount)

#define RX_VALIDATE_DRAW_INDEXED(cmdList, indexCount, instanceCount)                                                   \
    ::Rx::Validation::ValidationLayer::Get().ValidateDrawIndexed(cmdList, indexCount, instanceCount)

#define RX_VALIDATE_SET_PIPELINE(cmdList, pipeline)                                                                    \
    ::Rx::Validation::ValidationLayer::Get().ValidateSetPipeline(cmdList, pipeline)

#define RX_VALIDATE_SET_VERTEX_BUFFER(cmdList, buffer)                                                                 \
    ::Rx::Validation::ValidationLayer::Get().ValidateSetVertexBuffer(cmdList, buffer)

#define RX_VALIDATE_SET_INDEX_BUFFER(cmdList, buffer)                                                                  \
    ::Rx::Validation::ValidationLayer::Get().ValidateSetIndexBuffer(cmdList, buffer)

#define RX_VALIDATE_SET_RESOURCE_GROUP(cmdList, group)                                                                 \
    ::Rx::Validation::ValidationLayer::Get().ValidateSetResourceGroup(cmdList, group)

// Render pass validation macros
#define RX_VALIDATE_BEGIN_RENDER_PASS(cmdList, pass)                                                                   \
    ::Rx::Validation::ValidationLayer::Get().ValidateBeginRenderPass(cmdList, pass)

#define RX_VALIDATE_END_RENDER_PASS(cmdList) ::Rx::Validation::ValidationLayer::Get().ValidateEndRenderPass(cmdList)

#define RX_VALIDATE_BEGIN_RENDERING(cmdList, desc)                                                                     \
    ::Rx::Validation::ValidationLayer::Get().ValidateBeginRendering(cmdList, desc)

#define RX_VALIDATE_END_RENDERING(cmdList) ::Rx::Validation::ValidationLayer::Get().ValidateEndRendering(cmdList)

#define RX_VALIDATE_RENDER_PASS_DESC(desc) ::Rx::Validation::ValidationLayer::Get().ValidateRenderPassDesc(desc)

#define RX_VALIDATE_FRAMEBUFFER_DESC(desc) ::Rx::Validation::ValidationLayer::Get().ValidateFramebufferDesc(desc)

#define RX_VALIDATE_RENDERING_DESC(desc) ::Rx::Validation::ValidationLayer::Get().ValidateRenderingDesc(desc)

// Synchronization validation macros
#define RX_VALIDATE_QUEUE_SUBMIT(queue, info) ::Rx::Validation::ValidationLayer::Get().ValidateQueueSubmit(queue, info)

#define RX_VALIDATE_QUEUE_WAIT(queue, value) ::Rx::Validation::ValidationLayer::Get().ValidateQueueWait(queue, value)

#else

// No-op macros when validation is disabled
#define RX_VALIDATE_INIT(config)           ((void)0)
#define RX_VALIDATE_SHUTDOWN()             ((void)0)
#define RX_VALIDATE_BEGIN_FRAME()          ((void)0)
#define RX_VALIDATE_END_FRAME()            ((void)0)
#define RX_VALIDATE_ERROR(category, msg)   ((void)0)
#define RX_VALIDATE_WARNING(category, msg) ((void)0)
#define RX_VALIDATE_INFO(category, msg)    ((void)0)

#define RX_VALIDATE_BUFFER_REGISTER(handle, desc, name) ((void)0)
#define RX_VALIDATE_BUFFER_UNREGISTER(handle)           ((void)0)
#define RX_VALIDATE_BUFFER(handle, context)             (true)
#define RX_VALIDATE_BUFFER_DESC(desc)                   ((void)0)
#define RX_VALIDATE_BUFFER_MAP(handle, ptr)             ((void)0)
#define RX_VALIDATE_BUFFER_UNMAP(handle)                ((void)0)
#define RX_VALIDATE_BUFFER_COPY(src, dst, region)       ((void)0)
#define RX_VALIDATE_BUFFER_WRITE(handle, offset, size)  ((void)0)

#define RX_VALIDATE_TEXTURE_REGISTER(handle, desc, name)  ((void)0)
#define RX_VALIDATE_TEXTURE_UNREGISTER(handle)            ((void)0)
#define RX_VALIDATE_TEXTURE(handle, context)              (true)
#define RX_VALIDATE_TEXTURE_DESC(desc)                    ((void)0)
#define RX_VALIDATE_TEXTURE_VIEW_REGISTER(handle, parent) ((void)0)
#define RX_VALIDATE_TEXTURE_VIEW_UNREGISTER(handle)       ((void)0)
#define RX_VALIDATE_TEXTURE_VIEW(handle, context)         (true)
#define RX_VALIDATE_TEXTURE_COPY(src, dst, region)        ((void)0)

#define RX_VALIDATE_PIPELINE_REGISTER(handle, desc, name) ((void)0)
#define RX_VALIDATE_PIPELINE_UNREGISTER(handle)           ((void)0)
#define RX_VALIDATE_PIPELINE(handle, context)             (true)
#define RX_VALIDATE_PIPELINE_DESC(desc)                   ((void)0)

#define RX_VALIDATE_RESOURCE_GROUP_REGISTER(handle, desc) ((void)0)
#define RX_VALIDATE_RESOURCE_GROUP_UNREGISTER(handle)     ((void)0)
#define RX_VALIDATE_RESOURCE_GROUP(handle, context)       (true)
#define RX_VALIDATE_RESOURCE_GROUP_DESC(desc)             ((void)0)

#define RX_VALIDATE_CMD_REGISTER(cmdList)                            ((void)0)
#define RX_VALIDATE_CMD_UNREGISTER(cmdList)                          ((void)0)
#define RX_VALIDATE_CMD_BEGIN(cmdList)                               ((void)0)
#define RX_VALIDATE_CMD_END(cmdList)                                 ((void)0)
#define RX_VALIDATE_CMD_SUBMIT(cmdList)                              ((void)0)
#define RX_VALIDATE_CMD_RESET(cmdList)                               ((void)0)
#define RX_VALIDATE_DRAW(cmdList, vertexCount, instanceCount)        ((void)0)
#define RX_VALIDATE_DRAW_INDEXED(cmdList, indexCount, instanceCount) ((void)0)
#define RX_VALIDATE_SET_PIPELINE(cmdList, pipeline)                  ((void)0)
#define RX_VALIDATE_SET_VERTEX_BUFFER(cmdList, buffer)               ((void)0)
#define RX_VALIDATE_SET_INDEX_BUFFER(cmdList, buffer)                ((void)0)
#define RX_VALIDATE_SET_RESOURCE_GROUP(cmdList, group)               ((void)0)

#define RX_VALIDATE_BEGIN_RENDER_PASS(cmdList, pass) ((void)0)
#define RX_VALIDATE_END_RENDER_PASS(cmdList)         ((void)0)
#define RX_VALIDATE_BEGIN_RENDERING(cmdList, desc)   ((void)0)
#define RX_VALIDATE_END_RENDERING(cmdList)           ((void)0)
#define RX_VALIDATE_RENDER_PASS_DESC(desc)           ((void)0)
#define RX_VALIDATE_FRAMEBUFFER_DESC(desc)           ((void)0)
#define RX_VALIDATE_RENDERING_DESC(desc)             ((void)0)

#define RX_VALIDATE_QUEUE_SUBMIT(queue, info) ((void)0)
#define RX_VALIDATE_QUEUE_WAIT(queue, value)  ((void)0)

#endif // RX_ENABLE_VALIDATION
} // namespace Validation
} // namespace Rx