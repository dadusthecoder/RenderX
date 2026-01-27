# RenderX

A cross‑API **Render Hardware Interface (RHI)** written in modern C++ that provides a clean, engine‑friendly abstraction over multiple graphics backends.

Current status:
- ✅ OpenGL backend (primary, stable)
- 🚧 Vulkan backend (work in progress)

---

## Features

- Unified RHI for common GPU concepts:
  - Devices, command buffers, render passes
  - Buffers (vertex / index / uniform)
  - Textures, samplers, shaders, pipelines, swapchains
- Clear separation between API‑agnostic interfaces and backend implementations
- RAII‑managed GPU resources
- Uses **GLM** for math and **spdlog** for logging

---

## Repository layout

- `src/RenderX/` – core RHI interfaces and common code
- `src/Backend/OpenGL/` – OpenGL implementation of the RHI
- `src/Backend/Vulkan/` – Vulkan implementation (WIP)
- `Include/` – public headers for consumers of the RenderX library
- `External/` – third‑party libraries as git submodules
- `External_local/` – locally vendored libraries (e.g. GLEW, ProLog)
- `Test/HelloTriangle/` – example / test application using RenderX

---

## Dependencies

Most third‑party dependencies are included as submodules or vendored in the repo:

- **GLM** – math library (`External/glm`)
- **spdlog** – logging (`External/spdlog`)
- **GLFW** – windowing / input for examples (`External/glfw`)
- **GLEW** – OpenGL function loading (`External_local/glew-2.1.0`)
- **Vulkan SDK** – required for building Vulkan backend
- **ProLog** – logging helper library used by the examples (`External_local/ProLog`)

Make sure you have a C++17 compiler and CMake (>= 3.10) installed.

---

## Getting started (as a library consumer)

Most users will want to *embed* RenderX in their own engine / application and link against it.

### 1. Add RenderX to your project

Add this repository as a submodule (recommended) or clone it next to your project:

```bash
# from your project root
git submodule add https://github.com/dadusthecoder/RenderX.git external/RenderX
cd external/RenderX
git submodule update --init --recursive   # pulls third‑party deps
```

If you do not use git submodules, you can also clone directly:

```bash
git clone https://github.com/dadusthecoder/RenderX.git
cd RenderX
git submodule update --init --recursive
```

Locally vendored libraries under `External_local/` are already included in the repo.

### 2. Link RenderX from CMake

In your own `CMakeLists.txt`, add RenderX and link your target against it:

```cmake
add_subdirectory(external/RenderX)

add_executable(MyApp src/main.cpp)

target_link_libraries(MyApp PRIVATE RenderX)

target_include_directories(MyApp PRIVATE
    ${CMAKE_SOURCE_DIR}/external/RenderX/Include
)
```

This will build the RenderX shared library as part of your project and make its headers available.

### 3. Use the RHI in your code

A minimal example of creating a vertex buffer through the RHI:

```cpp
#include <RenderX/RenderX.h>

std::vector<Vertex> vertices = /* ... */;

Ref<Buffer> vbo = RenderX::CreateVertexBuffer(
    vertices.data(),
    vertices.size() * sizeof(Vertex)
);
```

The call above maps to the appropriate backend implementation (OpenGL or Vulkan) without changing your engine‑side code.

---

## Building RenderX standalone

If you want to build RenderX and its example application by itself:

```bash
git clone https://github.com/dadusthecoder/RenderX.git
cd RenderX
git submodule update --init --recursive

mkdir Build
cd Build
cmake ..
cmake --build . --config Release
```

This produces:

- `RenderX` shared library in `Build/bin/<Config>/`
- `HelloTriangle` test application in `Build/test/HelloTriangle/<Config>/`

A post‑build step copies `RenderX.dll` next to the `HelloTriangle` executable so the test app can run directly.

> Note: CMake will automatically look for a Vulkan SDK installation because the Vulkan backend sources are part of the build. Ensure the Vulkan SDK is installed and on your path when configuring.

To run the example on a typical setup:

```bash
cd Build/test/HelloTriangle/Release
./HelloTriangle   # or HelloTriangle.exe on Windows
```

You should see a simple triangle rendered using the RenderX RHI.

---

## Backend selection

By default both OpenGL and Vulkan backends are built; which one is used is controlled in code (e.g., via your renderer initialization path). A more explicit CMake‑time toggle may be added later; for now, consumers should assume OpenGL is the stable backend and Vulkan is experimental.

---

## Development notes

- C++ standard: C++17 (see `CMakeLists.txt`)
- Output directories are configured so that binaries are placed under `Build/bin/<Config>` and intermediate objects under `Build/bin-int/<Config>`
- The build currently always compiles both OpenGL and Vulkan backends; backend selection is handled in code.

---

## License

This project is licensed under the **MIT License**. See `LICENSE` for full details.

---

## Acknowledgements

RenderX is inspired by and/or makes use of ideas from:

- bgfx
- Granite
- Hazel Engine RHI
- Mini‑engine style renderers
- The wider OpenGL and Vulkan learning communities

Thanks to all open‑source graphics developers for their work and resources.
