#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 0) out vec3 fragPos;
void main() {
    fragPos = aPos;
    gl_Position = vec4(aPos, 1.0);
}