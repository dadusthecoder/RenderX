# RenderX — Cross-API Render Hardware Interface (RHI)

[![License](https://img.shields.io/badge/License-MIT-blue.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-blue)]()
[![Language](https://img.shields.io/badge/C%2B%2B-20-green)]()

RenderX is a modern, lightweight **Render Hardware Interface (RHI)** designed to abstract multiple graphics APIs under a single unified layer.

It provides a clean engine-friendly API that sits between your game/engine code and platform-specific graphics APIs like **OpenGL**, **Vulkan**, and future backends such as **DirectX12** and **Metal**.

The primary goal of RenderX is to make engine architecture API-independent while staying lightweight, modular, and easy to understand.

---

# 🚀 Features

## 🌐 Multi-Backend RHI
- ✔ **OpenGL Backend (Default / Stable)**
- 🔄 Vulkan Backend (WIP)
- 🗂 DX12 Backend (Planned)
- 🍎 Metal Backend (Planned)

## 🧩 RHI Abstractions
RenderX provides unified interfaces for:
- `Device`
- `CommandBuffer`
- `RenderPass`
- `Pipeline`
- `Buffer` (vertex, index, uniform)
- `Texture` & `Sampler`
- `Shader` (GLSL + future SPIR-V support)
- `Swapchain` (per backend)

Backend implementations live in:
/Backends/OpenGL
/Backends/Vulkan

---

# 📦 Dependencies

RenderX uses a minimal and common dependency set:

### **Required Dependencies**
| Library | Usage |
|--------|--------|
| **GLEW**| Loading Opengl functions
| **GLM** | Math (vectors, matrices, transforms) |
| **spdlog** | High-performance logging |

You can install these via:
- vcpkg  
- system package manager  
- manual build  

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
│ ├── OpenGL/ # OpenGL RHI backend using GLEW
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

# 🛠️ Building RenderX

RenderX uses **CMake** for its build system.

---

## 🔧 1. Install Dependencies

### **Windows (vcpkg recommended)**
```bash
vcpkg install glew glfw3 glm spdlog
Linux (apt example)
bash
Copy code
sudo apt install libglew-dev libglfw3-dev libglm-dev
🔨 2. Configure & Build (Default OpenGL Backend)
bash
Copy code
mkdir build
cd build

cmake .. -DCONFIG_BACKEND=OpenGL
cmake --build . --config Release
🔁 Switching Backend
Vulkan (if you have Vulkan SDK installed)
bash
Copy code
cmake .. -DCONFIG_BACKEND=Vulkan
RenderX automatically selects:

/Backends/OpenGL/*

/Backends/Vulkan/*

depending on this flag.

🧪 Running Examples
After building:

bash
Copy code
cd bin
./RenderXExample_Triangle
Current sample demos:

Triangle Example → Basic pipeline + buffer usage

Phong Lighting → Uniform buffers + shading

Model Viewer → GLM transforms + textures

Soft Body (XPBD) → Physics experiment using RenderX abstractions

🧱 Example Code (Creating a Vertex Buffer)
cpp
Copy code
Ref<Buffer> vbo = device->CreateVertexBuffer(
    vertices.data(),
    vertices.size() * sizeof(Vertex)
);
The RHI resolves this to:

GL_Buffer (OpenGL backend), or

VK_Buffer (Vulkan backend)

without changing your engine code.

📜 Logging (spdlog)
RenderX uses spdlog as its logging backend:

cpp
Copy code
RenderXLog::Init();
RX_CORE_INFO("Renderer initialized");
RX_CLIENT_WARN("This is a warning");
Output format is fully configurable.

🤝 Contributing
We welcome contributions!

Please check the CONTRIBUTING.md for:

PR workflow

Coding guidelines

Backend contribution rules

Issue reporting format

🧑‍🤝‍🧑 Community / Discord
Join the RenderX community for help, backend architecture discussions, shader debugging, and engine design chat:

👉 Discord: https://discord.gg/YOUR_INVITE

📄 License
RenderX is licensed under the MIT License.
See LICENSE for full details.

⭐ Acknowledgements
RenderX is inspired by:

BGFX

Granite

Hazel Engine RHI

Mini-engine renderers

OpenGL & Vulkan best practices

Special thanks to open-source graphics communities!
