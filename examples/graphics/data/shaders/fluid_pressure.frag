#version 450
#extension GL_GOOGLE_include_directive : require

// One Jacobi pressure iteration.
// The divergence is computed inline from the velocity field (which is
// unchanged during the solve), and an inputScale folds the per-frame
// PRESSURE damping (the original's separate "clear" pass) into the first
// iteration, so the whole pressure stage is a single shader.
layout(location = 0) in vec2 vUv;
layout(set = 0, binding = 0) uniform Params {
    float sizeX; float sizeY;
    float inputScale;
    float pad0; float pad1; float pad2; float pad3; float pad4; float pad5;
    float pad6; float pad7; float pad8; float pad9; float pad10; float pad11;
    float flipY;
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

    // Divergence of the velocity field at this texel (mirror boundaries).
    vec2 vL = vUv - vec2(texel.x, 0.0);
    vec2 vR = vUv + vec2(texel.x, 0.0);
    vec2 vT = vUv + vec2(0.0, texel.y);
    vec2 vB = vUv - vec2(0.0, texel.y);

    vec2 Cv = decodeVel(texture(sampler2D(uTex1, uSamp1), suv(vUv)));

    float Lv = decodeVel(texture(sampler2D(uTex1, uSamp1), suv(vL))).x;
    float Rv = decodeVel(texture(sampler2D(uTex1, uSamp1), suv(vR))).x;
    float Tv = decodeVel(texture(sampler2D(uTex1, uSamp1), suv(vT))).y;
    float Bv = decodeVel(texture(sampler2D(uTex1, uSamp1), suv(vB))).y;

    if (vL.x < 0.0) Lv = -Cv.x;
    if (vR.x > 1.0) Rv = -Cv.x;
    if (vT.y > 1.0) Tv = -Cv.y;
    if (vB.y < 0.0) Bv = -Cv.y;

    float divergence = 0.5 * (Rv - Lv + Tv - Bv);

    // Pressure neighbours, damped by inputScale (PRESSURE on the first
    // iteration, 1 afterwards).
    float s = u.inputScale;
    float L = s * decodeScalar(texture(sampler2D(uTex0, uSamp0), suv(vUv - vec2(texel.x, 0.0))).rgb);
    float R = s * decodeScalar(texture(sampler2D(uTex0, uSamp0), suv(vUv + vec2(texel.x, 0.0))).rgb);
    float T = s * decodeScalar(texture(sampler2D(uTex0, uSamp0), suv(vUv + vec2(0.0, texel.y))).rgb);
    float B = s * decodeScalar(texture(sampler2D(uTex0, uSamp0), suv(vUv - vec2(0.0, texel.y))).rgb);

    float pressure = (L + R + B + T - divergence) * 0.25;
    fragColor = vec4(encodeScalar(pressure), 1.0);
}
