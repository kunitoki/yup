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

// The spatial denoise of the screen-space reflections (ReflectionPass), after the resolve and its
// history, as the denoisers of stochastic reflections do (Schied et al. 2017, "Spatiotemporal
// Variance-Guided Filtering"; AMD FidelityFX SSSR 2020): a bilateral 3 x 3 filter whose taps spread
// with the roughness, by a pixel per 0.15 of perceptual roughness up to 3, so mirrors keep every
// detail and glossy surfaces, whose few rays per pixel scatter the most, average a neighborhood of
// up to 7 x 7 pixels. A tap counts by a binomial weight, by how close its depth and normal are to
// the pixel's, as in the resolve, and by how close its roughness is, so the reflections do not
// bleed across edges or between materials. The radiance averages reversibly tone mapped (Karis
// 2014) and weighted by its confidence, so a confident reflection is not darkened by neighbors
// without one; the confidence averages on its own.
// Output: the exposed radiance in rgb, the confidence in a, as the resolve.

layout(set = 1, binding = 3) uniform texture2D u_resolved;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec4 center = texelFetch(sampler2D(u_resolved, pointClamp), pixel, 0);
    fragColor = center;

    float depth = ssrDepth(pixel);
    vec4 normalRoughness = ssrNormalRoughness(pixel);
    int stride = int(min(floor(normalRoughness.w / 0.15 + 0.5), 3.0));
    if (depth >= 1.0 || stride == 0)
        return;

    vec3 position = ssrViewPosition((vec2(pixel) + 0.5) * view.viewport.zw, depth);
    ivec2 last = ivec2(view.viewport.xy) - 1;

    vec3 radiance = vec3(0.0);
    float radianceWeight = 0.0;
    float confidence = 0.0;
    float weights = 0.0;

    for (int i = 0; i < 9; ++i)
    {
        ivec2 offset = ivec2(i % 3 - 1, i / 3 - 1);
        ivec2 tap = clamp(pixel + offset * stride, ivec2(0), last);

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
        radiance += yupReversibleToneMap(value.rgb) * (value.a * weight);
        radianceWeight += value.a * weight;
        confidence += value.a * weight;
        weights += weight;
    }

    if (weights <= 0.0)
        return;

    vec3 filtered = radianceWeight > 0.0 ? yupReversibleToneMapInverse(radiance / radianceWeight) : vec3(0.0);
    fragColor = vec4(min(filtered, vec3(yupMaxHalf)), confidence / weights);
}
