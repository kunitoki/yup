#version 450

// Fullscreen triangle generated from gl_VertexIndex, without vertex buffers. vUv is the
// logical (0..1)^2 coordinate of each pixel.
layout(location = 0) out vec2 vUv;
void main() {
    uint idx = gl_VertexIndex;
    vec2 pos = vec2(float((idx & 1u) << 2u) - 1.0,
                    float((idx & 2u) << 1u) - 1.0);
    vUv = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}
