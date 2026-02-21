#version 450

// Shadow pass — vertex only, no fragment shader needed
// Writes depth buffer from light's perspective

layout(location = 0) in vec3 inPosition;
// Other attributes declared but unused — keep binding consistent with PBR
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec4 cameraPos;
    vec4 lightDir;
    vec4 lightColor;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 normalMatrix;
} push;

void main()
{
    gl_Position = camera.lightSpaceMatrix * push.model * vec4(inPosition, 1.0);
}
