#version 450

// 4-tap blur - smooths the small bloom surface between two ping-pong buffers.
layout(location = 0) in vec2 vUv;
layout(set = 0, binding = 0) uniform Params {
    float srcSizeX; float srcSizeY;
    float pad0; float pad1; float pad2; float pad3; float pad4; float pad5;
    float pad6; float pad7; float pad8; float pad9; float pad10; float pad11;
    float pad12; float flipY;
} u;
layout(set = 0, binding = 1) uniform texture2D uTex0;
layout(set = 0, binding = 2) uniform sampler uSamp0;
layout(location = 0) out vec4 fragColor;

vec2 suv(vec2 uv) {
    return vec2(uv.x, mix(uv.y, 1.0 - uv.y, u.flipY));
}
void main() {
    vec2 texel = vec2(1.0 / u.srcSizeX, 1.0 / u.srcSizeY);

    vec4 sum = vec4(0.0);
    sum += texture(sampler2D(uTex0, uSamp0), suv(vUv - vec2(texel.x, 0.0)));
    sum += texture(sampler2D(uTex0, uSamp0), suv(vUv + vec2(texel.x, 0.0)));
    sum += texture(sampler2D(uTex0, uSamp0), suv(vUv + vec2(0.0, texel.y)));
    sum += texture(sampler2D(uTex0, uSamp0), suv(vUv - vec2(0.0, texel.y)));
    sum *= 0.25;
    fragColor = sum;
}
