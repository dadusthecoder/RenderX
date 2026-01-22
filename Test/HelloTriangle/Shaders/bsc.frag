#version 460 core
layout(location = 0) in vec3 fragPos;
layout(location = 0) out vec4 FragColor;
void main() {
    FragColor = vec4(fragPos , 1.0);
}