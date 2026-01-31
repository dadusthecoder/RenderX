#version 450

layout(location = 0) in vec3 aPos;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 vertColor;

layout(set = 0, binding = 0) uniform Mvp_t {
    mat4 mvp;
}
Mvp;

void main() {
    fragPos = aPos;

    vertColor = vec3(
        (aPos.x + 1.0) * 0.5,
        (aPos.y + 1.0) * 0.5,
        0.8);

    gl_Position = Mvp.mvp * vec4(aPos, 1.0);
}
