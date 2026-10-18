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

#include "lib/ssr.glsl"

// The resolve of the screen-space reflections (ReflectionPass), at full resolution: the ratio
// estimator of Stachowiak 2015 ("Stochastic Screen-Space Reflections", SIGGRAPH Advances in Real-Time
// Rendering) over the rays of the 4 nearest trace texels. Each ray weighs the GGX lobe of this pixel
// towards its hit over the pdf it was sampled with, times how close the depth and normal of the texel
// it was traced from are to this pixel's, so rays do not leak across edges. The sum of the weighted
// radiance over the sum of the weights is the prefiltered radiance of the split sum, which the
// lighting pass shades with its own lookup tables.
// The radiance of a ray is the previous scene color (exposed, premultiplied by coverage) where the
// previous frame saw the hit, divided by that frame's exposure. Its confidence fades towards the
// screen edges and for rays turning towards the camera, whose hits the depth may hide behind what
// faces it; a ray reprojected off screen counts as a miss. The confidence of the pixel is the mean
// over the similar texels, a miss counting 0, faded out above the roughness fade start.
// Output: the radiance in rgb, the confidence in a; exactly 0 without any.

layout(set = 0, binding = 1) uniform sampler linearClamp;

layout(set = 1, binding = 3) uniform texture2D u_trace;
layout(set = 1, binding = 4) uniform texture2D u_velocity;
layout(set = 1, binding = 5) uniform texture2D u_previousSceneColor;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    float depth = ssrDepth(pixel);
    vec4 normalRoughness = ssrNormalRoughness(pixel);
    float fade = clamp((ssr.params.y - normalRoughness.w) / (ssr.params.y - ssr.params.x), 0.0, 1.0);
    fragColor = vec4(0.0);

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

    vec3 radiance = vec3(0.0);
    float radianceWeight = 0.0;
    float confidence = 0.0;
    float similarity = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        ivec2 texel = min(pixel / 2 + ivec2(i & 1, i >> 1), last);
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

        vec3 l = toHit * inversesqrt(dot(toHit, toHit));
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

        vec4 previous = textureLod(sampler2D(u_previousSceneColor, linearClamp), previousUv, 0.0);
        if (valid <= 0.0 || previous.a <= 1.0e-3)
            continue;

        float nDotH = max(dot(n, normalize(v + l)), 0.0);
        float lobe = bsdfDistributionGgx(nDotH, alpha2) * bsdfVisibilitySmithGgxCorrelated(nDotV, nDotL, alpha2) * nDotL / ray.w;
        float weight = lobe * similar * valid;

        radiance += previous.rgb / previous.a * weight;
        radianceWeight += weight;
        confidence += similar * valid;
    }

    if (radianceWeight <= 0.0 || confidence <= 0.0)
        return;

    fragColor = vec4(min(radiance / radianceWeight * ssr.frame.x, vec3(65504.0)), confidence / similarity * fade);
}
