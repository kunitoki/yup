#version 450
#extension GL_GOOGLE_include_directive : require

// Gradient subtraction: projects the velocity onto a divergence-free field.
layout(location = 0) in vec2 vUv;
layout(set = 0, binding = 0) uniform Params {
    float sizeX; float sizeY;
    float pad0; float pad1; float pad2; float pad3; float pad4; float pad5;
    float pad6; float pad7; float pad8; float pad9; float pad10; float pad11;
    float pad12; float flipY;
} u;
layout(set = 0, binding = 1) uniform texture2D uTex0;
layout(set = 0, binding = 2) uniform sampler uSamp0;
layout(set = 0, binding = 3) uniform texture2D uTex1;
layout(set = 0, binding = 4) uniform sampler uSamp1;
layout(location = 0) out vec4 fragColor;

#include "fluid_encode_vel.glsl"
#include "fluid_encode_scalar.glsl"
#include "fluid_suv.glsl"

void main() {
    vec2 texel = vec2(1.0 / u.sizeX, 1.0 / u.sizeY);

    float L = decodeScalar(texture(sampler2D(uTex0, uSamp0), suv(vUv - vec2(texel.x, 0.0))).rgb);
    float R = decodeScalar(texture(sampler2D(uTex0, uSamp0), suv(vUv + vec2(texel.x, 0.0))).rgb);
    float T = decodeScalar(texture(sampler2D(uTex0, uSamp0), suv(vUv + vec2(0.0, texel.y))).rgb);
    float B = decodeScalar(texture(sampler2D(uTex0, uSamp0), suv(vUv - vec2(0.0, texel.y))).rgb);

    vec2 velocity = decodeVel(texture(sampler2D(uTex1, uSamp1), suv(vUv)));
    velocity -= vec2(R - L, T - B);
    fragColor = encodeVel(velocity);
}
