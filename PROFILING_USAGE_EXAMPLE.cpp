// ============================================================================
// EXAMPLE: Using RenderX Debug Profiling
// ============================================================================
// This example demonstrates how to leverage detailed profiling in debug builds
// to identify bottlenecks and validate optimizations.

#include "RenderX/RenderX.h"
#include <iostream>
#include <chrono>

class Application {
private:
    static constexpr int TARGET_FRAMES = 100;
    int frameCount = 0;
    
public:
    void Run() {
        // Initialize RenderX (automatically configures detailed profiling in Debug)
        Window window = {
            .platform = Platform::Windows,
            .api = GraphicsAPI::Vulkan,
            .displayHandle = GetModuleHandle(nullptr),
            .nativeHandle = /* HWND */,
            .width = 1280,
            .height = 720,
            .instanceExtensions = /* extensions */,
            .extensionCount = 0
        };
        
        RenderX::Init(window);
        
        // Main loop with per-frame profiling
        auto sessionStart = std::chrono::high_resolution_clock::now();
        
        while (frameCount < TARGET_FRAMES) {
            PROFILE_FRAME_BEGIN(frameCount);
            
            // CPU-side frame begin (acquire backbuffer, reset allocators)
            RenderX::Begin();
            
            // Create and record command buffers
            RecordFrame();
            
            // Submit and present
            RenderX::End();
            
            PROFILE_FRAME_END(frameCount);
            frameCount++;
        }
        
        auto sessionEnd = std::chrono::high_resolution_clock::now();
        auto totalMs = std::chrono::duration<double, std::milli>(sessionEnd - sessionStart).count();
        
        RenderX::ShutDown();  // Writes RenderX.json and profile_statistics.txt
        
        std::cout << "Profiling session complete:\n";
        std::cout << "  Total frames: " << frameCount << "\n";
        std::cout << "  Total time: " << totalMs << " ms\n";
        std::cout << "  Average FPS: " << (frameCount / (totalMs / 1000.0)) << "\n";
        std::cout << "\nOpen 'RenderX.json' in Chrome tracing (chrome://tracing)\n";
        std::cout << "Review 'profile_statistics.txt' for aggregated metrics\n";
    }
    
private:
    void RecordFrame() {
        PROFILE_SCOPE("RecordFrame");
        
        // Create command list
        auto cmdList = RenderX::CreateCommandList();
        
        {
            PROFILE_COMMAND_BUFFER("CmdListRecording");
            
            cmdList.open();
            
            // Simulate multi-pass rendering
            RecordGeometryPass(cmdList);
            RecordLightingPass(cmdList);
            RecordPostprocessPass(cmdList);
            
            cmdList.close();
        }
        
        // Submit
        {
            PROFILE_SCOPE("QueueSubmission");
            RenderX::ExecuteCommandList(cmdList);
        }
    }
    
    void RecordGeometryPass(CommandList& cmdList) {
        PROFILE_SCOPE("GeometryPass");
        
        // Set pipeline
        RenderX::PipelineHandle geometryPipeline = {}; // Get from cache
        cmdList.setPipeline(geometryPipeline);
        
        // Bind buffers
        {
            PROFILE_MEMORY("BindVertexBuffer", 65536);  // 64KB vertex data
            cmdList.setVertexBuffer(geometryVB, 0);
            cmdList.setIndexBuffer(geometryIB, 0);
        }
        
        // Draw
        {
            PROFILE_GPU_CALL_WITH_SIZE("DrawGeometry", 50000);  // 50k indices
            cmdList.drawIndexed(50000, 0, 1, 0, 0);
        }
    }
    
    void RecordLightingPass(CommandList& cmdList) {
        PROFILE_SCOPE("LightingPass");
        
        RenderX::PipelineHandle lightingPipeline = {}; // Get from cache
        cmdList.setPipeline(lightingPipeline);
        
        {
            PROFILE_GPU_CALL("DrawLighting");
            cmdList.draw(6, 1, 0, 0);  // Full-screen quad
        }
    }
    
    void RecordPostprocessPass(CommandList& cmdList) {
        PROFILE_SCOPE("PostprocessPass");
        
        RenderX::PipelineHandle postPipeline = {}; // Get from cache
        cmdList.setPipeline(postPipeline);
        
        {
            PROFILE_GPU_CALL("ApplyPostprocess");
            cmdList.draw(6, 1, 0, 0);  // Full-screen quad
        }
    }
};

int main() {
    Application app;
    app.Run();
    return 0;
}

// ============================================================================
// INTERPRETATION GUIDE
// ============================================================================
//
// After running this example, examine the output files:
//
// 1. RenderX.json (Chrome DevTools Timeline)
//    - Shows frame-by-frame GPU/CPU timeline
//    - Identify GPU stalls (long bars in vkAcquireNextImageKHR)
//    - Spot CPU overhead (time between commands)
//    - Zoom on slow frames to diagnose issues
//
// 2. profile_statistics.txt (Aggregated Metrics)
//    - RecordFrame: Total CPU time per frame
//    - GeometryPass: Vertex/index bind overhead
//    - vkQueueSubmit: GPU submission latency
//    - Sort by "% Time" column to find hot spots
//
// COMMON FINDINGS:
// ┌─────────────────────────────────────────┐
// │ Issue              │ Solution           │
// ├────────────────────┼────────────────────┤
// │ High Acquire time  │ Vsync/mailbox tuning│
// │ High Submit time   │ Command buffer size │
// │ Variable frame     │ GPU sync/throttle  │
// │ High CPU overhead  │ Batch submissions  │
// └────────────────────┴────────────────────┘
