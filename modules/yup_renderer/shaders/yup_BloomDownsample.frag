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

// Halves the resolution of the bloom chain. The first level reads the scene color, premultiplied by
// its coverage: it inverts a reversibly tone mapped source per texel, weights its taps by luma
// against fireflies, and keeps only the light above the threshold, with a quadratic soft knee.
// Only rgb is filtered.

layout(set = 0, binding = 1) uniform sampler linearClamp;

layout(set = 1, binding = 0) uniform BloomDownsampleBlock
{
    vec4 params; // x 1 on the first level, y 1 when the source is reversibly tone mapped, z threshold, w knee
    vec4 texel;  // xy the size of a source texel in uv, zw the size of the source in texels
} bloom;

layout(set = 1, binding = 1) uniform texture2D u_source;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

// The upsample chain adds at most 6 levels onto level 0, each a weighted average no brighter than
// the first one, so capping the first at an eighth of the half range keeps every sum finite
const float maxFirstLevel = yupMaxHalf / 8.0;

vec3 fetchInverted(ivec2 pixel)
{
    ivec2 last = ivec2(bloom.texel.zw) - 1;
    return yupUnpremultiplyInverse(texelFetch(sampler2D(u_source, linearClamp), clamp(pixel, ivec2(0), last), 0));
}

// A bilinear tap @p offset source texels away. A reversibly tone mapped source is inverted per
// texel before the bilinear weights, so the filter averages linear color. Non-finite values are
// sanitized, so one bad texel can't spread over the whole glow
vec3 fetch(vec2 offset)
{
    vec2 position = uv + offset * bloom.texel.xy;
    vec3 c;

    if (bloom.params.y > 0.5)
    {
        vec2 p = position * bloom.texel.zw - 0.5;
        ivec2 i = ivec2(floor(p));
        vec2 f = fract(p);

        c = mix(mix(fetchInverted(i), fetchInverted(i + ivec2(1, 0)), f.x),
                mix(fetchInverted(i + ivec2(0, 1)), fetchInverted(i + ivec2(1, 1)), f.x),
                f.y);
    }
    else
    {
        c = textureLod(sampler2D(u_source, linearClamp), position, 0.0).rgb;
    }

    return yupSanitize(vec4(c, 0.0)).rgb;
}

// Karis average: a group of four taps weighs less the brighter it is
vec3 weighGroup(vec3 a, vec3 b, vec3 c, vec3 d, float weight, inout float total)
{
    vec3 average = (a + b + c + d) * 0.25;
    float w = weight / (1.0 + yupLuma(average));
    total += w;
    return average * w;
}

// Quadratic soft knee around the threshold, scaling the color by its kept fraction
vec3 applyThreshold(vec3 c)
{
    float brightness = max(c.r, max(c.g, c.b));
    float threshold = bloom.params.z;
    float knee = bloom.params.w;

    float soft = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 1.0e-4);

    return c * (max(soft, brightness - threshold) / max(brightness, 1.0e-4));
}

// Bloom downsample after Jimenez 2014, "Next Generation Post Processing in Call of Duty: Advanced
// Warfare" (SIGGRAPH Advances in Real-Time Rendering): 13 bilinear taps as five overlapping 2x2
// boxes, the partial Karis average on the first level only
void main()
{
    vec3 a = fetch(vec2(-2.0, -2.0));
    vec3 b = fetch(vec2(0.0, -2.0));
    vec3 c = fetch(vec2(2.0, -2.0));
    vec3 d = fetch(vec2(-2.0, 0.0));
    vec3 e = fetch(vec2(0.0, 0.0));
    vec3 f = fetch(vec2(2.0, 0.0));
    vec3 g = fetch(vec2(-2.0, 2.0));
    vec3 h = fetch(vec2(0.0, 2.0));
    vec3 i = fetch(vec2(2.0, 2.0));
    vec3 j = fetch(vec2(-1.0, -1.0));
    vec3 k = fetch(vec2(1.0, -1.0));
    vec3 l = fetch(vec2(-1.0, 1.0));
    vec3 m = fetch(vec2(1.0, 1.0));

    vec3 color;

    if (bloom.params.x > 0.5)
    {
        float total = 0.0;
        color = weighGroup(j, k, l, m, 0.5, total)
              + weighGroup(a, b, d, e, 0.125, total)
              + weighGroup(b, c, e, f, 0.125, total)
              + weighGroup(d, e, g, h, 0.125, total)
              + weighGroup(e, f, h, i, 0.125, total);

        color = min(applyThreshold(color / total), vec3(maxFirstLevel));
    }
    else
    {
        color = e * 0.125
              + (a + c + g + i) * 0.03125
              + (b + d + f + h) * 0.0625
              + (j + k + l + m) * 0.125;
    }

    fragColor = vec4(color, 0.0);
}
