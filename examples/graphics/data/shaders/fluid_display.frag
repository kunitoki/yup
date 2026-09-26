#version 450

// Final composite: shading, bloom add (gamma'd), ordered dither.
layout(location = 0) in vec2 vUv;
layout(set = 0, binding = 0) uniform Params {
    float dyeSizeX; float dyeSizeY;
    float shadingF; float bloomF;
    float bloomIntensity;
    float backR; float backG; float backB;
    float pad0; float pad1; float pad2; float pad3; float pad4; float pad5;
    float pad6; float flipY;
} u;
layout(set = 0, binding = 1) uniform texture2D uTex0;
layout(set = 0, binding = 2) uniform sampler uSamp0;
layout(set = 0, binding = 3) uniform texture2D uTex1;
layout(set = 0, binding = 4) uniform sampler uSamp1;
layout(location = 0) out vec4 fragColor;

vec3 linearToGamma(vec3 color) {
    color = max(color, vec3(0.0));
    return max(1.055 * pow(color, vec3(0.416666667)) - 0.055, vec3(0.0));
}

const float kDither[16] = float[16](
    0.0,  8.0,  2.0,  10.0,
    12.0, 4.0,  14.0, 6.0,
    3.0,  11.0, 1.0,  9.0,
    15.0, 7.0,  13.0, 5.0
);

vec2 suv(vec2 uv) {
    return vec2(uv.x, mix(uv.y, 1.0 - uv.y, u.flipY));
}
void main() {
    vec3 c = texture(sampler2D(uTex0, uSamp0), suv(vUv)).rgb;

    if (u.shadingF > 0.5)
    {
        vec2 texel = vec2(1.0 / u.dyeSizeX, 1.0 / u.dyeSizeY);

        vec3 lc = texture(sampler2D(uTex0, uSamp0), suv(vUv - vec2(texel.x, 0.0))).rgb;
        vec3 rc = texture(sampler2D(uTex0, uSamp0), suv(vUv + vec2(texel.x, 0.0))).rgb;
        vec3 tc = texture(sampler2D(uTex0, uSamp0), suv(vUv + vec2(0.0, texel.y))).rgb;
        vec3 bc = texture(sampler2D(uTex0, uSamp0), suv(vUv - vec2(0.0, texel.y))).rgb;

        float dx = length(rc) - length(lc);
        float dy = length(tc) - length(bc);

        vec3 n = normalize(vec3(dx, dy, length(texel)));
        vec3 l = vec3(0.0, 0.0, 1.0);

        float diffuse = clamp(dot(n, l) + 0.7, 0.7, 1.0);
        c *= diffuse;
    }

    if (u.bloomF > 0.5)
    {
        // Sample the small bloom surface with hardware linear filtering: the
        // smooth upscale hides the 8-bit steps of the bloom buffer.
        vec3 bloom = texture(sampler2D(uTex1, uSamp1), suv(vUv)).rgb * u.bloomIntensity;
        bloom = linearToGamma(bloom);
        c += bloom;
    }

    float alpha = max(c.r, max(c.g, c.b));
    vec3 outC = c + vec3(u.backR, u.backG, u.backB) * (1.0 - alpha);

    // Ordered (Bayer) dithering hides the 8-bit quantization of the surfaces on
    // slow fades, where gamma-expanded steps would otherwise band. Amplitude is
    // half an LSB, so no visible grain.
    ivec2 pc = ivec2(vUv * vec2(u.dyeSizeX, u.dyeSizeY)) & ivec2(3);
    float dither = (kDither[pc.y * 4 + pc.x] + 0.5) / 16.0 - 0.5;
    outC += vec3(dither) / 255.0;

    fragColor = vec4(outC, 1.0);
}
