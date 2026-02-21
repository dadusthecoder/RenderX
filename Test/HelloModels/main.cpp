#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#endif

#include <RenderX/RenderX.h>
#include "ModelRenderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct OrbitCamera {
    float     yaw      = 45.0f;
    float     pitch    = 20.0f;
    float     distance = 10.0f;
    glm::vec3 target   = {0, 0, 0};

    double lastMouseX = 0, lastMouseY = 0;
    bool   dragging = false;

    Rx::Camera toRxCamera() const {
        float yawR   = glm::radians(yaw);
        float pitchR = glm::radians(pitch);

        glm::vec3 pos = {target.x + distance * cos(pitchR) * sin(yawR),
                         target.y + distance * sin(pitchR),
                         target.z + distance * cos(pitchR) * cos(yawR)};

        Rx::Camera cam;
        cam.position  = pos;
        cam.target    = target;
        cam.up        = {0, 1, 0};
        cam.fovY      = 60.0f;
        cam.nearPlane = 0.1f;
        cam.farPlane  = 100.0f;
        return cam;
    }

    void onMouseButton(int button, int action) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            dragging = (action == GLFW_PRESS);
    }
    void onMouseMove(double x, double y) {
        if (dragging) {
            float dx  = (float)(x - lastMouseX) * 0.3f;
            float dy  = (float)(y - lastMouseY) * 0.3f;
            yaw      += dx;
            pitch     = glm::clamp(pitch - dy, -89.0f, 89.0f);
        }
        lastMouseX = x;
        lastMouseY = y;
    }
    void onScroll(double dy) { distance = glm::clamp(distance - (float)dy * 0.3f, 0.5f, 50.0f); }
};

struct PayerCamera {

    float yaw      = 45.0f;
    float pitch    = 20.0f;
    float distance = 10.0f;

    double lastMouseX = 0, lastMouseY = 0;
    bool   dragging = false;

    void onMouseButton(int button, int action) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            dragging = (action == GLFW_PRESS);
    }

    void onMouseMove(double x, double y) {
        if (dragging) {
            float dx  = (float)(x - lastMouseX) * 0.3f;
            float dy  = (float)(y - lastMouseY) * 0.3f;
            yaw      += dx;
            pitch     = glm::clamp(pitch - dy, -89.0f, 89.0f);
        }
        lastMouseX = x;
        lastMouseY = y;
    }
    void onScroll(double dy) { distance = glm::clamp(distance - (float)dy * 0.3f, 0.5f, 50.0f); }
};

static OrbitCamera g_Camera;

static void mouseButtonCb(GLFWwindow*, int button, int action, int) {
    g_Camera.onMouseButton(button, action);
}
static void cursorPosCb(GLFWwindow*, double x, double y) {
    g_Camera.onMouseMove(x, y);
}
static void scrollCb(GLFWwindow*, double, double dy) {
    g_Camera.onScroll(dy);
}

int main(int argc, char** argv) {
    const char* modelPath = argc > 1 ? argv[1] : "Alien/Test_Alien-Animal-Blender_2.81.fbx"; // default test model
    //: "sphere2.fbx";
    //"Alien/Untitled.fbx"

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "RenderX — PBR Model Viewer", nullptr, nullptr);

    glfwSetMouseButtonCallback(window, mouseButtonCb);
    glfwSetCursorPosCallback(window, cursorPosCb);
    glfwSetScrollCallback(window, scrollCb);

    int fbW = 0, fbH = 0;
    while (fbW == 0 || fbH == 0) {
        glfwGetFramebufferSize(window, &fbW, &fbH);
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
#endif

    Rx::Init(initInfo);

    Rx::CommandQueue* graphics = Rx::GetGpuQueue(Rx::QueueType::GRAPHICS);

    auto* swapchain = Rx::CreateSwapchain(
        Rx::SwapchainDesc::Default(fbW, fbH).setFormat(Rx::Format::BGRA8_SRGB).setMaxFramesInFlight(3));

    Rx::ModelRenderer renderer;
    renderer.init(swapchain, graphics);

    int modelIdx = renderer.loadModel(modelPath);
    if (modelIdx < 0) {
        fprintf(stderr, "Failed to load model: %s\n", modelPath);
        return 1;
    }

    Rx::FlushUploads();

    glm::vec3 lightDir       = glm::normalize(glm::vec3(-1.0f, -2.0f, -1.0f));
    glm::vec3 lightColor     = {1.0f, 0.95f, 0.88f}; // warm white
    float     lightIntensity = 3.0f;

    int currentW = fbW, currentH = fbH;

    Rx::PrintHandles();
    
    int i = 0;
    while (i < 1000000000) {
        i++;
    }

    renderer.printHandles();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int newW, newH;
        glfwGetFramebufferSize(window, &newW, &newH);

        if (newW == 0 || newH == 0)
            continue; // minimized

        if (newW != currentW || newH != currentH) {
            currentW = newW;
            currentH = newH;
            renderer.resize(currentW, currentH);
        }

        float      aspect = (float)currentW / (float)currentH;
        Rx::Camera cam    = g_Camera.toRxCamera();

        renderer.render(aspect, cam, lightDir, lightColor, lightIntensity);
    }

    renderer.shutdown();
    Rx::Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
