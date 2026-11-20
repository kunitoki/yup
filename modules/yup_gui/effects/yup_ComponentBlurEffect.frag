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

layout(set = 0, binding = 0) uniform texture2D u_source;
layout(set = 0, binding = 1) uniform sampler s_source;

layout(set = 0, binding = 2) uniform BlurParams
{
    vec4 metrics;   // x: sigma, y: radius, z: invWidth, w: invHeight
    vec4 direction; // x: dirX, y: dirY, z: alphaSilhouette, w: unused
} blur;

layout(location = 0) out vec4 fragColor;

vec4 sampleSource(vec2 uv)
{
    vec4 texel = textureLod(sampler2D(u_source, s_source), uv, 0.0);
    return blur.direction.z > 0.5 ? vec4(texel.a) : texel;
}

void main()
{
    vec2 uv = gl_FragCoord.xy * blur.metrics.zw;
    if (blur.metrics.x <= 0.0001)
    {
        fragColor = sampleSource(uv);
        return;
    }

    int radius = int(clamp(blur.metrics.y, 1.0, 128.0));
    vec2 stepUv = blur.direction.xy * blur.metrics.zw;
    float invTwoSigmaSq = 0.5 / (blur.metrics.x * blur.metrics.x);

    vec4 sum = sampleSource(uv);
    float weightSum = 1.0;

    for (int i = 1; i <= radius; ++i)
    {
        float offset = float(i);
        float weight = exp(-(offset * offset) * invTwoSigmaSq);
        vec2 delta = stepUv * offset;
        sum += sampleSource(uv + delta) * weight;
        sum += sampleSource(uv - delta) * weight;
        weightSum += 2.0 * weight;
    }

    fragColor = sum / weightSum;
}
