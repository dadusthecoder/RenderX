// #version 450

// layout(location = 0) in vec3 aPos;

// layout(location = 0) out vec3 fragPos;
// layout(location = 1) out vec3 vertColor;

// layout(set = 0, binding = 0) uniform Mvp_t {
//     mat4 mvp;
// }
// Mvp;

// void main() {
//     fragPos = aPos;

//     vertColor = vec3(
//         (aPos.x + 1.0) * 0.5,
//         (aPos.y + 1.0) * 0.5,
//         0.8);

//     gl_Position = Mvp.mvp * vec4(aPos, 1.0);
// }
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragColor   = inColor;
}
