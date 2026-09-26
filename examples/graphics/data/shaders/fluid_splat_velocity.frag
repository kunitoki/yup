#version 450
#extension GL_GOOGLE_include_directive : require

// Velocity splat: adds a radial velocity impulse to the (encoded) velocity field.
layout(location = 0) in vec2 vUv;
layout(set = 0, binding = 0) uniform Params {
    float sizeX; float sizeY;
    float aspectRatio;
    float radius;
    float pointX; float pointY;
    float colorR; float colorG;
    float pad0; float pad1; float pad2; float pad3; float pad4; float pad5; float pad6;
    float flipY;
} u;
layout(set = 0, binding = 1) uniform texture2D uTex0;
layout(set = 0, binding = 2) uniform sampler uSamp0;
layout(location = 0) out vec4 fragColor;

#include "fluid_encode_vel.glsl"
#include "fluid_suv.glsl"

void main() {
    vec2 p = vUv - vec2(u.pointX, u.pointY);
    p.x *= u.aspectRatio;
    float falloff = exp(-dot(p, p) / max(u.radius, 0.000001));

    vec2 vel = decodeVel(texture(sampler2D(uTex0, uSamp0), suv(vUv)));
    vel += falloff * vec2(u.colorR, u.colorG);
    fragColor = encodeVel(vel);
}
