// Project Astra Cosmos — shaders/triangle.frag
// Phase 0: passthrough color. Total SPIR-V well under 10 instructions
// (a mov of the interpolated color into the first output).
#version 450

layout(location = 0) in vec3 vColor;
layout(location = 0) out vec4 fragColor;

void main() {
  fragColor = vec4(vColor, 1.0);
}
