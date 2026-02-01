#pragma once
#include "ProLog/ProLog.h"
#include <string>

// Forward declaration to avoid circular includes
namespace Rx {
namespace Debug {
	inline void ConfigureDetailedProfiling();
	inline void PrintProfileReport();
}
}

// ============================================================================
// DEBUG PROFILING MACROS FOR DETAILED INSTRUMENTATION
// ============================================================================
// These macros provide fine-grained profiling with metadata capture
// Automatically disabled in Release builds via ProLog config

#ifdef RENDERX_DEBUG

#define PROFILE_GPU_CALL(name) \
	PROFILE_SCOPE_CAT(name, "GPU")

#define PROFILE_GPU_CALL_WITH_SIZE(name, size)  \
	ProLog::Timer timer##__LINE__(name, "GPU"); \
	timer##__LINE__.AddMetadata("size_bytes", std::to_string(size))

#define PROFILE_GPU_CALL_WITH_QUEUE(name, queue) \
	ProLog::Timer timer##__LINE__(name, "GPU");  \
	timer##__LINE__.AddMetadata("queue", queue)

#define PROFILE_MEMORY(name, size)                 \
	ProLog::Timer timer##__LINE__(name, "Memory"); \
	timer##__LINE__.AddMetadata("bytes", std::to_string(size))

#define PROFILE_MEMORY_ALLOC(size) \
	PROFILE_MEMORY("MemoryAllocation", size)

#define PROFILE_MEMORY_FREE(size)                          \
	ProLog::Timer timer##__LINE__("MemoryFree", "Memory"); \
	timer##__LINE__.AddMetadata("freed_bytes", std::to_string(size))

#define PROFILE_DESCRIPTOR(name) \
	PROFILE_SCOPE_CAT(name, "Descriptor")

#define PROFILE_SWAPCHAIN(name) \
	PROFILE_SCOPE_CAT(name, "Swapchain")

#define PROFILE_SYNC(name) \
	PROFILE_SCOPE_CAT(name, "Synchronization")

#define PROFILE_VALIDATION(name) \
	PROFILE_SCOPE_CAT(name, "Validation")

#define PROFILE_COMMAND_BUFFER(name) \
	PROFILE_SCOPE_CAT(name, "CommandBuffer")

// Frame-level profiling with context
#define PROFILE_FRAME_BEGIN(frameIndex) \
	ProLog::PerformanceMarker::BeginEvent("Frame_" + std::to_string(frameIndex), "Frame")

#define PROFILE_FRAME_END(frameIndex) \
	ProLog::PerformanceMarker::EndEvent("Frame_" + std::to_string(frameIndex))

#define PROFILE_FRAME_STAT(name, value)               \
	ProLog::Timer timer##__LINE__(name, "FrameStat"); \
	timer##__LINE__.AddMetadata("value", std::to_string(value))

#else

// Release build: no-op implementations
#define PROFILE_GPU_CALL(name) (void)0
#define PROFILE_GPU_CALL_WITH_SIZE(name, size) (void)0
#define PROFILE_GPU_CALL_WITH_QUEUE(name, queue) (void)0
#define PROFILE_MEMORY(name, size) (void)0
#define PROFILE_MEMORY_ALLOC(size) (void)0
#define PROFILE_MEMORY_FREE(size) (void)0
#define PROFILE_DESCRIPTOR(name) (void)0
#define PROFILE_SWAPCHAIN(name) (void)0
#define PROFILE_SYNC(name) (void)0
#define PROFILE_VALIDATION(name) (void)0
#define PROFILE_COMMAND_BUFFER(name) (void)0
#define PROFILE_FRAME_BEGIN(frameIndex) (void)0
#define PROFILE_FRAME_END(frameIndex) (void)0
#define PROFILE_FRAME_STAT(name, value) (void)0

#endif

// ============================================================================
// DEBUG PROFILING CONFIGURATION HELPER
// ============================================================================
namespace Rx {
namespace Debug {
	inline void ConfigureDetailedProfiling() {
#ifdef RENDERX_DEBUG
		ProLog::ProfilerConfig config;
		config.enableProfiling = true;
		config.enableLogging = true;
		config.bufferSize = 2000; // Larger buffer for detailed traces
		config.autoFlush = true;
		ProLog::SetConfig(config);
#endif
	}

	inline void PrintProfileReport() {
#ifdef RENDERX_DEBUG
		PROFILE_PRINT_STATS();
#endif
	}
}
}
