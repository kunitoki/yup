#version 450
#extension GL_GOOGLE_include_directive : require

// Vorticity confinement: sharpens swirls by adding force along the curl gradient.
layout(location = 0) in vec2 vUv;
layout(set = 0, binding = 0) uniform Params {
    float sizeX; float sizeY;
    float curlStrength;
    float dt;
    float pad0; float pad1; float pad2; float pad3; float pad4; float pad5;
    float pad6; float pad7; float pad8; float pad9; float pad10; float flipY;
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

    float L = decodeScalar(texture(sampler2D(uTex1, uSamp1), suv(vUv - vec2(texel.x, 0.0))).rgb);
    float R = decodeScalar(texture(sampler2D(uTex1, uSamp1), suv(vUv + vec2(texel.x, 0.0))).rgb);
    float T = decodeScalar(texture(sampler2D(uTex1, uSamp1), suv(vUv + vec2(0.0, texel.y))).rgb);
    float B = decodeScalar(texture(sampler2D(uTex1, uSamp1), suv(vUv - vec2(0.0, texel.y))).rgb);
    float C = decodeScalar(texture(sampler2D(uTex1, uSamp1), suv(vUv)).rgb);

    vec2 force = 0.5 * vec2(abs(T) - abs(B), abs(R) - abs(L));
    force /= length(force) + 0.0001;
    force *= u.curlStrength * C;
    force.y *= -1.0;

    vec2 velocity = decodeVel(texture(sampler2D(uTex0, uSamp0), suv(vUv)));
    velocity += force * u.dt;
    velocity = clamp(velocity, vec2(-1000.0), vec2(1000.0));
    fragColor = encodeVel(velocity);
}
