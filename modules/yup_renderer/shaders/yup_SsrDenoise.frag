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

// The denoise of the screen-space reflections (ReflectionPass), after the resolve, at full
// resolution: a spatial filter of the reflections of this frame, which then join their history, in
// the order of the denoisers of stochastic reflections (Schied et al. 2017, "Spatiotemporal
// Variance-Guided Filtering"; AMD FidelityFX reflection denoiser 2021).
// The filter is bilateral, 3 x 3, its taps spread with the roughness, by a pixel per 0.15 of
// perceptual roughness up to 3, so mirrors keep every detail and glossy surfaces, whose few rays
// per pixel scatter the most, average a neighborhood of up to 7 x 7 pixels. A tap counts by a
// binomial weight, by how close its depth and normal are to the pixel's, as in the resolve, and by
// how close its roughness is, so the reflections do not bleed across edges or between materials.
// The radiance averages reversibly tone mapped (Karis 2014) and weighted by its confidence, so a
// confident reflection is not darkened by neighbors without one; the confidence averages on its own.
// The result then joins the denoise of the previous frame, reprojected by the motion of the
// reflected point (the pixel's motion plus the parallax of the mean hit distance behind it,
// Stachowiak 2015) and rescaled to this exposure, by an exponential blend of ssrHistoryWeight
// (Karis 2014). The history is first clipped to the range of the taps, taken at least a pixel apart:
// the 81 rays behind them bound what the reflection may be, where the 9 of one pixel do not, since
// a frame of them may all miss a surface the lobe mostly leaves out, and would take from the
// history the light it gathered there. So a reflection the scene no longer shows leaves at once,
// the others hold still while the scene changes, and TemporalAA, whose clipping follows the noise
// of a single frame, restarts its running mean.
// Output: the exposed radiance in rgb, the confidence in a, exactly 0 without any. It is also the
// history of the next frame.

layout(set = 0, binding = 1) uniform sampler linearClamp;

layout(set = 1, binding = 3) uniform texture2D u_resolved;
layout(set = 1, binding = 4) uniform texture2D u_velocity;
layout(set = 1, binding = 5) uniform texture2D u_hitDistance;
layout(set = 1, binding = 6) uniform texture2D u_history;

// The share of this frame against the history
const float ssrHistoryWeight = 0.125;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

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

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    float depth = ssrDepth(pixel);
    vec4 normalRoughness = ssrNormalRoughness(pixel);
    fragColor = vec4(0.0);

    if (depth >= 1.0 || normalRoughness.w >= ssr.params.y)
        return;

    vec3 position = ssrViewPosition((vec2(pixel) + 0.5) * view.viewport.zw, depth);
    ivec2 last = ivec2(view.viewport.xy) - 1;

    // Without a spread only the pixel itself is filtered, and its neighbors still bound the history
    int stride = int(min(floor(normalRoughness.w / 0.15 + 0.5), 3.0));
    int spread = max(stride, 1);

    vec3 radiance = vec3(0.0);
    float radianceWeight = 0.0;
    float confidence = 0.0;
    float weights = 0.0;
    vec3 boxMin = vec3(yupMaxHalf);
    vec3 boxMax = vec3(-yupMaxHalf);

    for (int i = 0; i < 9; ++i)
    {
        ivec2 offset = ivec2(i % 3 - 1, i / 3 - 1);
        ivec2 tap = clamp(pixel + offset * spread, ivec2(0), last);

        float tapDepth = ssrDepth(tap);
        if (tapDepth >= 1.0)
            continue;

        vec4 tapNormalRoughness = ssrNormalRoughness(tap);
        vec3 tapPosition = ssrViewPosition((vec2(tap) + 0.5) * view.viewport.zw, tapDepth);
        float facing = max(dot(tapNormalRoughness.xyz, normalRoughness.xyz), 0.0);
        facing *= facing;
        facing *= facing;
        facing *= facing;

        float similar = clamp(1.0 - abs(tapPosition.z - position.z) / (0.05 * max(abs(position.z), 1.0e-6)), 0.0, 1.0)
                      * facing * clamp(1.0 - abs(tapNormalRoughness.w - normalRoughness.w) / 0.1, 0.0, 1.0);
        float weight = similar * (offset.x == 0 ? 1.0 : 0.5) * (offset.y == 0 ? 1.0 : 0.5);
        if (weight <= 0.0)
            continue;

        vec4 value = texelFetch(sampler2D(u_resolved, pointClamp), tap, 0);
        vec3 mapped = yupReversibleToneMap(value.rgb);

        if (value.a > 0.0)
        {
            vec3 ycocg = yupToYCoCg(mapped);
            boxMin = min(boxMin, ycocg);
            boxMax = max(boxMax, ycocg);
        }

        if (stride == 0 && offset != ivec2(0))
            continue;

        radiance += mapped * (value.a * weight);
        radianceWeight += value.a * weight;
        confidence += value.a * weight;
        weights += weight;
    }

    vec3 current = radianceWeight > 0.0 ? radiance / radianceWeight : vec3(0.0);
    float currentConfidence = weights > 0.0 ? confidence / weights : 0.0;

    // The history, where the previous frame saw the reflected point
    vec2 velocity = texelFetch(sampler2D(u_velocity, pointClamp), pixel, 0).xy;
    float hitDistance = texelFetch(sampler2D(u_hitDistance, pointClamp), pixel, 0).r;
    if (hitDistance > 0.0)
        velocity += reflectionParallax(position, position - ssrViewVector(position) * hitDistance);

    vec2 previousUv = (vec2(pixel) + 0.5) * view.viewport.zw - velocity;
    vec4 history = vec4(0.0);
    if (ssr.frame.w > 0.5 && all(greaterThanEqual(previousUv, vec2(0.0))) && all(lessThanEqual(previousUv, vec2(1.0))))
        history = yupSanitize(textureLod(sampler2D(u_history, linearClamp), previousUv, 0.0));

    vec3 kept = yupReversibleToneMap(history.rgb * (ssr.frame.x * view.cameraPosition.w));
    if (boxMin.x <= boxMax.x)
        kept = yupFromYCoCg(yupClipToBox(yupToYCoCg(kept), boxMin, boxMax));

    float historyShare = (1.0 - ssrHistoryWeight) * history.a;
    float currentShare = ssrHistoryWeight * currentConfidence;
    float shares = historyShare + currentShare;
    if (shares <= 0.0)
        return;

    vec3 blended = (kept * historyShare + current * currentShare) / shares;
    float blendedConfidence = history.a > 0.0 ? mix(history.a, currentConfidence, ssrHistoryWeight) : currentConfidence;
    fragColor = vec4(min(yupReversibleToneMapInverse(blended), vec3(yupMaxHalf)), blendedConfidence);
}
