#pragma once
#include <glm/glm.hpp>
#include <RenderX/RenderX.h>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

namespace Rx {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 tangent; // xyz=tangent, w=bitangent sign
};

// Material flags — tells the shader which textures are actually bound
// Backend fills this from what assimp found on disk
struct MaterialFlags {
    uint32_t hasAlbedo : 1;
    uint32_t hasNormal : 1;
    uint32_t hasMetalRough : 1;
    uint32_t hasAO : 1;
    uint32_t _pad : 28;
};

// GPU-side material constant buffer
// std140 layout
struct MaterialUBO {
    glm::vec4     baseColor; // fallback color if no albedo
    float         metallic;  // fallback if no metalRough map
    float         roughness; // fallback if no metalRough map
    float         normalStrength;
    float         aoStrength;
    MaterialFlags flags;
    float         _pad[3];
};
static_assert(sizeof(MaterialUBO) % 16 == 0);

// GPU-side per-frame data
struct CameraUBO {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 lightSpaceMatrix; // for shadow mapping
    glm::vec4 cameraPos;        // w unused
    glm::vec4 lightDir;         // w unused, world-space, points toward light
    glm::vec4 lightColor;       // w=intensity
};
static_assert(sizeof(CameraUBO) % 16 == 0);

// Per-draw push constants
struct PushConstants {
    glm::mat4 model;
    glm::mat4 normalMatrix; // transpose(inverse(model)) — avoids doing it in shader
};
static_assert(sizeof(PushConstants) <= 128); // guaranteed min push constant size

struct Material {
    // Textures — invalid handle = not present, use fallback
    TextureHandle albedoTex;
    TextureHandle normalTex;
    TextureHandle metalRoughTex; // R=metallic, G=roughness (glTF convention)
    TextureHandle aoTex;

    TextureViewHandle albedoView;
    TextureViewHandle normalView;
    TextureViewHandle metalRoughView;
    TextureViewHandle aoView;

    // Per-material descriptor set (set 1)
    SetHandle descriptorSet;

    // CPU data
    MaterialUBO params;
    std::string name;
};

struct Mesh {
    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;
    uint32_t     indexCount;
    uint32_t     materialIndex;
};

struct Model {
    std::vector<Mesh>     meshes;
    std::vector<Material> materials;
    std::string           path;
};

struct RendererFrame {
    CommandAllocator* graphicsAlloc = nullptr;
    CommandAllocator* computeAlloc  = nullptr;
    CommandList*      graphicsCmd   = nullptr;

    // Per-frame camera + light data
    BufferHandle     cameraBuffer;
    BufferViewHandle cameraView;
    void*            cameraMapped = nullptr;

    // Per-frame descriptor set (set 0)
    SetHandle frameSet;

    Timeline T = Timeline(0);
};

struct RendererConfig {
    uint32_t shadowMapSize  = 2048;
    uint32_t framesInFlight = 3;
    Format   colorFormat    = Format::BGRA8_SRGB;
    Format   depthFormat    = Format::D32_SFLOAT;
    Format   shadowFormat   = Format::D32_SFLOAT;
};

struct Camera {
    glm::vec3 position  = {0, 2, 5};
    glm::vec3 target    = {0, 0, 0};
    glm::vec3 up        = {0, 1, 0};
    float     fovY      = 60.0f;
    float     nearPlane = 0.1f;
    float     farPlane  = 100.0f;

    glm::mat4 getView() const { return glm::lookAt(position, target, up); }
    glm::mat4 getProj(float aspect) const {
        auto p   = glm::perspective(glm::radians(fovY), aspect, nearPlane, farPlane);
        p[1][1] *= -1.0f; // Vulkan Y flip
        return p;
    }
};

class ModelRenderer {
public:
    ModelRenderer() = default;
    ~ModelRenderer() { shutdown(); }

    // No copy
    ModelRenderer(const ModelRenderer&)            = delete;
    ModelRenderer& operator=(const ModelRenderer&) = delete;

    void init(Swapchain* swapchain, CommandQueue* graphics, const RendererConfig& config = {});

    // Load a model from disk using assimp
    // Returns model index, -1 on failure
    int  loadModel(const std::string& path);
    void unloadModel(int index);

    // Render all loaded models
    // Call once per frame, handles shadow pass + forward pass internally
    void render(float aspect, const Camera& camera, glm::vec3 lightDir, glm::vec3 lightColor, float lightIntensity);

    void resize(uint32_t width, uint32_t height);
    void shutdown();
    void printHandles();

private:
    // ── Init helpers ──────────────────────────────────────────────────────
    void createSetLayouts();
    void createPipelines();
    void createShadowResources();
    void createDepthBuffer();
    void createFallbackTextures();
    void createDescriptorPool();
    void createFrameResources();

    // ── Per-frame helpers ─────────────────────────────────────────────────
    void updateFrameData(RendererFrame& frame,
                         float          aspect,
                         const Camera&  cam,
                         glm::vec3      lightDir,
                         glm::vec3      lightColor,
                         float          intensity);
    void shadowPass(CommandList* cmd, RendererFrame& frame);
    void forwardPass(CommandList* cmd, RendererFrame& frame, uint32_t imageIndex, float aspect);
    void drawMeshes(CommandList* cmd, bool shadowPass);

    // ── Model loading helpers ─────────────────────────────────────────────
    TextureHandle loadTexture(const std::string& path, bool srgb);
    BufferHandle  uploadBuffer(const void* data, size_t size, BufferFlags usage);
    void          buildMaterialSet(Material& mat);

    glm::mat4 computeLightSpaceMatrix(glm::vec3 lightDir) const;

    // ── Resources ─────────────────────────────────────────────────────────
    Swapchain*     m_Swapchain = nullptr;
    CommandQueue*  m_Graphics  = nullptr;
    RendererConfig m_Config    = {};

    // Set layouts
    SetLayoutHandle m_FrameSetLayout;    // set 0
    SetLayoutHandle m_MaterialSetLayout; // set 1

    // Pipeline layouts
    PipelineLayoutHandle m_ForwardLayout;
    PipelineLayoutHandle m_ShadowLayout;

    // Pipelines
    PipelineHandle m_ForwardPipeline;
    PipelineHandle m_ShadowPipeline;

    // Shadow map
    TextureHandle     m_ShadowMap;
    TextureViewHandle m_ShadowView;
    SamplerHandle     m_ShadowSampler;  // comparison sampler for PCF
    SamplerHandle     m_TextureSampler; // regular sampler for PBR textures

    // Scene depth buffer
    TextureHandle     m_DepthBuffer;
    TextureViewHandle m_DepthView;

    // Fallback 1x1 textures (used when a material doesn't have a map)
    TextureHandle     m_FallbackWhite;  // albedo, AO
    TextureHandle     m_FallbackNormal; // normal map (0.5, 0.5, 1.0)
    TextureHandle     m_FallbackBlack;  // metalRough (non-metal, rough)
    TextureViewHandle m_FallbackWhiteView;
    TextureViewHandle m_FallbackNormalView;
    TextureViewHandle m_FallbackBlackView;

    // Descriptor pool — shared for frame sets and material sets
    DescriptorPoolHandle m_DescriptorPool;

    // Per-frame resources
    std::vector<RendererFrame> m_Frames;
    uint32_t                   m_FrameIndex = 0;

    // Loaded models
    std::vector<Model> m_Models;

    // Cached light space matrix written each frame
    glm::mat4 m_LightSpaceMatrix = glm::mat4(1.0f);
};

} // namespace Rx
