# RenderX Debug Profiling Guide

## Overview

RenderX includes a comprehensive, categorized profiling system via **ProLog**. In **Debug builds**, detailed profiling is automatically enabled with granular instrumentation across GPU operations, memory allocation, synchronization, and command buffer recording.

## Features

### Automatic Debug Profiling

- **Debug builds**: Detailed profiling enabled by default with 2000-item buffer for high-frequency events
- **Release builds**: Minimal overhead; profiling disabled unless explicitly enabled
- **Thread-safe**: All profiling events are thread-synchronized via ProLog mutex
- **JSON export**: Chrome DevTools-compatible timeline saved to `RenderX.json`
- **Statistics file**: Summary written to `profile_statistics.txt`

### Instrumentation Categories

| Category | Macros | Use Cases |
|----------|--------|-----------|
| **GPU** | `PROFILE_GPU_CALL`, `PROFILE_GPU_CALL_WITH_SIZE` | Draw calls, GPU operations |
| **Synchronization** | `PROFILE_SYNC` | Fences, semaphores, barriers, queue operations |
| **Swapchain** | `PROFILE_SWAPCHAIN` | Acquire, present, recreation |
| **CommandBuffer** | `PROFILE_COMMAND_BUFFER` | Recording, allocation, execution |
| **Memory** | `PROFILE_MEMORY`, `PROFILE_MEMORY_ALLOC`, `PROFILE_MEMORY_FREE` | Allocations, deallocations, transfers |
| **Descriptor** | `PROFILE_DESCRIPTOR` | SRG creation, descriptor updates |
| **Frame** | `PROFILE_FRAME_BEGIN`, `PROFILE_FRAME_END`, `PROFILE_FRAME_STAT` | Per-frame metrics |
| **Validation** | `PROFILE_VALIDATION` | Debug validation, assertions |

## Macro Reference

### Basic Profiling

```cpp
// Profile a scope (automatic RAII timing)
PROFILE_SCOPE("MyOperation");

// Profile current function
PROFILE_FUNCTION();

// Profile with category
PROFILE_FUNCTION_CAT("MyCategory");
```

### GPU Operations

```cpp
// Simple GPU call
PROFILE_GPU_CALL("vkQueueSubmit");

// With size parameter (e.g., vertex count)
PROFILE_GPU_CALL_WITH_SIZE("DrawCall", vertexCount);

// With queue name
PROFILE_GPU_CALL_WITH_QUEUE("queueSubmit", "Graphics");
```

### Memory Tracking

```cpp
// Track allocation
PROFILE_MEMORY_ALLOC(bufferSize);

// Track deallocation
PROFILE_MEMORY_FREE(bufferSize);

// Custom memory event with metadata
PROFILE_MEMORY("StagingBufferAlloc", uploadSize);
```

### Synchronization Points

```cpp
// Fence/semaphore operations
PROFILE_SYNC("WaitForFence");
PROFILE_SYNC("SignalSemaphore");
```

### Per-Frame Metrics

```cpp
// Frame boundaries
PROFILE_FRAME_BEGIN(frameIndex);
// ... frame work ...
PROFILE_FRAME_END(frameIndex);

// Per-frame statistics
PROFILE_FRAME_STAT("DrawCallCount", numDrawCalls);
PROFILE_FRAME_STAT("TriangleCount", totalTriangles);
```

## Output Files

### 1. Chrome DevTools Timeline (`RenderX.json`)

- High-resolution timestamp data
- Categorized events with nesting depth
- Metadata (sizes, queue names, etc.)
- Open in Chrome: `chrome://tracing` → Load `RenderX.json`

Example timeline view:
```
Frame_0
├── VKBegin
│   ├── WaitForFences (sync)
│   ├── AcquireNextImage (swapchain)
│   └── ResetCommandPool (command_buffer)
├── VKCmdOpen (command_buffer)
├── VKCmdDraw (gpu) [512 vertices]
├── VKCmdClose (command_buffer)
└── VKEnd
    ├── vkQueueSubmit (sync)
    └── vkQueuePresent (swapchain)
```

### 2. Statistics Summary (`profile_statistics.txt`)

Aggregated stats by function:

```
Function                                 Calls      Total (ms)   Avg (ms)   Min (ms)   Max (ms)   % Time
===========================================================================================================
vkQueueSubmit                               120        45.230       0.377      0.120      1.240    18.5%
vkBeginCommandBuffer                        120        12.450       0.104      0.045      0.340    5.1%
VKCmdDraw                                   600        8.340        0.014      0.008      0.120    3.4%
...
```

## Configuration

### In Code (Debug)

```cpp
RenderX::Debug::ConfigureDetailedProfiling();  // Called automatically in Init()
```

### Advanced Config

```cpp
ProLog::ProfilerConfig config;
config.enableProfiling = true;
config.enableLogging = true;
config.bufferSize = 5000;      // Increase for more events before flush
config.autoFlush = true;       // Flush periodically or on buffer full
ProLog::SetConfig(config);
```

## Example Usage in Application

```cpp
#include "RenderX/RenderX.h"

int main() {
    Window window = { ... };
    RenderX::Init(window);
    
    // Begin profiling session
    // (ProLog::BeginSession called internally)
    
    for (int frame = 0; frame < 1000; ++frame) {
        PROFILE_FRAME_BEGIN(frame);
        
        RenderX::Begin();
        
        auto cmdList = RenderX::CreateCommandList();
        cmdList.open();
        
        PROFILE_GPU_CALL_WITH_SIZE("FirstPass", 1024);
        cmdList.draw(1024, 1, 0, 0);
        
        PROFILE_GPU_CALL_WITH_SIZE("SecondPass", 2048);
        cmdList.draw(2048, 1, 0, 0);
        
        cmdList.close();
        RenderX::ExecuteCommandList(cmdList);
        
        RenderX::End();
        
        PROFILE_FRAME_END(frame);
    }
    
    RenderX::ShutDown();
    // Writes RenderX.json and profile_statistics.txt
    
    return 0;
}
```

## Interpreting Results

### High-Level Metrics
- **Session Duration**: Total time for profiling session
- **% Time**: Percentage of total session time spent in function
- **Call Count**: How many times function was invoked

### Performance Analysis
1. **Sort by Total Time** → Identify bottlenecks
2. **Sort by Avg Time** → Find slow individual calls
3. **Compare Min/Max** → Detect variance (may indicate stalls, UI interaction)

### Chrome DevTools Tips
- **Zoom**: Mouse wheel on timeline
- **Pan**: Click + drag on timeline
- **Highlight**: Hover over event to see details and metadata
- **Search**: CTRL+F in trace viewer

## Disabling Profiling

### For Minimal Overhead in Debug
```cpp
ProLog::ProfilerConfig config;
config.enableProfiling = false;  // Disables all profiling
ProLog::SetConfig(config);
```

### Release Builds
Profiling macros expand to no-op (`(void)0`) in Release, so **zero overhead**.

## Best Practices

1. **Use Categories**: Always pass category strings (e.g., "GPU", "Sync") for better timeline organization
2. **Add Metadata**: Include relevant numbers (sizes, counts) for context
3. **Frame Boundaries**: Wrap frame logic with `PROFILE_FRAME_BEGIN/END` for high-level tracking
4. **Avoid Overhead**: In hot loops, consider wrapping only outer scope
5. **Export Often**: Run with short frame counts first to identify issues before long profiling sessions
6. **Compare Baselines**: Save statistics file before/after optimization for direct comparison

## Troubleshooting

### Missing Events
- Check `config.enableProfiling = true`
- Ensure `PROFILE_START_SESSION()` called before events
- Verify `PROFILE_END_SESSION()` called to flush all events

### Large JSON Files
- Increase `config.bufferSize` if auto-flush is truncating events
- Or reduce profiling scope (profile fewer frames)

### Performance Impact
- Profiling adds ~1-5% overhead in Debug builds
- High-frequency macros in tight loops may show measurable impact
- Use Release builds for production performance testing

---

**ProLog Integration**: RenderX uses [ProLog](../External_local/ProLog) for all profiling infrastructure. See `External_local/ProLog/Include/ProLog/ProLog.h` for lower-level API.
