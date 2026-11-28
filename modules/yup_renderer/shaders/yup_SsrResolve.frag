/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#version 450
#extension GL_GOOGLE_include_directive : require

#include "lib/color.glsl"
#include "lib/ssr.glsl"

// The resolve of the screen-space reflections (ReflectionPass), at full resolution: the ratio
// estimator of Stachowiak 2015 ("Stochastic Screen-Space Reflections", SIGGRAPH Advances in Real-Time
// Rendering) over the rays of the 3 x 3 trace texels around the pixel, their center alternating
// between the two nearest every frame where the pixel lies between them. Each ray weighs the GGX lobe
// of this pixel towards its hit over the pdf it was sampled with, at most 1, the ratio of a ray
// sampled from this pixel's own lobe, so a neighbor's ray in the tail of its own lobe does not
// outweigh the others and sparkle, times how close the depth and normal of the texel it was traced
// from are to this pixel's, so rays do not leak across edges. The sum of the weighted
// radiance over the sum of the weights is the prefiltered radiance of the split sum, which the
// lighting pass shades with its own lookup tables. The radiance is averaged reversibly tone mapped
// (Karis 2014), so a rare ray reaching a bright highlight does not sparkle over the ones that did not.
// The radiance of a ray is the previous scene color (exposed, premultiplied by coverage) where the
// previous frame saw the hit, rescaled from that frame's exposure to this one's: stored exposed,
// the half float target holds the radiance of bright scenes shown at a low exposure. It is read
// filtered over the cone of directions the ray stands for among the 9 resolved, as the talk does
// (filtered importance sampling: Colbert and Krivanek 2007, "GPU-Based Importance Sampling", GPU
// Gems 3): the cone of solid angle 1 / (9 pdf), ssrConeBias levels wider, is as many texels across
// at the hit as the level of the radiance pyramid read. So a small bright surface a wide lobe seldom
// hits lights every ray whose cone holds it, by its share of the cone, where a single texel would
// sparkle in the few rays reaching it; the narrow cones of a mirror read the scene color itself.
// The confidence of a ray fades towards the screen edges and for rays turning towards the camera,
// whose hits the depth may hide behind what faces it; a ray reprojected off screen counts as a miss.
// The confidence of the pixel is the mean over the similar texels, a miss counting 0, faded out
// above the roughness fade start.
// Outputs: the exposed radiance in rgb and the confidence in a, exactly 0 without any, and the mean
// distance to the hits, 0 without any, by which the denoise follows the reflection across frames.

layout(set = 0, binding = 1) uniform sampler linearClamp;

layout(set = 1, binding = 3) uniform texture2D u_trace;
layout(set = 1, binding = 4) uniform texture2D u_velocity;
layout(set = 1, binding = 5) uniform texture2D u_previousSceneColor;
layout(set = 1, binding = 6) uniform texture2D u_radiancePyramid;

// The levels of the radiance pyramid a ray reads above the one of its cone: one, as ProbePrefilter
// does, so the 9 rays of a pixel still cover its lobe
const float ssrConeBias = 1.0;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragHitDistance;

// The previous scene color around a uv, over 2^lod of its texels: the scene color itself up to lod
// 0, the radiance pyramid from lod 1, whose level 0 halves it, and a mix of the two between
vec4 previousRadiance(vec2 position, float lod)
{
    if (lod <= 0.0)
        return textureLod(sampler2D(u_previousSceneColor, linearClamp), position, 0.0);

    vec4 filtered = textureLod(sampler2D(u_radiancePyramid, linearClamp), position, max(lod - 1.0, 0.0));
    return lod < 1.0 ? mix(textureLod(sampler2D(u_previousSceneColor, linearClamp), position, 0.0), filtered, lod) : filtered;
}

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    float depth = ssrDepth(pixel);
    vec4 normalRoughness = ssrNormalRoughness(pixel);
    float fade = clamp((ssr.params.y - normalRoughness.w) / (ssr.params.y - ssr.params.x), 0.0, 1.0);
    fragColor = vec4(0.0);
    fragHitDistance = vec4(0.0);

    if (depth >= 1.0 || fade <= 0.0)
        return;

    vec3 position = ssrViewPosition((vec2(pixel) + 0.5) * view.viewport.zw, depth);
    vec3 v = ssrViewVector(position);
    vec3 n = normalRoughness.xyz;
    float alpha = normalRoughness.w * normalRoughness.w;
    float alpha2 = alpha * alpha;
    float nDotV = max(dot(n, v), 1.0e-4);

    ivec2 viewport = ivec2(view.viewport.xy);
    ivec2 last = textureSize(sampler2D(u_trace, pointClamp), 0) - 1;
    ivec2 center = (pixel + ivec2(int(mod(view.clipPlanes.z, 2.0)))) / 2;

    // Upscaling keeps the previous scene color larger than the frame the rays were traced in
    float texelsPerPixel = float(textureSize(sampler2D(u_previousSceneColor, linearClamp), 0).y) / view.viewport.y;

    vec3 radiance = vec3(0.0);
    float radianceWeight = 0.0;
    float confidence = 0.0;
    float similarity = 0.0;
    float hitDistance = 0.0;

    for (int i = 0; i < 9; ++i)
    {
        ivec2 texel = clamp(center + ivec2(i % 3 - 1, i / 3 - 1), ivec2(0), last);
        ivec2 source = min(texel * 2, viewport - 1);

        float sourceDepth = ssrDepth(source);
        if (sourceDepth >= 1.0)
            continue;

        vec3 sourcePosition = ssrViewPosition((vec2(source) + 0.5) * view.viewport.zw, sourceDepth);
        float facing = max(dot(ssrNormalRoughness(source).xyz, n), 0.0);
        facing *= facing;
        facing *= facing;
        facing *= facing;

        float similar = clamp(1.0 - abs(sourcePosition.z - position.z) / (0.05 * max(abs(position.z), 1.0e-6)), 0.0, 1.0) * facing;
        if (similar <= 0.0)
            continue;

        similarity += similar;

        vec4 ray = texelFetch(sampler2D(u_trace, pointClamp), texel, 0);
        if (ray.w <= 0.0)
            continue;

        // A hit at this very position has no direction
        vec3 toHit = ssrViewPosition(ray.xy, ray.z) - position;
        if (dot(toHit, toHit) < 1.0e-12)
            continue;

        float distance = length(toHit);
        vec3 l = toHit / distance;
        float nDotL = dot(n, l);
        if (nDotL <= 0.0)
            continue;

        // Screen edges, and rays turning towards the camera: full up to 0.25 of l.v, none from 0.75
        vec2 edge = min(ray.xy, 1.0 - ray.xy);
        float valid = clamp(min(edge.x, edge.y) / ssr.frame.z, 0.0, 1.0) * clamp(1.5 - 2.0 * dot(l, v), 0.0, 1.0);

        // Where the previous frame saw the hit
        ivec2 hitPixel = clamp(ivec2(ray.xy * view.viewport.xy), ivec2(0), viewport - 1);
        vec2 previousUv = ray.xy - texelFetch(sampler2D(u_velocity, pointClamp), hitPixel, 0).xy;
        if (any(lessThan(previousUv, vec2(0.0))) || any(greaterThan(previousUv, vec2(1.0))))
            continue;

        // The cone of the ray, in texels of the previous scene color across at the hit
        float cone = 2.0 * distance * inversesqrt(9.0 * yupPi * ray.w);
        float lod = log2(cone * ssrPixelsPerUnit(ssrViewDepth(ray.z)) * texelsPerPixel) + ssrConeBias;

        vec4 previous = previousRadiance(previousUv, lod);
        if (valid <= 0.0 || previous.a <= 1.0e-3)
            continue;

        float nDotH = max(dot(n, normalize(v + l)), 0.0);
        float lobe = bsdfDistributionGgx(nDotH, alpha2) * bsdfVisibilitySmithGgxCorrelated(nDotV, nDotL, alpha2) * nDotL / ray.w;
        lobe = min(lobe, 1.0);
        float weight = lobe * similar * valid;

        radiance += yupReversibleToneMap(previous.rgb / previous.a) * weight;
        radianceWeight += weight;
        hitDistance += distance * weight;
        confidence += similar * valid;
    }

    if (radianceWeight <= 0.0 || confidence <= 0.0)
        return;

    float scale = ssr.frame.x * view.cameraPosition.w;
    fragColor = vec4(min(yupReversibleToneMapInverse(radiance / radianceWeight) * scale, vec3(yupMaxHalf)), confidence / similarity * fade);
    fragHitDistance = vec4(hitDistance / radianceWeight, 0.0, 0.0, 0.0);
}
