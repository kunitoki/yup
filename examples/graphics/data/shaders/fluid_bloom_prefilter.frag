#version 450

// Bloom prefilter - keeps only the bright parts of the dye (original curve).
layout(location = 0) in vec2 vUv;
layout(set = 0, binding = 0) uniform Params {
    float threshold;
    float curve0; float curve1; float curve2;
    float pad0; float pad1; float pad2; float pad3; float pad4; float pad5;
    float pad6; float pad7; float pad8; float pad9; float pad10;
    float flipY;
} u;
layout(set = 0, binding = 1) uniform texture2D uTex0;
layout(set = 0, binding = 2) uniform sampler uSamp0;
layout(location = 0) out vec4 fragColor;

vec2 suv(vec2 uv) {
    return vec2(uv.x, mix(uv.y, 1.0 - uv.y, u.flipY));
}
void main() {
    vec3 c = texture(sampler2D(uTex0, uSamp0), suv(vUv)).rgb;
    float br = max(c.r, max(c.g, c.b));
    float rq = clamp(br - u.curve0, 0.0, u.curve1);
    rq = u.curve2 * rq * rq;
    c *= max(rq, br - u.threshold) / max(br, 0.0001);
    fragColor = vec4(c, 1.0);
}
