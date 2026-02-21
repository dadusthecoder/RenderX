#include "ModelRenderer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include "Files.h"

namespace Rx {

namespace fs = std::filesystem;

static glm::mat4 toGlm(const aiMatrix4x4& m) {
    return glm::transpose(glm::make_mat4(&m.a1));
}

static glm::vec3 toGlm(const aiVector3D& v) {
    return {v.x, v.y, v.z};
}
static glm::vec2 toGlm2(const aiVector3D& v) {
    return {v.x, v.y};
}

void ModelRenderer::init(Swapchain* swapchain, CommandQueue* graphics, const RendererConfig& config) {
    m_Swapchain = swapchain;
    m_Graphics  = graphics;
    m_Config    = config;

    createSetLayouts();
    createFallbackTextures();
    createShadowResources();
    createDepthBuffer();
    createDescriptorPool();
    createPipelines();
    createFrameResources();
}

void ModelRenderer::createSetLayouts() {
    // Set 0 — per-frame data bound once at start of frame
    m_FrameSetLayout =
        Rx::CreateSetLayout(SetLayoutDesc()
                                .add(Binding::ConstantBuffer(0, PipelineStage::ALL_GRAPHICS)) // CameraUBO
                                .add(Binding::Texture(1, PipelineStage::FRAGMENT))            // shadowMap
                                .add(Binding::Sampler(2, PipelineStage::FRAGMENT))            // shadowSampler
                                .add(Binding::Sampler(3, PipelineStage::FRAGMENT))            // textureSampler
                                .setDebugName("FrameSetLayout"));

    // Set 1 — per-material, bound per mesh
    m_MaterialSetLayout =
        Rx::CreateSetLayout(SetLayoutDesc()
                                .add(Binding::Texture(0, PipelineStage::FRAGMENT))        // albedo
                                .add(Binding::Texture(1, PipelineStage::FRAGMENT))        // normal
                                .add(Binding::Texture(2, PipelineStage::FRAGMENT))        // metalRough
                                .add(Binding::Texture(3, PipelineStage::FRAGMENT))        // ao
                                .add(Binding::ConstantBuffer(4, PipelineStage::FRAGMENT)) // MaterialUBO
                                .setDebugName("MaterialSetLayout"));
}

void ModelRenderer::createPipelines() {
    auto vertexInput = VertexInputState()
                           .addBinding(VertexBinding::PerVertex(0, sizeof(Vertex)))
                           .addAttribute(VertexAttribute::Vec3(0, 0, offsetof(Vertex, position)))
                           .addAttribute(VertexAttribute::Vec3(1, 0, offsetof(Vertex, normal)))
                           .addAttribute(VertexAttribute::Vec2(2, 0, offsetof(Vertex, uv)))
                           .addAttribute(VertexAttribute::Vec4(3, 0, offsetof(Vertex, tangent)));

    // ── Forward / PBR pipeline ────────────────────────────────────────────
    SetLayoutHandle   forwardSets[] = {m_FrameSetLayout, m_MaterialSetLayout};
    PushConstantRange pushRange[]   = {PushConstantRange::Vertex(sizeof(PushConstants))};
    m_ForwardLayout                 = Rx::CreatePipelineLayout(forwardSets, 2, pushRange, 1);

    auto pbrVert = Rx::CreateShader(
        ShaderDesc::FromBytecode(PipelineStage::VERTEX, Files::ReadBinaryFile("Shaders/Bin/pbr.vert.spv")));
    auto pbrFrag = Rx::CreateShader(
        ShaderDesc::FromBytecode(PipelineStage::FRAGMENT, Files::ReadBinaryFile("Shaders/Bin/pbr.frag.spv")));

    m_ForwardPipeline = Rx::CreateGraphicsPipeline(PipelineDesc()
                                                       .setLayout(m_ForwardLayout)
                                                       .addColorFormat(m_Config.colorFormat)
                                                       .setDepthFormat(m_Config.depthFormat)
                                                       .addShader(pbrVert)
                                                       .addShader(pbrFrag)
                                                       .setVertexInput(vertexInput)
                                                       .setTopology(Topology::TRIANGLES)
                                                       .setRasterizer(RasterizerState::CullBack())
                                                       .setDepthStencil(DepthStencilState::Default())
                                                       .setBlend(BlendState::Disabled())
                                                       .setDebugName("PBR_Forward"));

    // Shadow pipeline
    SetLayoutHandle shadowSets[] = {m_FrameSetLayout};
    m_ShadowLayout               = Rx::CreatePipelineLayout(shadowSets, 1, pushRange, 1);

    auto shadowVert = Rx::CreateShader(
        ShaderDesc::FromBytecode(PipelineStage::VERTEX, Files::ReadBinaryFile("Shaders/Bin/shadow.vert.spv")));

    m_ShadowPipeline = Rx::CreateGraphicsPipeline(
        PipelineDesc()
            .setLayout(m_ShadowLayout)
            .setDepthFormat(m_Config.shadowFormat)
            .addShader(shadowVert)
            .setVertexInput(vertexInput)
            .setTopology(Topology::TRIANGLES)
            .setRasterizer(RasterizerState::CullFront()) // front-face culling reduces peter-panning
            .setDepthStencil(DepthStencilState::Default())
            .setBlend(BlendState::Disabled())
            .setDebugName("Shadow_Depth"));

    Rx::DestroyShader(pbrVert);
    Rx::DestroyShader(pbrFrag);
    Rx::DestroyShader(shadowVert);
}

void ModelRenderer::createShadowResources() {
    const uint32_t sz = m_Config.shadowMapSize;

    m_ShadowMap = Rx::CreateTexture(TextureDesc::DepthStencil(sz, sz, m_Config.shadowFormat)
                                        .setUsage(TextureUsage::DEPTH_STENCIL | TextureUsage::SAMPLED)
                                        .setDebugName("ShadowMap"));

    m_ShadowView = Rx::CreateTextureView(TextureViewDesc::Default(m_ShadowMap).setDebugName("ShadowMapView"));

    m_ShadowSampler = Rx::CreateSampler(SamplerDesc()
                                            .setFilter(Filter::LINEAR, Filter::LINEAR)
                                            .setAddressMode(AddressMode::CLAMP_TO_BORDER)
                                            .setBorderColor(BorderColor::FLOAT_OPAQUE_WHITE)
                                            .enableComparison(CompareOp::LESS_EQUAL)
                                            .setDebugName("ShadowSampler"));

    m_TextureSampler = Rx::CreateSampler(SamplerDesc()
                                             .setFilter(Filter::LINEAR, Filter::LINEAR)
                                             .setMipFilter(Filter::LINEAR)
                                             .setAddressMode(AddressMode::REPEAT)
                                             .setMaxAnisotropy(16.0f)
                                             .setDebugName("TextureSampler"));
}

void ModelRenderer::createDepthBuffer() {
    m_DepthBuffer = Rx::CreateTexture(
        TextureDesc::DepthStencil(m_Swapchain->GetWidth(), m_Swapchain->GetHeight(), m_Config.depthFormat)
            .setDebugName("SceneDepth"));

    m_DepthView = Rx::CreateTextureView(TextureViewDesc::Default(m_DepthBuffer).setDebugName("SceneDepthView"));
}

static TextureHandle createSolidTexture(uint8_t r, uint8_t g, uint8_t b, uint8_t a, const char* name) {
    uint8_t pixels[4] = {r, g, b, a};

    auto tex = Rx::CreateTexture(TextureDesc::Texture2D(1, 1, Format::RGBA8_UNORM)
                                     .setUsage(TextureUsage::SAMPLED | TextureUsage::TRANSFER_DST)
                                     .setInitialData(pixels, sizeof(pixels))
                                     .setDebugName(name));
    return tex;
}

void ModelRenderer::createFallbackTextures() {
    // White — used as fallback albedo and AO
    m_FallbackWhite = createSolidTexture(255, 255, 255, 255, "FallbackWhite");
    // Normal map blue — (0.5, 0.5, 1.0) in [0,255] = (128, 128, 255)
    m_FallbackNormal = createSolidTexture(128, 128, 255, 255, "FallbackNormal");
    // Black — metallic=0, roughness=0 (packed as R=metallic, G=roughness)
    // We want non-metal (0) medium-rough (0.5), so G=128
    m_FallbackBlack = createSolidTexture(0, 128, 0, 255, "FallbackMetalRough");

    m_FallbackWhiteView  = Rx::CreateTextureView(TextureViewDesc::Default(m_FallbackWhite));
    m_FallbackNormalView = Rx::CreateTextureView(TextureViewDesc::Default(m_FallbackNormal));
    m_FallbackBlackView  = Rx::CreateTextureView(TextureViewDesc::Default(m_FallbackBlack));
}

void ModelRenderer::createDescriptorPool() {
    m_DescriptorPool =
        Rx::CreateDescriptorPool(DescriptorPoolDesc::Persistent(SetLayoutHandle(0), 1000).setDebugName("RendererPool"));
}

void ModelRenderer::createFrameResources() {
    m_Frames.resize(m_Config.framesInFlight);

    for (auto& frame : m_Frames) {
        frame.graphicsAlloc = m_Graphics->CreateCommandAllocator();
        frame.graphicsCmd   = frame.graphicsAlloc->Allocate();

        // Camera UBO — one per frame so we don't stomp in-flight data
        frame.cameraBuffer = Rx::CreateBuffer(
            BufferDesc::UniformBuffer(sizeof(CameraUBO), MemoryType::CPU_TO_GPU).setDebugName("CameraUBO"));
        frame.cameraMapped = Rx::MapBuffer(frame.cameraBuffer);
        frame.cameraView   = Rx::CreateBufferView(BufferViewDesc::WholeBuffer(frame.cameraBuffer));

        // Frame descriptor set (set 0)
        frame.frameSet = Rx::AllocateSet(m_DescriptorPool, m_FrameSetLayout);

        DescriptorWrite writes[] = {
            DescriptorWrite::CBV(0, frame.cameraView),
            DescriptorWrite::Texture(1, m_ShadowView),
            DescriptorWrite::Sampler(2, m_ShadowSampler),
            DescriptorWrite::Sampler(3, m_TextureSampler),
        };
        Rx::WriteSet(frame.frameSet, writes, 4);
    }
}

int ModelRenderer::loadModel(const std::string& path) {
    Assimp::Importer importer;

    const aiScene* scene =
        importer.ReadFile(path,
                          aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
                              aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality |
                              aiProcess_GlobalScale | aiProcess_PreTransformVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        RENDERX_ERROR("ModelRenderer::loadModel: assimp failed to load '{}': {}", path, importer.GetErrorString());
        return -1;
    }

    Model model;
    model.path   = path;
    fs::path dir = fs::path(path).parent_path();

    // Load materials
    model.materials.resize(scene->mNumMaterials);

    for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
        const aiMaterial* aiMat = scene->mMaterials[i];
        Material&         mat   = model.materials[i];

        aiString matName;
        aiMat->Get(AI_MATKEY_NAME, matName);
        mat.name = matName.C_Str();

        // Helper to load a texture by type, fall back to handle if missing
        auto tryLoad = [&](aiTextureType type, bool srgb) -> TextureHandle {
            aiString texPath;
            if (aiMat->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
                std::string fullPath = (dir / texPath.C_Str()).string();
                return loadTexture(fullPath, srgb);
            }
            return TextureHandle{};
        };

        mat.albedoTex     = tryLoad(aiTextureType_DIFFUSE, true);            // sRGB
        mat.normalTex     = tryLoad(aiTextureType_NORMALS, false);           // linear
        mat.metalRoughTex = tryLoad(aiTextureType_METALNESS, false);         // linear
        mat.aoTex         = tryLoad(aiTextureType_AMBIENT_OCCLUSION, false); // linear

        // Create views — use fallback if texture missing
        auto makeView = [&](TextureHandle tex, TextureViewHandle fallback) -> TextureViewHandle {
            if (tex.isValid())
                return Rx::CreateTextureView(TextureViewDesc::Default(tex));
            return fallback;
        };

        mat.albedoView     = makeView(mat.albedoTex, m_FallbackWhiteView);
        mat.normalView     = makeView(mat.normalTex, m_FallbackNormalView);
        mat.metalRoughView = makeView(mat.metalRoughTex, m_FallbackBlackView);
        mat.aoView         = makeView(mat.aoTex, m_FallbackWhiteView);

        // Material params
        mat.params.flags.hasAlbedo     = mat.albedoTex.isValid() ? 1 : 0;
        mat.params.flags.hasNormal     = mat.normalTex.isValid() ? 1 : 0;
        mat.params.flags.hasMetalRough = mat.metalRoughTex.isValid() ? 1 : 0;
        mat.params.flags.hasAO         = mat.aoTex.isValid() ? 1 : 0;
        mat.params.normalStrength      = 1.0f;
        mat.params.aoStrength          = 1.0f;

        // Base color
        aiColor4D baseColor(1, 1, 1, 1);
        aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);
        mat.params.baseColor = {baseColor.r, baseColor.g, baseColor.b, baseColor.a};

        // Metallic / roughness
        float metallic = 0.0f, roughness = 0.5f;
        aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
        aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
        mat.params.metallic  = metallic;
        mat.params.roughness = roughness;

        // Upload MaterialUBO and build per-material descriptor set
        buildMaterialSet(mat);
    }

    // ── Load meshes (walk node tree) ──────────────────────────────────────
    std::function<void(aiNode*)> processNode = [&](aiNode* node) {
        glm::mat4 nodeTransform = toGlm(node->mTransformation);
        (void)nodeTransform; // we use per-instance push constants for the model matrix

        for (uint32_t m = 0; m < node->mNumMeshes; m++) {
            const aiMesh* aiM = scene->mMeshes[node->mMeshes[m]];

            std::vector<Vertex>   vertices;
            std::vector<uint32_t> indices;
            vertices.reserve(aiM->mNumVertices);

            for (uint32_t v = 0; v < aiM->mNumVertices; v++) {
                Vertex vert;
                vert.position = toGlm(aiM->mVertices[v]);
                vert.normal   = aiM->HasNormals() ? toGlm(aiM->mNormals[v]) : glm::vec3(0, 1, 0);
                vert.uv       = aiM->HasTextureCoords(0) ? toGlm2(aiM->mTextureCoords[0][v]) : glm::vec2(0);
                if (aiM->HasTangentsAndBitangents()) {
                    glm::vec3 T    = toGlm(aiM->mTangents[v]);
                    glm::vec3 B    = toGlm(aiM->mBitangents[v]);
                    glm::vec3 N    = vert.normal;
                    float     sign = glm::dot(glm::cross(N, T), B) < 0.0f ? -1.0f : 1.0f;
                    vert.tangent   = glm::vec4(T, sign);
                } else {
                    vert.tangent = glm::vec4(1, 0, 0, 1);
                }
                vertices.push_back(vert);
            }

            for (uint32_t f = 0; f < aiM->mNumFaces; f++) {
                const aiFace& face = aiM->mFaces[f];
                for (uint32_t fi = 0; fi < face.mNumIndices; fi++)
                    indices.push_back(face.mIndices[fi]);
            }

            Mesh mesh;
            mesh.vertexBuffer  = uploadBuffer(vertices.data(), vertices.size() * sizeof(Vertex), BufferFlags::VERTEX);
            mesh.indexBuffer   = uploadBuffer(indices.data(), indices.size() * sizeof(uint32_t), BufferFlags::INDEX);
            mesh.indexCount    = static_cast<uint32_t>(indices.size());
            mesh.materialIndex = aiM->mMaterialIndex;

            model.meshes.push_back(mesh);
        }

        for (uint32_t c = 0; c < node->mNumChildren; c++)
            processNode(node->mChildren[c]);
    };

    processNode(scene->mRootNode);

    m_Models.push_back(std::move(model));
    return static_cast<int>(m_Models.size() - 1);
}

TextureHandle ModelRenderer::loadTexture(const std::string& path, bool srgb) {
    int      w, h, channels;
    stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!pixels) {
        RENDERX_WARN("ModelRenderer::loadTexture: failed to load '{}', using fallback", path);
        return TextureHandle{};
    }

    Format fmt      = srgb ? Format::RGBA8_SRGB : Format::RGBA8_UNORM;
    size_t dataSize = static_cast<size_t>(w) * h * 4;

    auto tex = Rx::CreateTexture(TextureDesc::Texture2D(w, h, fmt)
                                     .setUsage(TextureUsage::SAMPLED | TextureUsage::TRANSFER_DST)
                                     //.setGeneratedMips()
                                     .setInitialData(pixels, (uint32_t)dataSize)
                                     .setDebugName(fs::path(path).filename().string().c_str()));

    stbi_image_free(pixels);
    return tex;
}

BufferHandle ModelRenderer::uploadBuffer(const void* data, size_t size, BufferFlags usage) {
    BufferDesc desc = (usage == BufferFlags::VERTEX) ? BufferDesc::VertexBuffer(size, MemoryType::GPU_ONLY, data)
                                                     : BufferDesc::IndexBuffer(size, MemoryType::GPU_ONLY, data);

    return Rx::CreateBuffer(desc);
}

void ModelRenderer::buildMaterialSet(Material& mat) {
    // Upload MaterialUBO
    auto matBuffer =
        Rx::CreateBuffer(BufferDesc::UniformBuffer(sizeof(MaterialUBO), MemoryType::CPU_TO_GPU, &mat.params));
    auto matView = Rx::CreateBufferView(BufferViewDesc::WholeBuffer(matBuffer));

    mat.descriptorSet = Rx::AllocateSet(m_DescriptorPool, m_MaterialSetLayout);

    DescriptorWrite writes[] = {
        DescriptorWrite::Texture(0, mat.albedoView),
        DescriptorWrite::Texture(1, mat.normalView),
        DescriptorWrite::Texture(2, mat.metalRoughView),
        DescriptorWrite::Texture(3, mat.aoView),
        DescriptorWrite::CBV(4, matView),
    };
    Rx::WriteSet(mat.descriptorSet, writes, 5);
}

void ModelRenderer::render(
    float aspect, const Camera& camera, glm::vec3 lightDir, glm::vec3 lightColor, float lightIntensity) {
    auto& frame = m_Frames[m_FrameIndex];

    // Wait for this frame slot to finish its previous GPU work
    m_Graphics->Wait(frame.T);

    uint32_t imageIndex = m_Swapchain->AcquireNextImage();

    frame.graphicsAlloc->Reset();
    frame.graphicsCmd->open();

    updateFrameData(frame, aspect, camera, lightDir, lightColor, lightIntensity);
    shadowPass(frame.graphicsCmd, frame);
    forwardPass(frame.graphicsCmd, frame, imageIndex, aspect);

    frame.graphicsCmd->close();

    frame.T = m_Graphics->Submit(SubmitInfo::Single(frame.graphicsCmd).setSwapchainWrite());

    m_Swapchain->Present(imageIndex);

    m_FrameIndex = (m_FrameIndex + 1) % m_Config.framesInFlight;
}

void ModelRenderer::updateFrameData(
    RendererFrame& frame, float aspect, const Camera& cam, glm::vec3 lightDir, glm::vec3 lightColor, float intensity) {
    m_LightSpaceMatrix = computeLightSpaceMatrix(glm::normalize(lightDir));

    CameraUBO ubo;
    ubo.view             = cam.getView();
    ubo.proj             = cam.getProj(aspect);
    ubo.lightSpaceMatrix = m_LightSpaceMatrix;
    ubo.cameraPos        = glm::vec4(cam.position, 0.0f);
    ubo.lightDir         = glm::vec4(glm::normalize(lightDir), 0.0f);
    ubo.lightColor       = glm::vec4(lightColor * intensity, intensity);

    memcpy(frame.cameraMapped, &ubo, sizeof(CameraUBO));
}

glm::mat4 ModelRenderer::computeLightSpaceMatrix(glm::vec3 lightDir) const {
    // Orthographic projection for a directional light
    // Covers a rough scene bounds — tune these for your scene
    float     range      = 20.0f;
    glm::mat4 lightProj  = glm::ortho(-range, range, -range, range, -range * 2.0f, range * 2.0f);
    lightProj[1][1]     *= -1.0f;

    glm::mat4 lightView = glm::lookAt(lightDir * range, // position — far along light direction
                                      glm::vec3(0),     // look at scene center
                                      glm::vec3(0, 1, 0));

    return lightProj * lightView;
}

static bool isFirstTransitionDpth = true;
void        ModelRenderer::shadowPass(CommandList* cmd, RendererFrame& frame) {
    // Transition shadow map -> depth attachment
    if (isFirstTransitionDpth) {
        TextureBarrier tb = TextureBarrier::UndefinedToDepthStencil(m_ShadowMap);
        cmd->Barrier(nullptr, 0, nullptr, 0, &tb, 1);
        isFirstTransitionDpth = false;
    } else {
        TextureBarrier tb = TextureBarrier::ShaderReadToDepthStencil(m_ShadowMap);
        cmd->Barrier(nullptr, 0, nullptr, 0, &tb, 1);
    }

    const uint32_t sz = m_Config.shadowMapSize;
    cmd->beginRendering(RenderingDesc(sz, sz).setDepthStencil(DepthStencilAttachmentDesc::Clear(m_ShadowView)));

    cmd->setPipeline(m_ShadowPipeline);
    cmd->setViewport(0, 0, (int)sz, (int)sz);
    cmd->setScissor(0, 0, sz, sz);

    // Set 0 — frame data (camera UBO has lightSpaceMatrix)
    cmd->setDescriptorSet(0, frame.frameSet);

    drawMeshes(cmd, true);

    cmd->endRendering();

    // Transition shadow map -> depth attachment
    {
        TextureBarrier tb = TextureBarrier::DepthStencilToShaderRead(m_ShadowMap);
        cmd->Barrier(nullptr, 0, nullptr, 0, &tb, 1);
    }
}

static bool isFirstTransitionForaward = true;
void        ModelRenderer::forwardPass(CommandList* cmd, RendererFrame& frame, uint32_t imageIndex, float aspect) {
    auto     swapTex = m_Swapchain->GetImage(imageIndex);
    uint32_t w       = m_Swapchain->GetWidth();
    uint32_t h       = m_Swapchain->GetHeight();

    TextureBarrier barriers[2] = {
        TextureBarrier::UndefinedToColorAttachment(swapTex),
        TextureBarrier::UndefinedToDepthStencil(m_DepthBuffer),
    };
    cmd->Barrier(nullptr, 0, nullptr, 0, barriers, 2);

    cmd->beginRendering(
        RenderingDesc(w, h)
            .addColorAttachment(AttachmentDesc::Clear(m_Swapchain->GetImageView(imageIndex), ClearColor::Black()))
            .setDepthStencil(DepthStencilAttachmentDesc::Clear(m_DepthView)));

    cmd->setPipeline(m_ForwardPipeline);
    cmd->setViewport(0, 0, (int)w, (int)h);
    cmd->setScissor(0, 0, w, h);

    // Set 0 — frame data (camera, shadow map, samplers) — bound once
    cmd->setDescriptorSet(0, frame.frameSet);

    drawMeshes(cmd, false);

    cmd->endRendering();

    // Transition swapchain -> present
    {
        TextureBarrier tb = TextureBarrier::ColorAttachmentToPresent(swapTex);
        cmd->Barrier(nullptr, 0, nullptr, 0, &tb, 1);
    }
}

void ModelRenderer::drawMeshes(CommandList* cmd, bool isShadowPass) {
    for (auto& model : m_Models) {
        for (auto& mesh : model.meshes) {
            auto& mat = model.materials[mesh.materialIndex];

            // Model matrix — identity for now, expose transform per model later
            PushConstants pc;
            pc.model        = glm::mat4(1.0f);
            pc.normalMatrix = glm::transpose(glm::inverse(pc.model));
            cmd->pushConstants(0, &pc, sizeof(PushConstants));

            if (!isShadowPass) {
                // Set 1 — per-material textures + material UBO
                cmd->setDescriptorSet(1, mat.descriptorSet);
            }

            cmd->setVertexBuffer(mesh.vertexBuffer);
            cmd->setIndexBuffer(mesh.indexBuffer);
            cmd->drawIndexed(mesh.indexCount);
        }
    }
}

void ModelRenderer::printHandles() {
    printf("ShadowMap id: %u \n", (uint32_t)m_ShadowMap.id);
    printf("depthBuffer id: %u\n", (uint32_t)m_DepthBuffer.id);
    printf("ShadowMapview  id: %u\n", (uint32_t)m_ShadowView.id);
    printf("depthView id: %u\n", (uint32_t)m_DepthView.id);
}

void ModelRenderer::resize(uint32_t width, uint32_t height) {
    m_Swapchain->Resize(width, height);
    Rx::DestroyTextureView(m_DepthView);
    Rx::DestroyTexture(m_DepthBuffer);
    createDepthBuffer();
}

void ModelRenderer::unloadModel(int index) {
    if (index < 0 || index >= (int)m_Models.size())
        return;

    m_Graphics->WaitIdle();

    auto& model = m_Models[index];
    for (auto& mesh : model.meshes) {
        Rx::DestroyBuffer(mesh.vertexBuffer);
        Rx::DestroyBuffer(mesh.indexBuffer);
    }
    for (auto& mat : model.materials) {
        Rx::FreeSet(m_DescriptorPool, mat.descriptorSet);
        if (mat.albedoTex.isValid()) {
            Rx::DestroyTextureView(mat.albedoView);
            Rx::DestroyTexture(mat.albedoTex);
        }
        if (mat.normalTex.isValid()) {
            Rx::DestroyTextureView(mat.normalView);
            Rx::DestroyTexture(mat.normalTex);
        }
        if (mat.metalRoughTex.isValid()) {
            Rx::DestroyTextureView(mat.metalRoughView);
            Rx::DestroyTexture(mat.metalRoughTex);
        }
        if (mat.aoTex.isValid()) {
            Rx::DestroyTextureView(mat.aoView);
            Rx::DestroyTexture(mat.aoTex);
        }
    }

    m_Models.erase(m_Models.begin() + index);
}

void ModelRenderer::shutdown() {
    if (!m_Graphics)
        return;
    m_Graphics->WaitIdle();

    for (int i = (int)m_Models.size() - 1; i >= 0; i--)
        unloadModel(i);

    for (auto& frame : m_Frames) {
        Rx::FreeSet(m_DescriptorPool, frame.frameSet);
        Rx::DestroyBufferView(frame.cameraView);
        Rx::DestroyBuffer(frame.cameraBuffer);
        frame.graphicsAlloc->Free(frame.graphicsCmd);
        m_Graphics->DestroyCommandAllocator(frame.graphicsAlloc);
    }
    m_Frames.clear();

    Rx::DestroyDescriptorPool(m_DescriptorPool);
    Rx::DestroyTextureView(m_DepthView);
    Rx::DestroyTexture(m_DepthBuffer);
    Rx::DestroyTextureView(m_ShadowView);
    Rx::DestroyTexture(m_ShadowMap);
    Rx::DestroyTextureView(m_FallbackWhiteView);
    Rx::DestroyTextureView(m_FallbackNormalView);
    Rx::DestroyTextureView(m_FallbackBlackView);
    Rx::DestroyTexture(m_FallbackWhite);
    Rx::DestroyTexture(m_FallbackNormal);
    Rx::DestroyTexture(m_FallbackBlack);
    Rx::DestroySampler(m_ShadowSampler);
    Rx::DestroySampler(m_TextureSampler);
    Rx::DestroyPipeline(m_ForwardPipeline);
    Rx::DestroyPipeline(m_ShadowPipeline);
    Rx::DestroyPipelineLayout(m_ForwardLayout);
    Rx::DestroyPipelineLayout(m_ShadowLayout);
    Rx::DestroySetLayout(m_FrameSetLayout);
    Rx::DestroySetLayout(m_MaterialSetLayout);

    m_Graphics = nullptr;
}

} // namespace Rx

// 