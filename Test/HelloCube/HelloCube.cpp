#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#endif

#include "Files.h"
#include "RenderX/RenderX.h"

struct Frame {
    Rx::CommandAllocator* graphicsAlloc;
    Rx::CommandAllocator* computeAlloc;
    Rx::CommandList*      graphicsList;
    Rx::CommandList*      computeList;

    Rx::BufferHandle     uniformBuffer;
    Rx::BufferViewHandle uniformBufferView;
    void*                uniformMappedPtr = nullptr;

    // NEW: one set per frame, allocated from the shared pool
    Rx::SetHandle descriptorSet;

    Rx::Timeline t = Rx::Timeline(0);
};

float cubeVertices[] = {
    -0.5f, -0.5f, -0.5f, 1, 0, 0, 0.5f, -0.5f, -0.5f, 0, 1, 0, 0.5f, 0.5f, -0.5f, 0, 0, 1, -0.5f, 0.5f, -0.5f, 1, 1, 0,
    -0.5f, -0.5f, 0.5f,  1, 0, 1, 0.5f, -0.5f, 0.5f,  0, 1, 1, 0.5f, 0.5f, 0.5f,  1, 1, 1, -0.5f, 0.5f, 0.5f,  0, 0, 0,
};

unsigned int cubeIndices[] = {
    0, 1, 2, 2, 3, 0, // back
    4, 5, 6, 6, 7, 4, // front
    4, 7, 3, 3, 0, 4, // left
    1, 5, 6, 6, 2, 1, // right
    4, 5, 1, 1, 0, 4, // bottom
    3, 2, 6, 6, 7, 3, // top
};

struct Mvp {
    glm::mat4 mvp;
};

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "RenderX - HelloCube", nullptr, nullptr);

    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwPollEvents();
    }

    Rx::InitDesc initInfo{};
    initInfo.api                = Rx::GraphicsAPI::VULKAN;
    initInfo.instanceExtensions = glfwGetRequiredInstanceExtensions(&initInfo.extensionCount);

#if defined(_WIN32)
    initInfo.nativeWindowHandle = glfwGetWin32Window(window);
    initInfo.displayHandle      = GetModuleHandle(nullptr);
#elif defined(__linux__)
    initInfo.nativeWindowHandle = reinterpret_cast<void*>(glfwGetX11Window(window));
    initInfo.displayHandle      = reinterpret_cast<void*>(glfwGetX11Display());
#else
#error Unsupported platform
#endif

    Rx::Init(initInfo);

    Rx::CommandQueue* graphics = Rx::GetGpuQueue(Rx::QueueType::GRAPHICS);
    Rx::CommandQueue* compute  = Rx::GetGpuQueue(Rx::QueueType::COMPUTE);

    // Geometry
    auto vertexBuffer =
        Rx::CreateBuffer(Rx::BufferDesc::VertexBuffer(sizeof(cubeVertices), Rx::MemoryType::GPU_ONLY, cubeVertices));
    auto indexBuffer =
        Rx::CreateBuffer(Rx::BufferDesc::IndexBuffer(sizeof(cubeIndices), Rx::MemoryType::GPU_ONLY, cubeIndices));

    auto vertexInputState = Rx::VertexInputState()
                                .addBinding(Rx::VertexBinding::PerVertex(0, sizeof(float) * 6))
                                .addAttribute(Rx::VertexAttribute::Vec3(0, 0, 0))                  // position
                                .addAttribute(Rx::VertexAttribute::Vec3(1, 0, sizeof(float) * 3)); // color

    // Shaders
    auto vertShader = Rx::CreateShader(
        Rx::ShaderDesc::FromBytecode(Rx::PipelineStage::VERTEX, Files::ReadBinaryFile("Shaders/Bin/bsc.vert.spv")));

    auto fragShader = Rx::CreateShader(
        Rx::ShaderDesc::FromBytecode(Rx::PipelineStage::FRAGMENT, Files::ReadBinaryFile("Shaders/Bin/bsc.frag.spv")));

    // Swapchain
    int swapWidth, swapHeight;
    glfwGetWindowSize(window, &swapWidth, &swapHeight);

    auto* swapchain = Rx::CreateSwapchain(
        Rx::SwapchainDesc::Default(swapWidth, swapHeight).setFormat(Rx::Format::RGBA8_SRGB).setMaxFramesInFlight(3));

    // Descriptor layout
    // Describe what the shaders expect:
    //   slot 0 = uniform buffer  — vertex stage
    auto setLayout = Rx::CreateSetLayout(Rx::SetLayoutDesc()
                                             .add(Rx::Binding::ConstantBuffer(0, Rx::PipelineStage::VERTEX))
                                             .setDebugName("CubeSetLayout"));

    // Pipeline layout
    // Pipeline layout now takes SetLayoutHandle instead of ResourceGroupLayoutHandle
    auto pipelineLayout = Rx::CreatePipelineLayout(&setLayout, 1, nullptr, 0);

    // ---- Pipeline ---------------------------------------------------------------------------
    auto pipeline = Rx::CreateGraphicsPipeline(Rx::PipelineDesc()
                                                   .setLayout(pipelineLayout)
                                                   .addColorFormat(Rx::Format::RGBA8_SRGB)
                                                   .setDepthFormat(Rx::Format::D24_UNORM_S8_UINT)
                                                   .addShader(vertShader)
                                                   .addShader(fragShader)
                                                   .setVertexInput(vertexInputState)
                                                   .setTopology(Rx::Topology::TRIANGLES)
                                                   .setRasterizer(Rx::RasterizerState::Default())
                                                   .setDepthStencil(Rx::DepthStencilState::Default())
                                                   .setBlend(Rx::BlendState::Disabled()));

    //---- Descriptor pool -----------------------------------------------------------------------
    // LINEAR pool — one reset per frame, 3 sets total (one per frame in flight)
    // DESCRIPTOR_SETS path — classic Vulkan, works on every device
    // I only create ONE pool for all frames.
    // Each frame allocates its set from this pool at startup.
    // At the start of each frame we don't reset the pool — sets are persistent.
    // If the MVP data changes, we update the set's buffer contents, not the set itself.
    auto descriptorPool =
        Rx::CreateDescriptorPool(Rx::DescriptorPoolDesc::Persistent(setLayout, 3) // POOL policy, max 3 sets
                                     .setDebugName("CubeDescriptorPool"));

    // Perframe resources
    constexpr uint32_t FRAME_COUNT = 3;
    Frame              frames[FRAME_COUNT];

    for (auto& frame : frames) {
        // Command infrastructure
        frame.graphicsAlloc = graphics->CreateCommandAllocator();
        frame.computeAlloc  = compute->CreateCommandAllocator();
        frame.graphicsList  = frame.graphicsAlloc->Allocate();
        frame.computeList   = frame.computeAlloc->Allocate();

        // Uniform buffer  one per frame , so that app dosen't stomp on the GPU's copy
        frame.uniformBuffer = Rx::CreateBuffer(Rx::BufferDesc::UniformBuffer(sizeof(Mvp), Rx::MemoryType::CPU_TO_GPU));
        frame.uniformMappedPtr  = Rx::MapBuffer(frame.uniformBuffer);
        frame.uniformBufferView = Rx::CreateBufferView(Rx::BufferViewDesc::WholeBuffer(frame.uniformBuffer));

        // Allocate one descriptor set per frame from the shared pool
        frame.descriptorSet = Rx::AllocateSet(descriptorPool, setLayout);

        // Write the initial descriptor bindings into the set.
        // The uniform buffer handle points to frame-specific data.
        // The texture is shared — same view bound across all frames.
        Rx::DescriptorWrite writes[] = {
            Rx::DescriptorWrite::CBV(0, frame.uniformBufferView),
        };
        Rx::WriteSet(frame.descriptorSet, writes, 1);

        // @note: we never call WriteSet again for the texture since it doesn't change.
        // The uniform buffer is CPU_TO_GPU mapped — we just memcpy new data each frame.
        // No descriptor update needed because the buffer handle stays the same,
        // only the contents of the buffer change.
    }

    uint32_t currentFrame      = 0;
    uint32_t currentImageIndex = 0;
    int      currentWidth      = swapWidth;
    int      currentHeight     = swapHeight;
    int      i                 = 0;

    Mvp   mvp{};
    float angle = 0.0f;

    Rx::FlushUploads();
    Rx::PrintHandles();

    while (!glfwWindowShouldClose(window)) {
        auto& frame = frames[currentFrame];

        // Resize handling
        glfwGetWindowSize(window, &currentWidth, &currentHeight);
        if (swapchain->GetWidth() != currentWidth || swapchain->GetHeight() != currentHeight) {
            swapchain->Resize(currentWidth, currentHeight);
        }

        // Wait for this frame slot to finish its previous GPU submission
        graphics->Wait(frame.t);

        currentImageIndex = swapchain->AcquireNextImage();

        // Update MVP CPU write into persistently mapped buffer
        angle           += 0.01f;
        glm::mat4 model  = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 view =
            glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj =
            glm::perspective(glm::radians(60.0f), (float)currentWidth / (float)currentHeight, 0.1f, 100.0f);
        // proj[1][1] *= -1.0f; // Vulkan clip space Y flip

        mvp.mvp = proj * view * model;
        memcpy(frame.uniformMappedPtr, &mvp, sizeof(Mvp));
        // No WriteSet needed — the descriptor still points at the same buffer,
        // we just changed the buffer's contents via the mapped pointer.

        // Compute :empty pass for now
        frame.computeList->open();
        // Compute Work...
        frame.computeList->close();

        // Graphics
        frame.graphicsList->open();

        // Transition swapchain image → color attachment
        {
            Rx::TextureBarrier barrier{};
            barrier.texture   = swapchain->GetImage(currentImageIndex);
            barrier.srcStage  = Rx::PipelineStage::TOP_OF_PIPE;
            barrier.dstStage  = Rx::PipelineStage::COLOR_ATTACHMENT_OUTPUT;
            barrier.srcAccess = Rx::AccessFlags::NONE;
            barrier.dstAccess = Rx::AccessFlags::COLOR_ATTACHMENT_WRITE;
            barrier.oldLayout = Rx::TextureLayout::UNDEFINED;
            barrier.newLayout = Rx::TextureLayout::COLOR_ATTACHMENT;
            barrier.range     = {0, 1, 0, 1, Rx::TextureAspect::IMAGE_ASPECT_COLOR};
            frame.graphicsList->Barrier(nullptr, 0, nullptr, 0, &barrier, 1);
        }

        frame.graphicsList->setPipeline(pipeline);
        frame.graphicsList->setVertexBuffer(vertexBuffer);
        frame.graphicsList->setIndexBuffer(indexBuffer);

        // VK backend: vkCmdBindDescriptorSets(set=0, frame.descriptorSet)
        frame.graphicsList->setDescriptorSet(0, frame.descriptorSet);
        frame.graphicsList->setViewport(0, 0, (int)currentWidth, (int)currentHeight);
        frame.graphicsList->setScissor(0, 0, currentWidth, currentHeight);
        frame.graphicsList->beginRendering(
            Rx::RenderingDesc(currentWidth, currentHeight)
                .addColorAttachment(Rx::AttachmentDesc::RenderTarget(swapchain->GetImageView(currentImageIndex)))
                .setDepthStencil(Rx::DepthStencilAttachmentDesc::Clear(swapchain->GetDepthView(currentImageIndex))));

        frame.graphicsList->drawIndexed(36);
        frame.graphicsList->endRendering();

        // Transition swapchain image → present
        {
            Rx::TextureBarrier barrier{};
            barrier.texture   = swapchain->GetImage(currentImageIndex);
            barrier.srcStage  = Rx::PipelineStage::COLOR_ATTACHMENT_OUTPUT;
            barrier.srcAccess = Rx::AccessFlags::COLOR_ATTACHMENT_WRITE;
            barrier.dstStage  = Rx::PipelineStage::BOTTOM_OF_PIPE;
            barrier.dstAccess = Rx::AccessFlags::NONE;
            barrier.oldLayout = Rx::TextureLayout::COLOR_ATTACHMENT;
            barrier.newLayout = Rx::TextureLayout::PRESENT;
            barrier.range     = {0, 1, 0, 1, Rx::TextureAspect::IMAGE_ASPECT_COLOR};
            frame.graphicsList->Barrier(nullptr, 0, nullptr, 0, &barrier, 1);
        }

        frame.graphicsList->close();

        // Submit
        auto t0 = compute->Submit(frame.computeList);
        frame.t = graphics->Submit(Rx::SubmitInfo::Single(frame.graphicsList)
                                       .setSwapchainWrite()
                                       .addDependency(Rx::QueueDependency(Rx::QueueType::COMPUTE, t0)));

        swapchain->Present(currentImageIndex);

        currentFrame = (currentFrame + 1) % FRAME_COUNT;
        glfwPollEvents();
        i++;
    }

    // Cleanup
    // Wait for all GPU work to finish before destroying anything
    graphics->WaitIdle();
    compute->WaitIdle();

    for (auto& frame : frames) {
        // Free the descriptor set back to the pool
        Rx::FreeSet(descriptorPool, frame.descriptorSet);

        frame.computeAlloc->Free(frame.computeList);
        frame.graphicsAlloc->Free(frame.graphicsList);
        graphics->DestroyCommandAllocator(frame.graphicsAlloc);
        compute->DestroyCommandAllocator(frame.computeAlloc);
    }

    Rx::DestroyDescriptorPool(descriptorPool);

    Rx::Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
