// #version 450 core
// layout(location = 0) in vec3 fragPos;
// layout(location = 1) in vec3 vertColor;
// layout(location = 0) out vec4 FragColor;

// void main() {
//     // Coordinate-based visualization
//     float x = fragPos.x;  // ranges from -0.5 to 0.5
//     float y = fragPos.y;  // ranges from -0.5 to 0.5

//     // Map coordinates to color channels for direct visualization
//     // Red = X axis (left to right), Green = Y axis (bottom to top), Blue = magnitude
//     vec3 coordColor = vec3(
//         (x + 1.0) * 0.5,              // Red: X coordinate mapped to 0-1
//         (y + 1.0) * 0.5,              // Green: Y coordinate mapped to 0-1
//         length(fragPos.xy) * 0.8);     // Blue: distance from center);

//     // Add grid lines for coordinate visualization
//     float gridSize = 0.1;
//     float gridX = fract(x / gridSize);
//     float gridY = fract(y / gridSize);

//     // Create grid lines (fade at edges)
//     float gridLine = 0.0;
//     if (gridX < 0.05 || gridX > 0.95) gridLine += 0.3;
//     if (gridY < 0.05 || gridY > 0.95) gridLine += 0.3;

//     // Combine with interpolated vertex color
//     vec3 color = mix(vertColor, coordColor, 0.6);

//     // Add shading based on position
//     float dist = length(fragPos.xy);
//     float shading = 0.5 + 0.5 * sin(dist * 3.14159 * 2.0);
//     color = color * (0.7 + 0.3 * shading);

//     // Apply grid overlay
//     color = color + vec3(gridLine * 0.5);

//     FragColor = vec4(color * 1.2, 1.0);
// }

#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
