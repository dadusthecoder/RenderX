# RenderX

RenderX is a cross‑API Rendering Hardware Interface (RHI) written in modern C++. It provides a clean, engine‑friendly abstraction over multiple graphics backends so engines can target a single API surface while using OpenGL or Vulkan implementations under the hood.

**Current status**
- ✅ OpenGL backend — primary and stable implementation used by examples.
- 🚧 Vulkan backend — work in progress; experimental and improving.

**Planned features (short term)**
- **Resource State Tracking** — explicit tracking of resource states to enable correct and efficient barriers when targeting APIs that require them (e.g., Vulkan).
- **Bindless resources (behind a single flag)** — bindless-style resource access will be available behind a single build/runtime toggle so engines can opt into bindless semantics with minimal integration changes.

---

## Features

- Unified RHI for common GPU concepts:
  - Devices, command buffers, render passes
  - Buffers (vertex / index / uniform)
  - Textures, samplers, shaders, pipelines, swapchains
- Clear separation between API‑agnostic interfaces and backend implementations

---

## Dependencies

Most third‑party dependencies are included as submodules or vendored in the repo:

- **GLM** – math library (`External/glm`)
- **spdlog** – logging (`External/spdlog`)
- **GLFW** – windowing / input for examples (`External/glfw`)
- **GLEW** – OpenGL function loading (`External_local/glew-2.1.0`)
- **Vulkan SDK** – required when building or running the Vulkan backend
- **ProLog** – helper logging library used by examples (`External_local/ProLog`)

Make sure you have a C++17 compiler and CMake (>= 3.10) installed.

---

## Getting started (as a library consumer)

Most users will embed RenderX in their own engine / application and link against it.

1. Add this repository as a submodule or clone it next to your project and update submodules:

```bash
git submodule add https://github.com/dadusthecoder/RenderX.git external/RenderX
cd external/RenderX
git submodule update --init --recursive
```

2. Link RenderX from `CMakeLists.txt` of your project:

```cmake
add_subdirectory(external/RenderX)

add_executable(MyApp src/main.cpp)

target_link_libraries(MyApp PRIVATE RenderX)

target_include_directories(MyApp PRIVATE
    ${CMAKE_SOURCE_DIR}/external/RenderX/Include
)
```

3. Use the RHI in your code via the engine-agnostic API (examples live in `Test/HelloTriangle`).

---

## Building RenderX standalone

```bash
git clone https://github.com/dadusthecoder/RenderX.git
cd RenderX
git submodule update --init --recursive

mkdir Build
cd Build
cmake ..
cmake --build . --config Release
```

This builds the `RenderX` library and the example `HelloTriangle` application under `Build/`.

> Note: The Vulkan backend sources are included in the build; ensure the Vulkan SDK is installed and on your PATH when configuring if you plan to enable or test the Vulkan backend.

---

## Backend selection and toggles

By default the repository builds both backends. OpenGL is the stable, recommended backend today; Vulkan is experimental. Bindless resource access will be provided behind a single toggle (build-time or runtime) so integrators can opt-in without invasive code changes. See project CMake options or platform-specific init code for the exact toggle name used in your checkout.

---

## Development notes

- C++ standard: C++17 (see `CMakeLists.txt`).
- Build outputs are organized under `Build/bin/<Config>/` and intermediate files under `Build/bin-int/<Config>/`.
- The repository currently compiles both OpenGL and Vulkan backends; backend selection at runtime/integration is handled by the engine's initialization path.

---

## License

This project is licensed under the MIT License. See `LICENSE` for details.

---

## Acknowledgements

RenderX is inspired by many open-source renderers and learning resources. Thanks to the community for tooling and examples that helped shape this project.
