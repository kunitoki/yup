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
// the half float target holds the radiance of bright scenes shown at a low exposure. Its confidence fades towards the
// screen edges and for rays turning towards the camera, whose hits the depth may hide behind what
// faces it; a ray reprojected off screen counts as a miss. The confidence of the pixel is the mean
// over the similar texels, a miss counting 0, faded out above the roughness fade start.
// The result then joins the resolve of the previous frame, reprojected by the motion of the
// reflected point (the pixel's motion plus the parallax of the mean hit distance behind it) and rescaled to this exposure, by an exponential blend of ssrHistoryWeight (Karis 2014): the
// history is first clamped to the range of the rays' radiance, wide where the rough lobes scatter
// the rays and tight on mirrors, so the reflections hold still while the scene changes and
// TemporalAA, whose clipping follows the noise of a single frame, restarts its running mean.
// Output: the exposed radiance in rgb, the confidence in a; exactly 0 without any.

layout(set = 0, binding = 1) uniform sampler linearClamp;

layout(set = 1, binding = 3) uniform texture2D u_trace;
layout(set = 1, binding = 4) uniform texture2D u_velocity;
layout(set = 1, binding = 5) uniform texture2D u_previousSceneColor;
layout(set = 1, binding = 6) uniform texture2D u_history;

// The share of this frame against the history
const float ssrHistoryWeight = 0.125;

layout(location = 0) in vec2 uv;

// The motion of a reflection beyond the motion of its surface, from the camera alone: the motion of
// the reflected point, seen at its distance along the view ray behind the surface, minus the
// motion of the surface's own position (Stachowiak 2015). Both in view space, of this frame
vec2 reflectionParallax(vec3 surface, vec3 reflected)
{
    mat3 rotation = transpose(mat3(view.view));
    vec4 surfaceWorld = vec4(rotation * (surface - view.view[3].xyz), 1.0);
    vec4 reflectedWorld = vec4(rotation * (reflected - view.view[3].xyz), 1.0);

    vec4 surfaceNow = view.unjitteredViewProjection * surfaceWorld;
    vec4 surfaceBefore = view.previousViewProjection * surfaceWorld;
    vec4 reflectedNow = view.unjitteredViewProjection * reflectedWorld;
    vec4 reflectedBefore = view.previousViewProjection * reflectedWorld;
    if (min(min(surfaceNow.w, surfaceBefore.w), min(reflectedNow.w, reflectedBefore.w)) <= 1.0e-6)
        return vec2(0.0);

    return viewVelocity(reflectedNow, reflectedBefore) - viewVelocity(surfaceNow, surfaceBefore);
}

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
    ivec2 center = (pixel + ivec2(int(mod(view.clipPlanes.z, 2.0)))) / 2;

    vec3 radiance = vec3(0.0);
    float radianceWeight = 0.0;
    float confidence = 0.0;
    float similarity = 0.0;
    float hitDistance = 0.0;
    vec3 boxMin = vec3(yupMaxHalf);
    vec3 boxMax = vec3(-yupMaxHalf);

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
        lobe = min(lobe, 1.0);
        float weight = lobe * similar * valid;

        vec3 rayRadiance = yupReversibleToneMap(previous.rgb / previous.a);
        radiance += rayRadiance * weight;
        vec3 rayYCoCg = yupToYCoCg(rayRadiance);
        boxMin = min(boxMin, rayYCoCg);
        boxMax = max(boxMax, rayYCoCg);
        radianceWeight += weight;
        hitDistance += length(toHit) * weight;
        confidence += similar * valid;
    }

    float scale = ssr.frame.x * view.cameraPosition.w;
    bool traced = radianceWeight > 0.0 && confidence > 0.0;
    vec3 current = traced ? radiance / radianceWeight : vec3(0.0);
    float currentConfidence = traced ? confidence / similarity * fade : 0.0;

    // The history, exposed by the previous frame like the rays' radiance, so both share its tone mapping,
    // where the previous frame saw the reflected point
    vec2 velocity = texelFetch(sampler2D(u_velocity, pointClamp), pixel, 0).xy;
    if (traced)
        velocity += reflectionParallax(position, position - v * (hitDistance / radianceWeight));

    vec2 previousUv = (vec2(pixel) + 0.5) * view.viewport.zw - velocity;
    vec4 history = vec4(0.0);
    if (ssr.frame.w > 0.5 && all(greaterThanEqual(previousUv, vec2(0.0))) && all(lessThanEqual(previousUv, vec2(1.0))))
        history = yupSanitize(textureLod(sampler2D(u_history, linearClamp), previousUv, 0.0));

    vec3 kept = yupReversibleToneMap(history.rgb);
    if (traced)
        kept = yupFromYCoCg(yupClipToBox(yupToYCoCg(kept), boxMin, boxMax));

    float historyShare = (1.0 - ssrHistoryWeight) * history.a;
    float currentShare = ssrHistoryWeight * currentConfidence;
    float shares = historyShare + currentShare;
    if (shares <= 0.0)
        return;

    vec3 blended = (kept * historyShare + current * currentShare) / shares;
    float blendedConfidence = history.a > 0.0 ? mix(history.a, currentConfidence, ssrHistoryWeight) : currentConfidence;
    fragColor = vec4(min(yupReversibleToneMapInverse(blended) * scale, vec3(yupMaxHalf)), blendedConfidence);
}
