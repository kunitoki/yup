#version 450

// Clears a surface to a flat color (initialisation only).
layout(location = 0) in vec2 vUv;
layout(set = 0, binding = 0) uniform Params {
    float colorR; float colorG; float colorB; float colorA;
    float pad0; float pad1; float pad2; float pad3;
    float pad4; float pad5; float pad6; float pad7;
    float pad8; float pad9; float pad10; float pad11;
} u;
layout(location = 0) out vec4 fragColor;
void main() {
    fragColor = vec4(u.colorR, u.colorG, u.colorB, u.colorA);
}
