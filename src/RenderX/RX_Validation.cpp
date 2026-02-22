#include "RX_Validation.h"
#include <ctime>
#include <fstream>
#include <iomanip>

namespace Rx {
namespace Validation {

// Singleton instance
ValidationLayer& ValidationLayer::Get() {
    static ValidationLayer instance;
    return instance;
}

void ValidationLayer::Initialize(const ValidationConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        RX_CORE_WARN("ValidationLayer::Initialize called multiple times");
        return;
    }

    config_       = config;
    initialized_  = true;
    currentFrame_ = 0;
    errorCount_   = 0;
    warningCount_ = 0;

    RX_CORE_INFO("Validation Layer initialized");
    RX_CORE_INFO("  - Enabled categories: 0x{:08X}", static_cast<uint32_t>(config_.enabledCategories));
    RX_CORE_INFO("  - Break on _ERROR: {}", config_.breakOnError);
    RX_CORE_INFO("  - Break on warning: {}", config_.breakOnWarning);
}

void ValidationLayer::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return;
    }

    // Report any leaked resources
    if (!buffers_.empty()) {
        RX_CORE_ERROR("Validation: {} buffer(s) not destroyed before shutdown", buffers_.size());
        for (const auto& [id, info] : buffers_) {
            RX_CORE_ERROR("  - Buffer 0x{:016X} ({})", id, info.debugName.empty() ? "unnamed" : info.debugName);
        }
    }

    if (!textures_.empty()) {
        RX_CORE_ERROR("Validation: {} texture(s) not destroyed before shutdown", textures_.size());
        for (const auto& [id, info] : textures_) {
            RX_CORE_ERROR("  - Texture 0x{:016X} ({})", id, info.debugName.empty() ? "unnamed" : info.debugName);
        }
    }

    if (!pipelines_.empty()) {
        RX_CORE_ERROR("Validation: {} pipeline(s) not destroyed before shutdown", pipelines_.size());
    }

    // Clear all tracked resources
    buffers_.clear();
    textures_.clear();
    textureViews_.clear();
    pipelines_.clear();
    resourceGroups_.clear();
    commandLists_.clear();
    messages_.clear();

    RX_CORE_INFO("Validation Layer shutdown - Total _ERRORs: {}, warnings: {}", errorCount_, warningCount_);

    initialized_ = false;
}

void ValidationLayer::SetConfig(const ValidationConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

void ValidationLayer::EnableCategory(ValidationCategory category) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.enabledCategories = config_.enabledCategories | category;
}

void ValidationLayer::DisableCategory(ValidationCategory category) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.enabledCategories = config_.enabledCategories & ~category;
}

void ValidationLayer::BeginFrame() {
    std::lock_guard<std::mutex> lock(mutex_);
    currentFrame_++;
}

void ValidationLayer::EndFrame() {
    // Could add per-frame validation here
}

void ValidationLayer::Report(
    ValidationSeverity severity, ValidationCategory category, const std::string& message, const char* file, int line) {
    if (!initialized_ || !IsCategoryEnabled(category)) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    ValidationMessage msg(severity, category, message, file ? file : "", line);
    msg.frame = currentFrame_;

    messages_.push_back(msg);

    if (severity == ValidationSeverity::_ERROR || severity == ValidationSeverity::FATAL) {
        errorCount_++;
    } else if (severity == ValidationSeverity::WARNING) {
        warningCount_++;
    }

    LogMessage(msg);

    // Break if configured
    if ((severity == ValidationSeverity::_ERROR || severity == ValidationSeverity::FATAL) && config_.breakOnError) {
        RENDERX_DEBUGBREAK();
    } else if (severity == ValidationSeverity::WARNING && config_.breakOnWarning) {
        RENDERX_DEBUGBREAK();
    }
}

void ValidationLayer::ClearMessages() {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.clear();
}

void ValidationLayer::ResetStatistics() {
    std::lock_guard<std::mutex> lock(mutex_);
    errorCount_   = 0;
    warningCount_ = 0;
}

// Buffer validation
void ValidationLayer::RegisterBuffer(BufferHandle handle, const BufferDesc& desc, const char* debugName) {
    if (!IsCategoryEnabled(ValidationCategory::RESOURCE))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    BufferInfo info;
    info.handleId      = handle.id;
    info.state         = ResourceState::CREATED;
    info.desc          = desc;
    info.creationFrame = currentFrame_;
    info.lastUsedFrame = currentFrame_;
    if (debugName) {
        info.debugName = debugName;
    }

    buffers_[handle.id] = info;
}

void ValidationLayer::UnregisterBuffer(BufferHandle handle) {
    if (!IsCategoryEnabled(ValidationCategory::RESOURCE))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = buffers_.find(handle.id);
    if (it == buffers_.end()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Attempting to destroy non-existent buffer 0x{:016X}", handle.id));
        return;
    }

    if (it->second.isMapped) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::RESOURCE,
               fmt::format("Buffer 0x{:016X} destroyed while still mapped", handle.id));
    }

    buffers_.erase(it);
}

bool ValidationLayer::ValidateBuffer(BufferHandle handle, const char* context) {
    if (!IsCategoryEnabled(ValidationCategory::HANDLE))
        return true;

    std::lock_guard<std::mutex> lock(mutex_);

    if (!handle.isValid()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Invalid buffer handle in {}", context ? context : "unknown context"));
        return false;
    }

    auto it = buffers_.find(handle.id);
    if (it == buffers_.end()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Buffer 0x{:016X} not found ({})", handle.id, context ? context : "unknown context"));
        return false;
    }

    if (it->second.state == ResourceState::DESTROYED) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::RESOURCE,
               fmt::format("Using destroyed buffer 0x{:016X} ({})", handle.id, context ? context : "unknown context"));
        return false;
    }

    it->second.lastUsedFrame = currentFrame_;
    return true;
}

void ValidationLayer::ValidateBufferDesc(const BufferDesc& desc) {
    if (!IsCategoryEnabled(ValidationCategory::RESOURCE))
        return;

    if (desc.size == 0) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RESOURCE, "Buffer size cannot be 0");
    }

    if (!IsValidBufferFlags(desc.usage)) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RESOURCE, "Invalid buffer usage flags combination");
    }

    // Check for conflicting flags
    bool hasStatic    = Has(desc.usage, BufferFlags::STATIC);
    bool hasDynamic   = Has(desc.usage, BufferFlags::DYNAMIC);
    bool hasStreaming = Has(desc.usage, BufferFlags::STREAMING);

    if ((hasStatic && hasDynamic) || (hasStatic && hasStreaming) || (hasDynamic && hasStreaming)) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::RESOURCE,
               "Buffer cannot have multiple mutually exclusive flags (STATIC/DYNAMIC/STREAMING)");
    }

    // Validate memory type compatibility
    if (desc.memoryType == MemoryType::GPU_ONLY && hasDynamic) {
        Report(ValidationSeverity::WARNING,
               ValidationCategory::RESOURCE,
               "GPU_ONLY memory with DYNAMIC usage may cause performance issues");
    }

    if (desc.memoryType == MemoryType::CPU_TO_GPU && !Has(desc.usage, BufferFlags::UNIFORM) &&
        !Has(desc.usage, BufferFlags::VERTEX)) {
        Report(ValidationSeverity::WARNING,
               ValidationCategory::RESOURCE,
               "CPU_TO_GPU memory typically used for UNIFORM or VERTEX buffers");
    }
}

void ValidationLayer::OnBufferMap(BufferHandle handle, void* ptr) {
    if (!IsCategoryEnabled(ValidationCategory::MEMORY))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = buffers_.find(handle.id);
    if (it == buffers_.end()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Mapping non-existent buffer 0x{:016X}", handle.id));
        return;
    }

    if (it->second.isMapped) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::MEMORY, fmt::format("Buffer 0x{:016X} already mapped", handle.id));
        return;
    }

    // Check if memory type supports mapping
    if (it->second.desc.memoryType == MemoryType::GPU_ONLY) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::MEMORY,
               fmt::format("Cannot map GPU_ONLY buffer 0x{:016X}", handle.id));
        return;
    }

    it->second.isMapped      = true;
    it->second.mappedPointer = ptr;
}

void ValidationLayer::OnBufferUnmap(BufferHandle handle) {
    if (!IsCategoryEnabled(ValidationCategory::MEMORY))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = buffers_.find(handle.id);
    if (it == buffers_.end()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Unmapping non-existent buffer 0x{:016X}", handle.id));
        return;
    }

    if (!it->second.isMapped) {
        Report(ValidationSeverity::WARNING, ValidationCategory::MEMORY, fmt::format("Buffer 0x{:016X} not mapped", handle.id));
        return;
    }

    it->second.isMapped      = false;
    it->second.mappedPointer = nullptr;
}

// Texture validation
void ValidationLayer::RegisterTexture(TextureHandle handle, const TextureDesc& desc, const char* debugName) {
    if (!IsCategoryEnabled(ValidationCategory::RESOURCE))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    TextureInfo info;
    info.handleId      = handle.id;
    info.state         = ResourceState::CREATED;
    info.desc          = desc;
    info.creationFrame = currentFrame_;
    info.lastUsedFrame = currentFrame_;
    if (debugName) {
        info.debugName = debugName;
    }

    textures_[handle.id] = info;
}

void ValidationLayer::UnregisterTexture(TextureHandle handle) {
    if (!IsCategoryEnabled(ValidationCategory::RESOURCE))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = textures_.find(handle.id);
    if (it == textures_.end()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Attempting to destroy non-existent texture 0x{:016X}", handle.id));
        return;
    }

    // Check for orphaned views
    if (!it->second.viewHandles.empty()) {
        Report(ValidationSeverity::WARNING,
               ValidationCategory::RESOURCE,
               fmt::format("Texture 0x{:016X} destroyed with {} active views", handle.id, it->second.viewHandles.size()));
    }

    textures_.erase(it);
}

bool ValidationLayer::ValidateTexture(TextureHandle handle, const char* context) {
    if (!IsCategoryEnabled(ValidationCategory::HANDLE))
        return true;

    std::lock_guard<std::mutex> lock(mutex_);

    if (!handle.isValid()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Invalid texture handle in {}", context ? context : "unknown context"));
        return false;
    }

    auto it = textures_.find(handle.id);
    if (it == textures_.end()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Texture 0x{:016X} not found ({})", handle.id, context ? context : "unknown context"));
        return false;
    }

    it->second.lastUsedFrame = currentFrame_;
    return true;
}

void ValidationLayer::ValidateTextureDesc(const TextureDesc& desc) {
    if (!IsCategoryEnabled(ValidationCategory::RESOURCE))
        return;

    if (desc.width == 0 || desc.height == 0) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RESOURCE, "Texture dimensions cannot be 0");
    }

    if (desc.mipLevels == 0) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RESOURCE, "Texture must have at least 1 mip level");
    }

    if (desc.arrayLayers == 0) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RESOURCE, "Texture must have at least 1 array layer");
    }

    // Validate cube maps
    if (desc.type == TextureType::TEXTURE_CUBE && desc.arrayLayers != 6) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RESOURCE, "Cubemap textures must have exactly 6 array layers");
    }

    if (desc.type == TextureType::TEXTURE_CUBE && desc.width != desc.height) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RESOURCE, "Cubemap textures must be square (width == height)");
    }

    // Validate depth/stencil formats
    bool isDepthStencil = desc.format == Format::D24_UNORM_S8_UINT || desc.format == Format::D32_SFLOAT;

    if (isDepthStencil && !Has(desc.usage, TextureUsage::DEPTH_STENCIL)) {
        Report(
            ValidationSeverity::WARNING, ValidationCategory::RESOURCE, "Depth/stencil format without DEPTH_STENCIL usage flag");
    }

    if (!isDepthStencil && Has(desc.usage, TextureUsage::DEPTH_STENCIL)) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RESOURCE, "DEPTH_STENCIL usage requires depth/stencil format");
    }
}

void ValidationLayer::RegisterTextureView(TextureViewHandle handle, TextureHandle parent) {
    if (!IsCategoryEnabled(ValidationCategory::RESOURCE))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto parentIt = textures_.find(parent.id);
    if (parentIt != textures_.end()) {
        parentIt->second.viewHandles.push_back(handle.id);
    }

    ResourceInfo info;
    info.handleId      = handle.id;
    info.state         = ResourceState::CREATED;
    info.creationFrame = currentFrame_;
    info.lastUsedFrame = currentFrame_;

    textureViews_[handle.id] = info;
}

void ValidationLayer::UnregisterTextureView(TextureViewHandle handle) {
    if (!IsCategoryEnabled(ValidationCategory::RESOURCE))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = textureViews_.find(handle.id);
    if (it == textureViews_.end()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Attempting to destroy non-existent texture view 0x{:016X}", handle.id));
        return;
    }

    textureViews_.erase(it);
}

bool ValidationLayer::ValidateTextureView(TextureViewHandle handle, const char* context) {
    if (!IsCategoryEnabled(ValidationCategory::HANDLE))
        return true;

    std::lock_guard<std::mutex> lock(mutex_);

    if (!handle.isValid()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Invalid texture view handle in {}", context ? context : "unknown context"));
        return false;
    }

    auto it = textureViews_.find(handle.id);
    if (it == textureViews_.end()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Texture view 0x{:016X} not found ({})", handle.id, context ? context : "unknown context"));
        return false;
    }

    it->second.lastUsedFrame = currentFrame_;
    return true;
}

// Pipeline validation
void ValidationLayer::RegisterPipeline(PipelineHandle handle, const PipelineDesc& desc, const char* debugName) {
    if (!IsCategoryEnabled(ValidationCategory::PIPELINE))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    PipelineInfo info;
    info.handleId      = handle.id;
    info.state         = ResourceState::CREATED;
    info.desc          = desc;
    info.creationFrame = currentFrame_;
    info.lastUsedFrame = currentFrame_;
    if (debugName) {
        info.debugName = debugName;
    }

    pipelines_[handle.id] = info;
}

void ValidationLayer::UnregisterPipeline(PipelineHandle handle) {
    if (!IsCategoryEnabled(ValidationCategory::PIPELINE))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pipelines_.find(handle.id);
    if (it == pipelines_.end()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Attempting to destroy non-existent pipeline 0x{:016X}", handle.id));
        return;
    }

    pipelines_.erase(it);
}

bool ValidationLayer::ValidatePipeline(PipelineHandle handle, const char* context) {
    if (!IsCategoryEnabled(ValidationCategory::HANDLE))
        return true;

    std::lock_guard<std::mutex> lock(mutex_);

    if (!handle.isValid()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Invalid pipeline handle in {}", context ? context : "unknown context"));
        return false;
    }

    auto it = pipelines_.find(handle.id);
    if (it == pipelines_.end()) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::HANDLE,
               fmt::format("Pipeline 0x{:016X} not found ({})", handle.id, context ? context : "unknown context"));
        return false;
    }

    it->second.lastUsedFrame = currentFrame_;
    return true;
}

void ValidationLayer::ValidatePipelineDesc(const PipelineDesc& desc) {
    if (!IsCategoryEnabled(ValidationCategory::PIPELINE))
        return;

    if (desc.shaders.empty()) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::PIPELINE, "Pipeline must have at least one shader");
        return;
    }

    bool hasVertex   = false;
    bool hasFragment = false;

    for (const auto& shader : desc.shaders) {
        if (!shader.isValid()) {
            Report(ValidationSeverity::_ERROR, ValidationCategory::PIPELINE, "Pipeline contains invalid shader handle");
        }
    }

    if (!desc.layout.isValid()) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::PIPELINE, "Pipeline has invalid layout handle");
    }

    // Color format validation
    if (desc.colorFromats.empty() && !desc.renderPass.isValid()) {
        Report(ValidationSeverity::WARNING, ValidationCategory::PIPELINE, "Pipeline has no color attachments and no render pass");
    }
}

// Command list validation
void ValidationLayer::RegisterCommandList(CommandList* cmdList) {
    if (!IsCategoryEnabled(ValidationCategory::COMMAND_LIST))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    CommandListInfo info;
    info.state          = CommandListState::INITIAL;
    info.recordingFrame = currentFrame_;

    commandLists_[cmdList] = info;
}

void ValidationLayer::UnregisterCommandList(CommandList* cmdList) {
    if (!IsCategoryEnabled(ValidationCategory::COMMAND_LIST))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = commandLists_.find(cmdList);
    if (it == commandLists_.end()) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::COMMAND_LIST, "Attempting to destroy non-existent command list");
        return;
    }

    if (it->second.isInsideRenderPass || it->second.isInsideRendering) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::COMMAND_LIST, "Command list destroyed while inside render pass");
    }

    commandLists_.erase(it);
}

void ValidationLayer::OnCommandListBegin(CommandList* cmdList) {
    if (!IsCategoryEnabled(ValidationCategory::COMMAND_LIST))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    if (info->state == CommandListState::RECORDING) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::STATE, "Command list already in recording state");
        return;
    }

    if (info->state == CommandListState::SUBMITTED) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::STATE,
               "Cannot begin command list that has been submitted but not reset");
        return;
    }

    info->state              = CommandListState::RECORDING;
    info->recordingFrame     = currentFrame_;
    info->isInsideRenderPass = false;
    info->isInsideRendering  = false;
    info->boundPipeline      = PipelineHandle(0);
    info->boundVertexBuffers.clear();
    info->boundIndexBuffer = BufferHandle(0);
}

void ValidationLayer::OnCommandListEnd(CommandList* cmdList) {
    if (!IsCategoryEnabled(ValidationCategory::COMMAND_LIST))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    if (info->state != CommandListState::RECORDING) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::STATE,
               fmt::format("Command list not in recording state (current: {})", CommandListStateToString(info->state)));
        return;
    }

    if (info->isInsideRenderPass) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RENDER_PASS, "Command list ended while inside render pass");
    }

    if (info->isInsideRendering) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RENDER_PASS, "Command list ended while inside rendering block");
    }

    info->state = CommandListState::EXECUTABLE;
}

void ValidationLayer::OnCommandListSubmit(CommandList* cmdList) {
    if (!IsCategoryEnabled(ValidationCategory::COMMAND_LIST))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    if (info->state != CommandListState::EXECUTABLE) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::STATE,
               fmt::format("Cannot submit command list in state: {}", CommandListStateToString(info->state)));
        return;
    }

    info->state = CommandListState::SUBMITTED;
}

void ValidationLayer::OnCommandListReset(CommandList* cmdList) {
    if (!IsCategoryEnabled(ValidationCategory::COMMAND_LIST))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    info->state              = CommandListState::INITIAL;
    info->isInsideRenderPass = false;
    info->isInsideRendering  = false;
    info->boundPipeline      = PipelineHandle(0);
    info->boundVertexBuffers.clear();
    info->boundIndexBuffer = BufferHandle(0);
}

// Command recording validation
void ValidationLayer::ValidateDrawCall(CommandList* cmdList, uint32_t vertexCount, uint32_t instanceCount) {
    if (!IsCategoryEnabled(ValidationCategory::COMMAND_LIST))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    if (info->state != CommandListState::RECORDING) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::STATE, "Draw call outside recording state");
        return;
    }

    if (!info->isInsideRenderPass && !info->isInsideRendering) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::RENDER_PASS,
               "Draw call must be inside render pass or rendering block");
    }

    if (!info->boundPipeline.isValid()) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::PIPELINE, "Draw call without bound pipeline");
    }

    if (info->boundVertexBuffers.empty()) {
        Report(ValidationSeverity::WARNING, ValidationCategory::STATE, "Draw call without bound vertex buffer");
    }

    if (vertexCount == 0) {
        Report(ValidationSeverity::WARNING, ValidationCategory::COMMAND_LIST, "Draw call with 0 vertices");
    }

    if (instanceCount == 0) {
        Report(ValidationSeverity::WARNING, ValidationCategory::COMMAND_LIST, "Draw call with 0 instances");
    }
}

void ValidationLayer::ValidateDrawIndexed(CommandList* cmdList, uint32_t indexCount, uint32_t instanceCount) {
    if (!IsCategoryEnabled(ValidationCategory::COMMAND_LIST))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    if (info->state != CommandListState::RECORDING) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::STATE, "Draw indexed call outside recording state");
        return;
    }

    if (!info->isInsideRenderPass && !info->isInsideRendering) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::RENDER_PASS,
               "Draw indexed call must be inside render pass or rendering block");
    }

    if (!info->boundPipeline.isValid()) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::PIPELINE, "Draw indexed call without bound pipeline");
    }

    if (!info->boundIndexBuffer.isValid()) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::STATE, "Draw indexed call without bound index buffer");
    }

    if (info->boundVertexBuffers.empty()) {
        Report(ValidationSeverity::WARNING, ValidationCategory::STATE, "Draw indexed call without bound vertex buffer");
    }

    if (indexCount == 0) {
        Report(ValidationSeverity::WARNING, ValidationCategory::COMMAND_LIST, "Draw indexed call with 0 indices");
    }
}

void ValidationLayer::ValidateBeginRenderPass(CommandList* cmdList, RenderPassHandle pass) {
    if (!IsCategoryEnabled(ValidationCategory::RENDER_PASS))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    if (info->state != CommandListState::RECORDING) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::STATE, "BeginRenderPass outside recording state");
        return;
    }

    if (info->isInsideRenderPass) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::RENDER_PASS,
               "BeginRenderPass called while already inside render pass");
        return;
    }

    if (info->isInsideRendering) {
        Report(
            ValidationSeverity::_ERROR, ValidationCategory::RENDER_PASS, "BeginRenderPass called while inside rendering block");
        return;
    }

    if (!pass.isValid()) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::HANDLE, "BeginRenderPass with invalid render pass handle");
        return;
    }

    info->isInsideRenderPass = true;
    info->activeRenderPass   = pass;
}

void ValidationLayer::ValidateEndRenderPass(CommandList* cmdList) {
    if (!IsCategoryEnabled(ValidationCategory::RENDER_PASS))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    if (info->state != CommandListState::RECORDING) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::STATE, "EndRenderPass outside recording state");
        return;
    }

    if (!info->isInsideRenderPass) {
        Report(
            ValidationSeverity::_ERROR, ValidationCategory::RENDER_PASS, "EndRenderPass called without matching BeginRenderPass");
        return;
    }

    info->isInsideRenderPass = false;
    info->activeRenderPass   = RenderPassHandle(0);
}

void ValidationLayer::ValidateBeginRendering(CommandList* cmdList, const RenderingDesc& desc) {
    if (!IsCategoryEnabled(ValidationCategory::RENDER_PASS))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    if (info->state != CommandListState::RECORDING) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::STATE, "BeginRendering outside recording state");
        return;
    }

    if (info->isInsideRendering) {
        Report(ValidationSeverity::_ERROR,
               ValidationCategory::RENDER_PASS,
               "BeginRendering called while already inside rendering block");
        return;
    }

    if (info->isInsideRenderPass) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RENDER_PASS, "BeginRendering called while inside render pass");
        return;
    }

    if (desc.colorAttachments.empty() && !desc.hasDepthStencil) {
        Report(ValidationSeverity::WARNING, ValidationCategory::RENDER_PASS, "BeginRendering with no color or depth attachments");
    }

    info->isInsideRendering = true;
}

void ValidationLayer::ValidateEndRendering(CommandList* cmdList) {
    if (!IsCategoryEnabled(ValidationCategory::RENDER_PASS))
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    if (info->state != CommandListState::RECORDING) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::STATE, "EndRendering outside recording state");
        return;
    }

    if (!info->isInsideRendering) {
        Report(
            ValidationSeverity::_ERROR, ValidationCategory::RENDER_PASS, "EndRendering called without matching BeginRendering");
        return;
    }

    info->isInsideRendering = false;
}

void ValidationLayer::ValidateSetPipeline(CommandList* cmdList, PipelineHandle pipeline) {
    if (!IsCategoryEnabled(ValidationCategory::PIPELINE))
        return;

    if (!ValidatePipeline(pipeline, "SetPipeline")) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    if (info->state != CommandListState::RECORDING) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::STATE, "SetPipeline outside recording state");
        return;
    }

    info->boundPipeline = pipeline;
}

void ValidationLayer::ValidateSetVertexBuffer(CommandList* cmdList, BufferHandle buffer) {
    if (!IsCategoryEnabled(ValidationCategory::STATE))
        return;

    if (!ValidateBuffer(buffer, "SetVertexBuffer")) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    if (info->state != CommandListState::RECORDING) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::STATE, "SetVertexBuffer outside recording state");
        return;
    }

    // Check buffer usage
    auto bufIt = buffers_.find(buffer.id);
    if (bufIt != buffers_.end()) {
        if (!Has(bufIt->second.desc.usage, BufferFlags::VERTEX)) {
            Report(ValidationSeverity::_ERROR,
                   ValidationCategory::RESOURCE,
                   "Buffer used as vertex buffer without VERTEX usage flag");
        }
    }

    info->boundVertexBuffers.push_back(buffer);
}

void ValidationLayer::ValidateSetIndexBuffer(CommandList* cmdList, BufferHandle buffer) {
    if (!IsCategoryEnabled(ValidationCategory::STATE))
        return;

    if (!ValidateBuffer(buffer, "SetIndexBuffer")) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto* info = GetCommandListInfo(cmdList);
    if (!info)
        return;

    if (info->state != CommandListState::RECORDING) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::STATE, "SetIndexBuffer outside recording state");
        return;
    }

    // Check buffer usage
    auto bufIt = buffers_.find(buffer.id);
    if (bufIt != buffers_.end()) {
        if (!Has(bufIt->second.desc.usage, BufferFlags::INDEX)) {
            Report(
                ValidationSeverity::_ERROR, ValidationCategory::RESOURCE, "Buffer used as index buffer without INDEX usage flag");
        }
    }

    info->boundIndexBuffer = buffer;
}

// Buffer operation validation
void ValidationLayer::ValidateBufferCopy(BufferHandle src, BufferHandle dst, const BufferCopy& region) {
    if (!IsCategoryEnabled(ValidationCategory::RESOURCE))
        return;

    if (!ValidateBuffer(src, "CopyBuffer source") || !ValidateBuffer(dst, "CopyBuffer destination")) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto srcIt = buffers_.find(src.id);
    auto dstIt = buffers_.find(dst.id);

    if (srcIt != buffers_.end() && dstIt != buffers_.end()) {
        // Check usage flags
        if (!Has(srcIt->second.desc.usage, BufferFlags::TRANSFER_SRC)) {
            Report(ValidationSeverity::_ERROR, ValidationCategory::RESOURCE, "Source buffer missing TRANSFER_SRC usage flag");
        }

        if (!Has(dstIt->second.desc.usage, BufferFlags::TRANSFER_DST)) {
            Report(
                ValidationSeverity::_ERROR, ValidationCategory::RESOURCE, "Destination buffer missing TRANSFER_DST usage flag");
        }

        // Check bounds
        if (region.srcOffset + region.size > srcIt->second.desc.size) {
            Report(ValidationSeverity::_ERROR,
                   ValidationCategory::RESOURCE,
                   fmt::format("Buffer copy source out of bounds (offset: {}, size: {}, buffer size: {})",
                               region.srcOffset,
                               region.size,
                               srcIt->second.desc.size));
        }

        if (region.dstOffset + region.size > dstIt->second.desc.size) {
            Report(ValidationSeverity::_ERROR,
                   ValidationCategory::RESOURCE,
                   fmt::format("Buffer copy destination out of bounds (offset: {}, size: {}, buffer size: {})",
                               region.dstOffset,
                               region.size,
                               dstIt->second.desc.size));
        }
    }
}

void ValidationLayer::ValidateBufferWrite(BufferHandle buffer, uint32_t offset, uint32_t size) {
    if (!IsCategoryEnabled(ValidationCategory::MEMORY))
        return;

    if (!ValidateBuffer(buffer, "WriteBuffer")) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = buffers_.find(buffer.id);
    if (it != buffers_.end()) {
        if (offset + size > it->second.desc.size) {
            Report(ValidationSeverity::_ERROR,
                   ValidationCategory::MEMORY,
                   fmt::format(
                       "Buffer write out of bounds (offset: {}, size: {}, buffer size: {})", offset, size, it->second.desc.size));
        }

        if (it->second.desc.memoryType == MemoryType::GPU_ONLY) {
            Report(ValidationSeverity::WARNING, ValidationCategory::MEMORY, "Direct write to GPU_ONLY buffer may be inefficient");
        }
    }
}

// Render pass validation
void ValidationLayer::ValidateRenderPassDesc(const RenderPassDesc& desc) {
    if (!IsCategoryEnabled(ValidationCategory::RENDER_PASS))
        return;

    if (desc.colorAttachments.empty() && !desc.hasDepthStencil) {
        Report(ValidationSeverity::WARNING, ValidationCategory::RENDER_PASS, "Render pass with no color or depth attachments");
    }

    for (size_t i = 0; i < desc.colorAttachments.size(); ++i) {
        const auto& attachment = desc.colorAttachments[i];

        if (!attachment.handle.isValid()) {
            Report(ValidationSeverity::_ERROR,
                   ValidationCategory::RENDER_PASS,
                   fmt::format("Color attachment {} has invalid texture view", i));
        }

        if (attachment.format == Format::UNDEFINED) {
            Report(ValidationSeverity::_ERROR,
                   ValidationCategory::RENDER_PASS,
                   fmt::format("Color attachment {} has undefined format", i));
        }
    }

    if (desc.hasDepthStencil) {
        if (!desc.depthStencilAttachment.handle.isValid()) {
            Report(
                ValidationSeverity::_ERROR, ValidationCategory::RENDER_PASS, "Depth/stencil attachment has invalid texture view");
        }
    }
}

void ValidationLayer::ValidateFramebufferDesc(const FramebufferDesc& desc) {
    if (!IsCategoryEnabled(ValidationCategory::RENDER_PASS))
        return;

    if (desc.colorAttachments.empty() && !desc.depthStencilAttachment.isValid()) {
        Report(ValidationSeverity::WARNING, ValidationCategory::RENDER_PASS, "Framebuffer with no attachments");
    }

    if (desc.width == 0 || desc.height == 0) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RENDER_PASS, "Framebuffer dimensions cannot be 0");
    }

    for (size_t i = 0; i < desc.colorAttachments.size(); ++i) {
        if (!desc.colorAttachments[i].isValid()) {
            Report(ValidationSeverity::_ERROR,
                   ValidationCategory::RENDER_PASS,
                   fmt::format("Color attachment {} has invalid texture view", i));
        }
    }
}

void ValidationLayer::ValidateRenderingDesc(const RenderingDesc& desc) {
    if (!IsCategoryEnabled(ValidationCategory::RENDER_PASS))
        return;

    if (desc.colorAttachments.empty() && !desc.hasDepthStencil) {
        Report(ValidationSeverity::WARNING, ValidationCategory::RENDER_PASS, "Dynamic rendering with no attachments");
    }

    if (desc.width <= 0 || desc.height <= 0) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::RENDER_PASS, "Rendering dimensions must be positive");
    }
}

void ValidationLayer::ValidateQueueSubmit(QueueType queue, const SubmitInfo& info) {
    if (!IsCategoryEnabled(ValidationCategory::SYNCHRONIZATION))
        return;

    if (!info.commandList) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::SYNCHRONIZATION, "Queue submit with null command list");
        return;
    }

    if (info.commandListCount == 0) {
        Report(ValidationSeverity::WARNING, ValidationCategory::SYNCHRONIZATION, "Queue submit with 0 command list count");
    }
}

void ValidationLayer::ValidateQueueWait(QueueType queue, Timeline value) {
    if (!IsCategoryEnabled(ValidationCategory::SYNCHRONIZATION))
        return;

    // Could track timeline values per queue to detect potential deadlocks
}

// Private helper methods
bool ValidationLayer::IsCategoryEnabled(ValidationCategory category) const {
    return Has(config_.enabledCategories, category);
}

void ValidationLayer::LogMessage(const ValidationMessage& msg) {
    const char*               severityStr = "";
    spdlog::level::level_enum logLevel    = spdlog::level::info;

    switch (msg.severity) {
    case ValidationSeverity::INFO:
        severityStr = "INFO";
        logLevel    = spdlog::level::info;
        break;
    case ValidationSeverity::WARNING:
        severityStr = "WARNING";
        logLevel    = spdlog::level::warn;
        break;
    case ValidationSeverity::_ERROR:
        severityStr = "_ERROR";
        logLevel    = spdlog::level::err;
        break;
    case ValidationSeverity::FATAL:
        severityStr = "FATAL";
        logLevel    = spdlog::level::critical;
        break;
    }

    if (config_.logToConsole) {
        if (!msg.file.empty()) {
            Log::Core()->log(logLevel, "[VALIDATION {}] {} ({}:{})", severityStr, msg.message, msg.file, msg.line);
        } else {
            Log::Core()->log(logLevel, "[VALIDATION {}] {}", severityStr, msg.message);
        }
    }
}

CommandListInfo* ValidationLayer::GetCommandListInfo(CommandList* cmdList) {
    auto it = commandLists_.find(cmdList);
    if (it == commandLists_.end()) {
        Report(ValidationSeverity::_ERROR, ValidationCategory::COMMAND_LIST, "Command list not registered with validation layer");
        return nullptr;
    }
    return &it->second;
}

} // namespace Validation
} // namespace Rx