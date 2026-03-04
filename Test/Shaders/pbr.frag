#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;
layout(location = 5) in vec4 inLightSpacePos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec4 cameraPos;
    vec4 lightDir;   // world space, normalized, toward light
    vec4 lightColor; // xyz=color, w=intensity
}
camera;

layout(set = 0, binding = 1) uniform texture2D shadowMap;
layout(set = 0, binding = 2) uniform samplerShadow shadowSampler;
layout(set = 0, binding = 3) uniform sampler textureSampler;

layout(set = 1, binding = 0) uniform texture2D albedoTex;
layout(set = 1, binding = 1) uniform texture2D normalTex;
layout(set = 1, binding = 2) uniform texture2D metalRoughTex;
layout(set = 1, binding = 3) uniform texture2D aoTex;

layout(set = 1, binding = 4) uniform MaterialUBO {
    vec4  baseColor;
    float metallic;
    float roughness;
    float normalStrength;
    float aoStrength;
    uint  flags; // bit 0=hasAlbedo, 1=hasNormal, 2=hasMetalRough, 3=hasAO
    float _pad[3];
}
material;

bool hasAlbedo() {
    return (material.flags & 1u) != 0u;
}
bool hasNormal() {
    return (material.flags & 2u) != 0u;
}
bool hasMetalRough() {
    return (material.flags & 4u) != 0u;
}
bool hasAO() {
    return (material.flags & 8u) != 0u;
}

const float PI = 3.14159265358979323846;

// GGX normal distribution function
float D_GGX(float NdotH, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// Smith's geometry function (Schlick-GGX)
float G_SchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}
float G_Smith(float NdotV, float NdotL, float roughness) {
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

// Fresnel-Schlick approximation
vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Full Cook-Torrance specular
vec3 CookTorrance(vec3 N, vec3 V, vec3 L, vec3 F0, float roughness, out vec3 F_out) {
    vec3  H     = normalize(V + L);
    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);
    vec3  F = F_Schlick(HdotV, F0);
    F_out   = F;

    return (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);
}

float sampleShadow(vec4 lightSpacePos) {
    // Perspective divide
    vec3 proj = lightSpacePos.xyz / lightSpacePos.w;

    // Already in [0,1] for Vulkan NDC after the ortho projection,
    // but UV needs remapping from [-1,1] to [0,1] for XY
    vec2  uv    = proj.xy * 0.5 + 0.5;
    float depth = proj.z;

    // Outside shadow frustum — no shadow
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || depth > 1.0)
        return 1.0;

    // PCF 3x3 with comparison sampler
    // texelSize depends on shadow map resolution (passed implicitly via constants)
    float shadow    = 0.0;
    float texelSize = 1.0 / 2048.0; // matches RendererConfig::shadowMapSize

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            // sampler2DShadow comparison: returns 1.0 if depth < shadowMap, else 0.0
            shadow +=
                texture(sampler2DShadow(shadowMap, shadowSampler), vec3(uv + offset, depth - 0.001)); // 0.001 bias
        }
    }

    return shadow / 9.0;
}

void main() {
    vec4 albedo4 = hasAlbedo() ? texture(sampler2D(albedoTex, textureSampler), inUV) : material.baseColor;
    vec3 albedo  = albedo4.rgb;

    // Normal mapping
    vec3 N;
    if (hasNormal()) {
        vec3 tsNormal  = texture(sampler2D(normalTex, textureSampler), inUV).xyz;
        tsNormal       = tsNormal * 2.0 - 1.0;
        tsNormal.xy   *= material.normalStrength;
        tsNormal       = normalize(tsNormal);

        mat3 TBN = mat3(normalize(inTangent), normalize(inBitangent), normalize(inNormal));
        N        = normalize(TBN * tsNormal);
    } else {
        N = normalize(inNormal);
    }

    float metallic, roughness;
    if (hasMetalRough()) {
        vec4 mr   = texture(sampler2D(metalRoughTex, textureSampler), inUV);
        metallic  = mr.b; // glTF: B=metallic, G=roughness
        roughness = mr.g;
    } else {
        metallic  = material.metallic;
        roughness = material.roughness;
    }
    roughness = max(roughness, 0.04); // clamp to avoid specular singularity

    float ao = hasAO() ? texture(sampler2D(aoTex, textureSampler), inUV).r : 1.0;
    ao       = 1.0 + material.aoStrength * (ao - 1.0); // remap by strength

    // F0: base reflectivity — 0.04 for dielectrics, albedo for metals
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 V = normalize(camera.cameraPos.xyz - inWorldPos);
    vec3 L = normalize(camera.lightDir.xyz); // toward light, world space

    float NdotL = max(dot(N, L), 0.0);

    vec3 F;
    vec3 specular = CookTorrance(N, V, L, F0, roughness, F);

    // Energy conservation: metals have no diffuse, dielectrics lose energy to specular
    vec3 kD      = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    vec3 lightColor = camera.lightColor.rgb;

    // Shadow
    float shadow = sampleShadow(inLightSpacePos);

    vec3 Lo = (diffuse + specular) * lightColor * NdotL * shadow;

    // Simple ambient — no IBL for now, but this is where you'd plug in
    // a pre-filtered environment map
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, albedo4.a);
}
