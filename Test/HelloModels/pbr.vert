#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent; // xyz=tangent, w=bitangent sign

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec3 outTangent;
layout(location = 4) out vec3 outBitangent;
layout(location = 5) out vec4 outLightSpacePos;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec4 cameraPos;
    vec4 lightDir;
    vec4 lightColor;
}
camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 normalMatrix;
}
push;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    outWorldPos   = worldPos.xyz;

    // Transform normal and tangent to world space using the normal matrix
    // (handles non-uniform scaling correctly)
    outNormal  = normalize((push.normalMatrix * vec4(inNormal, 0.0)).xyz);
    outTangent = normalize((push.normalMatrix * vec4(inTangent.xyz, 0.0)).xyz);
    // Re-orthogonalize tangent with Gram-Schmidt, then compute bitangent
    outTangent   = normalize(outTangent - dot(outTangent, outNormal) * outNormal);
    outBitangent = cross(outNormal, outTangent) * inTangent.w;

    outUV            = inUV;
    outLightSpacePos = camera.lightSpaceMatrix * worldPos;

    gl_Position = camera.proj * camera.view * worldPos;
}
