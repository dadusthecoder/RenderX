
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
    Rx::CommandList*      graphicslist;
    Rx::CommandList*      computelist;
    Rx::Timeline          T = Rx::Timeline(0);
};

float cubeVertices[] = {
    // positions          // colors
    -0.5f, -0.5f, -0.5f, 1, 0, 0, 0.5f, -0.5f, -0.5f, 0, 1, 0, 0.5f, 0.5f, -0.5f, 0, 0, 1, -0.5f, 0.5f, -0.5f, 1, 1, 0,

    -0.5f, -0.5f, 0.5f,  1, 0, 1, 0.5f, -0.5f, 0.5f,  0, 1, 1, 0.5f, 0.5f, 0.5f,  1, 1, 1, -0.5f, 0.5f, 0.5f,  0, 0, 0};

unsigned int cubeIndices[] = {
    // back
    0,
    1,
    2,
    2,
    3,
    0,
    // front
    4,
    5,
    6,
    6,
    7,
    4,
    // left
    4,
    7,
    3,
    3,
    0,
    4,
    // right
    1,
    5,
    6,
    6,
    2,
    1,
    // bottom
    4,
    5,
    1,
    1,
    0,
    4,
    // top
    3,
    2,
    6,
    6,
    7,
    3};

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "RenderX", nullptr, nullptr);

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

    initInfo.displayHandle = reinterpret_cast<void*>(glfwGetX11Display());

#else
#error Unsupported platform
#endif
    Rx::Init(initInfo);

    // Get GPU queues
    Rx::CommandQueue* graphics = Rx::GetGpuQueue(Rx::QueueType::GRAPHICS);
    Rx::CommandQueue* compute  = Rx::GetGpuQueue(Rx::QueueType::COMPUTE);

    // Create vertex buffer
    auto vertexbuffer =
        Rx::CreateBuffer(Rx::BufferDesc::VertexBuffer(sizeof(cubeVertices), Rx::MemoryType::GPU_ONLY, cubeVertices));
    auto indexbuffer =
        Rx::CreateBuffer(Rx::BufferDesc::IndexBuffer(sizeof(cubeIndices), Rx::MemoryType::GPU_ONLY, cubeIndices));

    // Create vertex input state
    auto vertexInputState = Rx::VertexInputState()
                                .addBinding(Rx::VertexBinding::PerVertex(0, sizeof(float) * 6))
                                .addAttribute(Rx::VertexAttribute::Vec3(0, 0, 0))                  // position
                                .addAttribute(Rx::VertexAttribute::Vec3(1, 0, sizeof(float) * 3)); // color

    // Create shaders
    // Note : For now the backend dose not support the corss compilation and each backend expects it's own shader lang
    // glsl(openGl) , sprv(Vulkan) ...
    auto vertexshader = Rx::CreateShader(Rx::ShaderDesc::FromBytecode(
        Rx::PipelineStage::VERTEX,
        Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.vert.spv")));

    auto fragmentshader = Rx::CreateShader(Rx::ShaderDesc::FromBytecode(
        Rx::PipelineStage::FRAGMENT,
        Files::ReadBinaryFile("D:/dev/cpp/RenderX/Test/HelloTriangle/Shaders/bsc.frag.spv")));

    // Create swapchain
    int swapWidth, swapHeight;
    glfwGetWindowSize(window, &swapWidth, &swapHeight);

    auto* swapchain = Rx::CreateSwapchain(
        Rx::SwapchainDesc::Default(swapWidth, swapHeight).setFormat(Rx::Format::BGRA8_SRGB).setMaxFramesInFlight(4));

    // Create pipeline layout
    auto pipelineLayout = Rx::CreatePipelineLayout(nullptr, 0);

    // Create graphics pipeline
    auto pipeline = Rx::CreateGraphicsPipeline(Rx::PipelineDesc()
                                                   .setLayout(pipelineLayout)
                                                   .addColorFormat(Rx::Format::BGRA8_SRGB)
                                                   .addShader(vertexshader)
                                                   .addShader(fragmentshader)
                                                   .setVertexInput(vertexInputState)
                                                   .setTopology(Rx::Topology::TRIANGLES)
                                                   .setRasterizer(Rx::RasterizerState::Default())
                                                   .setDepthStencil(Rx::DepthStencilState::NoDepth())
                                                   .setBlend(Rx::BlendState::Disabled()));

    // Setup per-frame resources
    Frame frames[3];
    for (auto& frame : frames) {
        frame.graphicsAlloc = graphics->CreateCommandAllocator();
        frame.computeAlloc  = compute->CreateCommandAllocator();
        frame.graphicslist  = frame.graphicsAlloc->Allocate();
        frame.computelist   = frame.computeAlloc->Allocate();
    }

    uint32_t currentFrame      = 0;
    uint32_t currentImageIndex = 0;
    int      currentWidth      = swapWidth;
    int      currentHeight     = swapHeight;

    while (!glfwWindowShouldClose(window)) {
        auto& frame = frames[currentFrame];

        // Handle window resize
        glfwGetWindowSize(window, &currentWidth, &currentHeight);
        if (swapchain->GetWidth() != currentWidth || swapchain->GetHeight() != currentHeight) {
            swapchain->Resize(currentWidth, currentHeight);
        }

        // Wait for this frame to complete its prevois submition
        graphics->Wait(frame.T);

        // Acquire next swapchain image
        currentImageIndex = swapchain->AcquireNextImage();

        // Record compute commands
        frame.computelist->open();
        // Compute work...
        frame.computelist->close();

        // Record graphics commands
        frame.graphicslist->open();

        // Transition swapchain image to color attachment
        {
            Rx::TextureBarrier barrier{};
            barrier.texture          = swapchain->GetImageView(currentImageIndex).id; // ⚠️ see note below
            barrier.srcStage         = Rx::PipelineStage::TOP_OF_PIPE;
            barrier.srcAccess        = Rx::AccessFlags::NONE;
            barrier.dstStage         = Rx::PipelineStage::COLOR_ATTACHMENT_OUTPUT;
            barrier.dstAccess        = Rx::AccessFlags::COLOR_ATTACHMENT_WRITE;
            barrier.oldLayout        = Rx::TextureLayout::UNDEFINED; // or PRESENT if tracking
            barrier.newLayout        = Rx::TextureLayout::COLOR_ATTACHMENT ;
            barrier.srcQueue         = 0;
            barrier.dstQueue         = 0;
            barrier.range.baseMip    = 0;
            barrier.range.mipCount   = 1;
            barrier.range.baseLayer  = 0;
            barrier.range.layerCount = 1;
            barrier.range.aspect     = Rx::TextureAspect::IMAGE_ASPECT_COLOR;

            frame.graphicslist->Barrier(nullptr, 0, nullptr, 0, &barrier, 1);
        }

        frame.graphicslist->setVertexBuffer(vertexbuffer);
        frame.graphicslist->setIndexBuffer(indexbuffer);
        frame.graphicslist->setPipeline(pipeline);

        frame.graphicslist->beginRendering(
            Rx::RenderingDesc(currentWidth, currentHeight)
                .addColorAttachment(Rx::AttachmentDesc::RenderTarget(swapchain->GetImageView(currentImageIndex))));

        frame.graphicslist->drawIndexed(36);

        frame.graphicslist->endRendering();

        // Transition to PRESENT
        {
            Rx::TextureBarrier barrier{};
            barrier.texture          = swapchain->GetImageView(currentImageIndex).id; 
            barrier.srcStage         = Rx::PipelineStage::COLOR_ATTACHMENT_OUTPUT;
            barrier.srcAccess        = Rx::AccessFlags::COLOR_ATTACHMENT_WRITE;
            barrier.dstStage         = Rx::PipelineStage::BOTTOM_OF_PIPE;
            barrier.dstAccess        = Rx::AccessFlags::NONE;
            barrier.oldLayout        = Rx::TextureLayout::COLOR_ATTACHMENT;
            barrier.newLayout        = Rx::TextureLayout::PRESENT;
            barrier.srcQueue         = 0;
            barrier.dstQueue         = 0;
            barrier.range.baseMip    = 0;
            barrier.range.mipCount   = 1;
            barrier.range.baseLayer  = 0;
            barrier.range.layerCount = 1;
            barrier.range.aspect     = Rx::TextureAspect::IMAGE_ASPECT_COLOR;
            frame.graphicslist->Barrier(nullptr, 0, nullptr, 0, &barrier, 1);
        }

        frame.graphicslist->close();

        // Submit compute work
        auto t0 = compute->Submit(frame.computelist);

        // Submit graphics work set dependency on compute and swapchain write
        frame.T = graphics->Submit(Rx::SubmitInfo::Single(frame.graphicslist)
                                       .setSwapchainWrite()
                                       .addDependency(Rx::QueueDependency(Rx::QueueType::COMPUTE, t0)));

        // Present
        swapchain->Present(currentImageIndex);

        // Advance to next frame
        currentFrame = (currentFrame + 1) % 3;
        glfwPollEvents();
    }

    // Cleanup
    graphics->WaitIdle();
    compute->WaitIdle();

    for (auto& frame : frames) {
        frame.computeAlloc->Free(frame.computelist);
        frame.graphicsAlloc->Free(frame.graphicslist);
        graphics->DestroyCommandAllocator(frame.graphicsAlloc);
        compute->DestroyCommandAllocator(frame.computeAlloc);
    }

    Rx::Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
