// Project Astra Cosmos — shaders/triangle.vert
// Phase 0: rotating triangle. Input: vec2 pos (loc 0) + vec3 color (loc 1).
// Rotation is driven by ONE push-constant float (the angle, radians).
#version 450

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

layout(location = 0) out vec3 vColor;

layout(push_constant) uniform Push { float angle; } pc;

void main() {
  float c = cos(pc.angle);
  float s = sin(pc.angle);
  vec2 rotated = mat2(c, s, -s, c) * aPos;
  gl_Position = vec4(rotated, 0.0, 1.0);
  vColor = aColor;
}
