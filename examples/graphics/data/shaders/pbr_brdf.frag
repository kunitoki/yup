#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 fragColor;
layout(set = 0, binding = 0) uniform Bake {
    int   face;
    float roughness;
    float flipY;
    float envSize;
    float sunIntensity;
} u;

const float PI = 3.14159265359;
const uint kSampleCount = 512u;

float radicalInverse(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec3 importanceSampleGGX(vec2 xi, vec3 n, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 h = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, n));
    vec3 bitangent = cross(n, tangent);
    return normalize(tangent * h.x + bitangent * h.y + n * h.z);
}

// Smith geometry term with the IBL (rather than direct-lighting) k.
float geometrySmith(float nDotV, float nDotL, float roughness) {
    float k = (roughness * roughness) * 0.5;
    float ggxV = nDotV / (nDotV * (1.0 - k) + k);
    float ggxL = nDotL / (nDotL * (1.0 - k) + k);
    return ggxV * ggxL;
}

void main() {
    vec2 uv = vec2(v_uv.x, u.flipY > 0.5 ? 1.0 - v_uv.y : v_uv.y);

    float nDotV = max(uv.x, 0.001);
    float roughness = max(uv.y, 0.001);

    vec3 v = vec3(sqrt(1.0 - nDotV * nDotV), 0.0, nDotV);
    vec3 n = vec3(0.0, 0.0, 1.0);

    float scale = 0.0;
    float bias = 0.0;

    for (uint i = 0u; i < kSampleCount; ++i) {
        vec2 xi = vec2(float(i) / float(kSampleCount), radicalInverse(i));
        vec3 h = importanceSampleGGX(xi, n, roughness);
        vec3 l = normalize(2.0 * dot(v, h) * h - v);

        float nDotL = max(l.z, 0.0);
        float nDotH = max(h.z, 0.0);
        float vDotH = max(dot(v, h), 0.0);

        if (nDotL > 0.0) {
            float g = geometrySmith(nDotV, nDotL, roughness);
            float gVis = (g * vDotH) / max(nDotH * nDotV, 0.001);
            float fc = pow(1.0 - vDotH, 5.0);

            scale += (1.0 - fc) * gVis;
            bias += fc * gVis;
        }
    }

    fragColor = vec4(scale / float(kSampleCount), bias / float(kSampleCount), 0.0, 1.0);
}
