#version 450

// Dye splat: adds a radial color blob to the dye field.
layout(location = 0) in vec2 vUv;
layout(set = 0, binding = 0) uniform Params {
    float sizeX; float sizeY;
    float aspectRatio;
    float radius;
    float pointX; float pointY;
    float colorR; float colorG; float colorB;
    float pad0; float pad1; float pad2; float pad3; float pad4; float pad5;
    float flipY;
} u;
layout(set = 0, binding = 1) uniform texture2D uTex0;
layout(set = 0, binding = 2) uniform sampler uSamp0;
layout(location = 0) out vec4 fragColor;

vec2 suv(vec2 uv) {
    return vec2(uv.x, mix(uv.y, 1.0 - uv.y, u.flipY));
}
void main() {
    vec2 p = vUv - vec2(u.pointX, u.pointY);
    p.x *= u.aspectRatio;
    float falloff = exp(-dot(p, p) / max(u.radius, 0.000001));

    vec3 base = texture(sampler2D(uTex0, uSamp0), suv(vUv)).rgb;
    fragColor = vec4(base + falloff * vec3(u.colorR, u.colorG, u.colorB), 1.0);
}
