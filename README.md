# RenderX — Cross-API Render Hardware Interface (RHI)

[![License](https://img.shields.io/badge/License-MIT-blue.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-blue)]()
[![Language](https://img.shields.io/badge/C%2B%2B-20-green)]()

RenderX is a modern, lightweight **Render Hardware Interface (RHI)** designed to unify multiple low-level graphics APIs under a clean, engine-friendly abstraction.  
It enables you to write rendering code **once**, and run it across different backends with minimal changes.

RenderX currently ships with a **stable OpenGL backend** and a **work-in-progress Vulkan backend**.

---

# 🚀 Features

### 🌐 Multi-Backend RHI
- ✔ **OpenGL Backend (Stable, uses GLEW)**  
- 🔄 **Vulkan Backend (WIP)**  
- 🗂 DX12 Backend (Planned)  
- 🍎 Metal Backend (Planned)

### 🧩 RHI Abstractions
RenderX exposes unified interfaces for:
- `Device`
- `CommandBuffer`
- `Swapchain`
- `Pipeline`
- `Buffer` (vertex, index, uniform)
- `Texture` + `Sampler`
- `Shader`  
- `RenderPass`

### 🏗 Engine-Friendly Design
- Clean separation between:
  - **RHI interfaces** (API-agnostic)
  - **Backend implementations** (OpenGL / Vulkan)
- Zero-cost, modern C++20 abstractions
- RAII-managed GPU resources
- Math powered by **GLM**
- Logging via **spdlog**

---

# 📁 Project Structure

RenderX/
│
├── RHI/ # API-agnostic interfaces
│ ├── Buffer.h
│ ├── Texture.h
│ ├── Pipeline.h
│ ├── Device.h
│ └── CommandBuffer.h
│
├── Backends/
│ ├── OpenGL/ # OpenGL backend (GLEW)
│ │ ├── GL_Buffer.cpp
│ │ ├── GL_Shader.cpp
│ │ ├── GL_Pipeline.cpp
│ │ └── GL_Device.cpp
│ └── Vulkan/ # Vulkan backend (WIP)
│
├── Core/ # Logging, utilities, platform helpers
│ ├── Log.h
│ └── Application.cpp
│
├── Examples/ # Demo applications
│ ├── Triangle/
│ ├── ModelViewer/
│ └── SoftBody_XPBD/
│
├── CMakeLists.txt
└── README.md

yaml
Copy code

---


# 📦 Dependencies

RenderX uses a lightweight and common dependency set:

| Library | Purpose |
|--------|---------|
| **GLEW** | OpenGL function loader |
| **GLM** | Math (vectors/matrices) |
| **spdlog** | Logging system |

You can install dependencies via **vcpkg**, **apt**, **pacman**, etc.

---

# 🛠️ Building RenderX

RenderX uses **CMake** for its build system.

---

## 🔧 1. Install Dependencies

### Windows (vcpkg recommended)
```bash
vcpkg install glew glfw3 glm spdlog
```

###Linux (apt example)

Copy code
```bash
sudo apt install libglew-dev libglfw3-dev libglm-dev
```

🔨 2. Configure & Build (Default: OpenGL Backend)
```bash
mkdir build
cd build
cmake .. -DCONFIG_BACKEND=OpenGL
cmake --build . --config Release
```

🔁 Switching Backend
Vulkan (requires Vulkan SDK)
```bash
cmake .. -DCONFIG_BACKEND=Vulkan
RenderX will automatically select:
```

🧪 Running Examples
After building:

```bash
cd bin
./RenderXExample_Triangle
```

Included sample demos:
Triangle Example → Basic RHI pipeline usage
Phong Lighting → Uniform buffers + shading
Model Viewer → Textures, GLM transforms
Soft Body (XPBD) → Physics experiment using RHI buffers

🧱 Example Code — Creating a Vertex Buffer
```cpp
Ref<Buffer> vbo = device->CreateVertexBuffer(
    vertices.data(),
    vertices.size() * sizeof(Vertex)
);
```
The RHI automatically resolves this to:
GL_Buffer (OpenGL backend), or
VK_Buffer (Vulkan backend)
without changing any engine-side code.

📜 Logging (spdlog)
RenderX uses spdlog for logging:

```cpp
Copy code
RenderXLog::Init();
RX_CORE_INFO("Renderer initialized");
RX_CLIENT_WARN("This is a warning");
You can fully customize formatting, time stamps, log levels, etc.
```

🤝 Contributing
We welcome contributions of all kinds — backend work, sample demos, bug fixes, and documentation improvements.
See CONTRIBUTING.md for
Contribution workflow
Coding standards
Backend architecture rules
Issue and PR templates

🧑‍🤝‍🧑 Community / Discord
Join the RenderX community for backend discussions, shader debugging, and engine design help:

👉 Discord: https://discord.gg/YOUR_INVITE
(Replace with your active invite!)

📄 License
RenderX is licensed under the MIT License.
See LICENSE for details.

⭐ Acknowledgements
RenderX is inspired by:
BGFX
Granite
Hazel Engine RHI
Mini-engine renderers
OpenGL & Vulkan learning resources

Special thanks to all open-source graphics communities!
