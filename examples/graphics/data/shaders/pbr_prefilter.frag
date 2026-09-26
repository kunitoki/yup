#version 450
#extension GL_GOOGLE_include_directive : require

#include "pbr_bake.glsl"

layout(set = 0, binding = 1) uniform textureCube u_env;
layout(set = 0, binding = 2) uniform sampler     u_samp;

const uint kSampleCount = 256u;

void main() {
    vec3 n = faceDirection(u.face, faceUV());
    vec3 v = n;

    vec3 prefiltered = vec3(0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < kSampleCount; ++i) {
        vec3 h = importanceSampleGGX(hammersley(i, kSampleCount), n, u.roughness);
        vec3 l = normalize(2.0 * dot(v, h) * h - v);

        float nDotL = dot(n, l);
        if (nDotL > 0.0) {
            prefiltered += textureLod(samplerCube(u_env, u_samp), l, 0.0).rgb * nDotL;
            totalWeight += nDotL;
        }
    }

    fragColor = vec4(prefiltered / max(totalWeight, 0.001), 1.0);
}
