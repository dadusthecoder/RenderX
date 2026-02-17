# RenderX

**A Modern Explicit Rendering Hardware Interface**

RenderX is a multi-queue rendering abstraction layer for building game engines and real-time renderers. It provides explicit control over GPU synchronization, memory, and resource states.

> ⚠️ **Development Status:** This project is in active development and is not ready for production use. The OpenGL backend is currently non-functional. Only the Vulkan backend is being actively developed.

---

## Overview

RenderX abstracts graphics APIs behind a clean interface while preserving control over memory, synchronization, and resource tracking.

**Features**
- Multi-queue architecture with timeline-based synchronization
- Multiple descriptor binding models (static, dynamic, bindless, descriptor buffer)
- Per-subresource resource state tracking(optional)
- Automatic Barrier optimization with hazard detection(optional)
- Staging buffer suballocation for static initial gpu uploads (internally uses transfer queue)

**Design Principles**
- Explicit Over Implicit: No hidden state transitions. User controls synchronization.
- Multi-Queue First: Graphics, compute, and transfer queues operate independently. Timeline semaphores enable async workflows.
- Fine-Grained Tracking: Per-subresource states. Depth/stencil aspects tracked separately. Array layers handled individually.
- Performance Focused: Read-after-read barrier skip. Stage mask merging. Transient attachment detection. Descriptor pool recycling.

---

## Current Status

**✅ Working**
- Core initialization, swapchain, resource management
- Memory allocator with staging and upload pipeline
- Command recording and submission
- Static and dynamic descriptor sets
- Resource state comparison and hazard detection

**⚠️ In Progress**
- Barrier emission batching and merging
- Automatic queue ownership transfers
- Timeline-based resource retirement
- Full bindless descriptor support

**🚀 Planned**
- Depth/stencil aspect split tracking
- Per-array-layer tracking
- Stage mask per queue
- UAV overlap detection
- Read-after-read optimization
- Async compute dependency injection
- RenderGraph integration

**❌ Not Working**
- OpenGL backend — not functional, Vulkan only

---

## Backend Support

| Backend | Status |
|---------|--------|
| Vulkan | Active Development |
| OpenGL | Non-functional |
| Direct3D 12 | Planned |

---

## Dependencies

Most dependencies are included as submodules:

- **GLM** — math library (`External/glm`)
- **spdlog** — logging (`External/spdlog`)
- **GLFW** — windowing (`External/glfw`)
- **GLEW** — OpenGL loader (`External_local/glew-2.1.0`)
- **Vulkan SDK** — required for Vulkan backend
- **VMA** — Vulkan Memory Allocator (`External/VulkanMemoryAllocator`)

**Requirements:**
- C++17 compiler (MSVC, GCC, or Clang)
- CMake 3.10+
- Vulkan SDK 1.3+ (must be installed separately)

---

## Building

### Prerequisites

1. **Install Vulkan SDK**
   - Download from [LunarG](https://vulkan.lunarg.com/)
   - Ensure `VULKAN_SDK` environment variable is set
   - Verify installation: `vulkaninfo` or `vkcube`

2. **Clone with Submodules**
```bash
git clone https://github.com/dadusthecoder/RenderX.gitv
cd RenderX
git submodule update --init --recursive
```

### Windows (Visual Studio)
```bash
mkdir Build
cd Build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Or open `RenderX.sln` in Visual Studio and build from the IDE. 

### Linux
```bash
mkdir Build
cd Build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### macOS
```bash
mkdir Build
cd Build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(sysctl -n hw.ncpu)
```

### CMake Options

- `RX_BUILD_VULKAN` — Enable Vulkan backend (default: ON)
- `RX_BUILD_OPENGL` — Enable OpenGL backend (default: OFF, non-functional)
- `RX_BUILD_DLL` — Build as shared library (default: ON)

Example with custom options:
```bash
cmake .. -DRX_BUILD_VULKAN=ON -DRX_BUILD_OPENGL=OFF -DCMAKE_BUILD_TYPE=Debug
```

### Build Outputs

- **Binaries:** `Build/bin/<Config>/RenderX/`
- **Intermediate:** `Build/bin-int/<Config>/RenderX/`
- **HelloTriangle Example:** `Build/test/HelloTriangle/<Config>/`

---

## Using RenderX as a Library

### Method 1: Add as Subdirectory
```cmake
add_subdirectory(external/RenderX)

target_link_libraries(MyApp PRIVATE RenderX)

target_include_directories(MyApp PRIVATE
    ${CMAKE_SOURCE_DIR}/external/RenderX/src
)
```
### Method 2: Install and Link

After building RenderX:
```cmake
find_package(RenderX REQUIRED)
target_link_libraries(MyApp PRIVATE RenderX::RenderX)
```

---

## Quick Start
See `Test/HelloTriangle` for complete example.

---

## Troubleshooting

**Vulkan SDK not found:**
- Ensure `VULKAN_SDK` environment variable is set
- Add Vulkan SDK bin directory to PATH
- Restart terminal/IDE after installation

**Submodule errors:**
- Run `git submodule update --init --recursive`
- Check internet connection for submodule fetching

**Build errors on Linux:**
- Install required packages: `sudo apt-get install libvulkan-dev libxcb1-dev libx11-dev`

**64-bit build requirement:**
- RenderX requires 64-bit architecture. 32-bit builds are not supported.

---

## Use Cases

RenderX is designed for:
- Custom game engines
- Real-time renderers
- Multi-queue GPU experiments
- High-performance C++ graphics applications

Familiarity with modern GPU synchronization and resource barriers expected.

---

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## License

MIT License. See `LICENSE` for details.

---

Built with Vulkan and VMA. Inspired by NVRHI, Diligent Engine, and modern engine RHI patterns.