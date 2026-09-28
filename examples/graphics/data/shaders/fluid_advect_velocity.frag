#version 450
#extension GL_GOOGLE_include_directive : require

// Semi-Lagrangian advection of an encoded field (velocity self-advection).
layout(location = 0) in vec2 vUv;
layout(set = 0, binding = 0) uniform Params {
    float sizeX; float sizeY;
    float dt; float dissipation;
    float pad0; float pad1; float pad2; float pad3; float pad4; float pad5;
    float pad6; float pad7; float pad8; float pad9; float pad10; float flipY;
} u;
layout(set = 0, binding = 1) uniform texture2D uTex0;
layout(set = 0, binding = 2) uniform sampler uSamp0;
layout(location = 0) out vec4 fragColor;

#include "fluid_encode_vel.glsl"
#include "fluid_suv.glsl"

vec2 sampleEncoded(vec2 uv) {
    vec2 dims = vec2(u.sizeX, u.sizeY);
    vec2 st = uv * dims - 0.5;
    vec2 b = floor(st);
    vec2 f = st - b;
    vec2 t00 = (clamp(b, vec2(0.0), dims - 1.0) + vec2(0.5)) / dims;
    vec2 t10 = (clamp(b + vec2(1.0, 0.0), vec2(0.0), dims - 1.0) + vec2(0.5)) / dims;
    vec2 t01 = (clamp(b + vec2(0.0, 1.0), vec2(0.0), dims - 1.0) + vec2(0.5)) / dims;
    vec2 t11 = (clamp(b + vec2(1.0), vec2(0.0), dims - 1.0) + vec2(0.5)) / dims;
    vec2 v00 = decodeVel(texture(sampler2D(uTex0, uSamp0), suv(t00)));
    vec2 v10 = decodeVel(texture(sampler2D(uTex0, uSamp0), suv(t10)));
    vec2 v01 = decodeVel(texture(sampler2D(uTex0, uSamp0), suv(t01)));
    vec2 v11 = decodeVel(texture(sampler2D(uTex0, uSamp0), suv(t11)));
    return mix(mix(v00, v10, f.x), mix(v01, v11, f.x), f.y);
}
void main() {
    vec2 uv = vUv;
    vec2 vel = sampleEncoded(uv);
    vec2 coord = uv - u.dt * vel * vec2(1.0 / u.sizeX, 1.0 / u.sizeY);
    vec2 result = sampleEncoded(coord);
    result /= 1.0 + u.dissipation * u.dt;
    fragColor = encodeVel(result);
}
