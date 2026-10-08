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
#include "lib/view.glsl"

// Resolves the jittered scene color against the history reprojected from the previous frame, and
// writes the result both to the output and to the history of the next frame. The scene color is
// premultiplied by its coverage alpha, and all four channels are resolved.

layout(set = 0, binding = 1) uniform sampler linearClamp;
layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform TemporalAABlock
{
    vec4 params; // x weight of the current frame (0 keeps the history as it is), y 1 when the history is valid
} taa;

layout(set = 1, binding = 1) uniform texture2D u_current;
layout(set = 1, binding = 2) uniform texture2D u_history;
layout(set = 1, binding = 3) uniform texture2D u_depth;
layout(set = 1, binding = 4) uniform texture2D u_velocity;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outHistory;

vec3 toYCoCg(vec3 c)
{
    return vec3(0.25 * c.r + 0.5 * c.g + 0.25 * c.b, 0.5 * c.r - 0.5 * c.b, -0.25 * c.r + 0.5 * c.g - 0.25 * c.b);
}

vec3 fromYCoCg(vec3 c)
{
    return vec3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z);
}

ivec2 clampPixel(ivec2 pixel)
{
    return clamp(pixel, ivec2(0), ivec2(view.viewport.xy) - 1);
}

// Sanitized, so one bad texel can't spread through the neighborhood statistics into the history
vec4 fetchCurrent(ivec2 pixel)
{
    return yupSanitize(texelFetch(sampler2D(u_current, pointClamp), clampPixel(pixel), 0));
}

// Bicubic Catmull-Rom from 5 bilinear taps: the 9-tap form with its four corners dropped
vec4 sampleHistory(vec2 position)
{
    vec2 samplePosition = position * view.viewport.xy;
    vec2 center = floor(samplePosition - 0.5) + 0.5;
    vec2 f = samplePosition - center;

    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);
    vec2 w12 = w1 + w2;

    vec2 p0 = (center - 1.0) * view.viewport.zw;
    vec2 p3 = (center + 2.0) * view.viewport.zw;
    vec2 p12 = (center + w2 / w12) * view.viewport.zw;

    vec4 result = textureLod(sampler2D(u_history, linearClamp), vec2(p12.x, p0.y), 0.0) * (w12.x * w0.y)
                + textureLod(sampler2D(u_history, linearClamp), vec2(p0.x, p12.y), 0.0) * (w0.x * w12.y)
                + textureLod(sampler2D(u_history, linearClamp), p12, 0.0) * (w12.x * w12.y)
                + textureLod(sampler2D(u_history, linearClamp), vec2(p3.x, p12.y), 0.0) * (w3.x * w12.y)
                + textureLod(sampler2D(u_history, linearClamp), vec2(p12.x, p3.y), 0.0) * (w12.x * w3.y);

    float weight = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;
    return result / weight;
}

// Moves the history towards the center of the box until it is inside
vec3 clipToBox(vec3 history, vec3 boxMin, vec3 boxMax)
{
    vec3 center = 0.5 * (boxMax + boxMin);
    vec3 extents = max(0.5 * (boxMax - boxMin), vec3(1.0e-6));
    vec3 offset = history - center;
    vec3 units = abs(offset / extents);
    float largest = max(units.x, max(units.y, units.z));

    return largest > 1.0 ? center + offset / largest : history;
}

// Each color weighs 1 / (1 + luma) of its un-premultiplied color, so a bright sample can't dominate
// an edge between two surfaces, and its coverage on top, so an edge against the uncovered
// background stays linear in the coverage. The coverage alpha blends linearly
vec4 blendWeighted(vec4 history, vec4 current, float weight)
{
    float historyWeight = (1.0 - weight) / (1.0 + yupLuma(history.rgb) / max(history.a, 1.0e-4));
    float currentWeight = weight / (1.0 + yupLuma(current.rgb) / max(current.a, 1.0e-4));
    float coverage = historyWeight * history.a + currentWeight * current.a;
    float alpha = mix(history.a, current.a, weight);

    if (coverage <= 1.0e-8)
        return mix(history, current, weight);

    return vec4((history.rgb * historyWeight + current.rgb * currentWeight) * (alpha / coverage), alpha);
}

// Temporal anti-aliasing after Karis 2014, "High Quality Temporal Supersampling" (SIGGRAPH Advances
// in Real-Time Rendering): velocity of the closest depth, Catmull-Rom history, YCoCg variance
// clipping (Salvi 2016) and an exponential blend weighted by luma
void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec4 current = fetchCurrent(pixel);
    bool historyValid = taa.params.y > 0.5;

    // Converged and unchanged: keep the history exactly, so the image stops changing
    if (historyValid && taa.params.x <= 0.0)
    {
        vec4 kept = texelFetch(sampler2D(u_history, pointClamp), pixel, 0);
        outColor = kept;
        outHistory = kept;
        return;
    }

    vec3 mean = vec3(0.0);
    vec3 meanSquared = vec3(0.0);
    float alphaMin = current.a;
    float alphaMax = current.a;
    float closestDepth = texelFetch(sampler2D(u_depth, pointClamp), pixel, 0).r;
    ivec2 closestOffset = ivec2(0);

    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            ivec2 offset = ivec2(x, y);
            vec4 neighbor = fetchCurrent(pixel + offset);
            vec3 ycocg = toYCoCg(neighbor.rgb);

            mean += ycocg;
            meanSquared += ycocg * ycocg;
            alphaMin = min(alphaMin, neighbor.a);
            alphaMax = max(alphaMax, neighbor.a);

            float depth = texelFetch(sampler2D(u_depth, pointClamp), clampPixel(pixel + offset), 0).r;
            if (depth < closestDepth)
            {
                closestDepth = depth;
                closestOffset = offset;
            }
        }
    }

    mean /= 9.0;
    meanSquared /= 9.0;
    vec3 sigma = sqrt(max(meanSquared - mean * mean, vec3(0.0)));

    // Velocity is current uv minus previous uv, so the history of this pixel is at uv - velocity
    vec2 velocity = texelFetch(sampler2D(u_velocity, pointClamp), clampPixel(pixel + closestOffset), 0).xy;
    vec2 previousUv = uv - velocity;

    vec4 result = current;
    bool onScreen = all(greaterThanEqual(previousUv, vec2(0.0))) && all(lessThanEqual(previousUv, vec2(1.0)));

    if (historyValid && onScreen)
    {
        vec4 history = sampleHistory(previousUv);

        if (! any(yupIsNonFinite(history)))
        {
            // Back in RGB the box can reach slightly outside the range of the sanitized samples
            vec3 clipped = fromYCoCg(clipToBox(toYCoCg(history.rgb), mean - 1.25 * sigma, mean + 1.25 * sigma));
            vec4 clamped = vec4(clamp(clipped, vec3(0.0), vec3(yupMaxHalf)), clamp(history.a, alphaMin, alphaMax));
            result = blendWeighted(clamped, current, taa.params.x);
        }
    }

    // Both blended colors are in range already: no final clamp, which would hide a NaN on some backends
    outColor = result;
    outHistory = result;
}
