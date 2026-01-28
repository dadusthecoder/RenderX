# Quick-Start: Debug Profiling in RenderX

## What You Get

- **Automatic profiling** in Debug builds (zero config needed)
- **Chrome DevTools timeline** visualization (`RenderX.json`)
- **Aggregated statistics** (`profile_statistics.txt`)
- **Zero overhead** in Release builds
- **Categorized events** (GPU, Sync, Memory, Swapchain, etc.)

## 5-Minute Setup

### 1. Build Debug
```bash
cd Build
msbuild RenderX.sln /p:Configuration=Debug
```

### 2. Run Your App
```cpp
#include "RenderX/RenderX.h"

int main() {
    RenderX::Init(window);
    
    // Profiling is AUTOMATIC in Debug builds!
    for (int i = 0; i < 100; ++i) {
        PROFILE_FRAME_BEGIN(i);
        RenderX::Begin();
        // ... render ...
        RenderX::End();
        PROFILE_FRAME_END(i);
    }
    
    RenderX::ShutDown();  // Writes RenderX.json and profile_statistics.txt
    return 0;
}
```

### 3. View Results
```bash
# Timeline visualization
open chrome://tracing
# Then load RenderX.json

# Statistics summary
cat profile_statistics.txt
```

## What To Look For

| File | Contains | Purpose |
|------|----------|---------|
| `RenderX.json` | Microsecond-precision timeline | Identify GPU stalls, sync overhead, CPU bottlenecks |
| `profile_statistics.txt` | Function call counts & times | Find hotspots by % of session time |

### Timeline Tips
- **Red bars** = GPU operations (draws, submits)
- **Yellow bars** = Synchronization (fences, semaphores)
- **Blue bars** = Swapchain (acquire, present)
- **Wide gaps** = GPU stall waiting for backbuffer
- **Tall bars** = Expensive single operation

### Statistics Tips
- **Sort by % Time** = Find biggest bottlenecks
- **Max duration** = Occasional spikes or consistent load?
- **Avg vs Min/Max** = Variance indicates stalls

## Common Optimizations

### GPU Stall (High `vkAcquireNextImageKHR`)
```
Before: 2.5 ms average
After:  0.5 ms average
```
**Solution**: Use mailbox present mode instead of FIFO
```cpp
// In RenderX Vulkan backend, prefer VK_PRESENT_MODE_MAILBOX_KHR
```

### High Command Overhead (Large `VKCmdOpen/Close/Submit`)
```
Before: 0.15 ms per cmdlist
After:  0.05 ms per cmdlist
```
**Solution**: Batch multiple draws per command buffer
```cpp
cmdList.open();
for (int i = 0; i < 100; ++i) {
    cmdList.draw(...);  // 100 draws in one buffer = 100x faster open/close
}
cmdList.close();
```

### Frame Variance (Min/Max differ by 10x+)
```
Min: 0.5 ms  Max: 5.0 ms
```
**Solution**: Reduce GPU sync points, enable vsync
```cpp
// Use timeline semaphores or frame-pipelined submission
```

## Advanced: Custom Profiling

Add your own hot-path profiling:
```cpp
#include "RenderX/DebugProfiler.h"

void MyFunction() {
    PROFILE_FUNCTION();  // Auto-names function
    
    // GPU work
    PROFILE_GPU_CALL_WITH_SIZE("MyDrawCall", vertexCount);
    
    // Memory
    PROFILE_MEMORY_ALLOC(bufferSize);
    
    // Sync
    PROFILE_SYNC("WaitForGPU");
}
```

**All macros** automatically disable in Release builds → **zero performance penalty**.

## Troubleshooting

| Issue | Cause | Fix |
|-------|-------|-----|
| No RenderX.json | Profiling disabled | Ensure Debug build: `NDEBUG` not set |
| Tiny JSON file | Few events captured | Increase `bufferSize` in ProLog config |
| Chrome won't load | Wrong format | Ensure file is valid JSON (check file size > 1MB) |
| Missing frame stats | Early shutdown | Call `RenderX::ShutDown()` to flush data |

## File Locations

After running your application:
```
Build/bin/Debug/RenderX/
├── RenderX.json              (Chrome timeline)
└── profile_statistics.txt     (Aggregated stats)
```

## Next Steps

1. **Run HelloTriangle** (already instrumented):
   ```bash
   .\Build\bin\Debug\RenderX\HelloTriangle.exe
   ```
2. **Open RenderX.json** in Chrome tracing
3. **Review profile_statistics.txt** for metrics
4. **Identify bottlenecks** (GPU acquire, command overhead, sync)
5. **Optimize** based on findings

---

**Full Documentation**: See [DEBUG_PROFILING.md](DEBUG_PROFILING.md)  
**Usage Example**: See [PROFILING_USAGE_EXAMPLE.cpp](PROFILING_USAGE_EXAMPLE.cpp)  
**Completion Report**: See [PROFILING_COMPLETION_REPORT.md](PROFILING_COMPLETION_REPORT.md)
