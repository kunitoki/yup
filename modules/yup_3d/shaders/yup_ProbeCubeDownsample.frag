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
#include "lib/cube.glsl"

// Writes one face of mip n of a cube from mip n - 1 of the same cube, bound alone so that it is
// lod 0 here: each texel is the average of the four texels it covers, weighted by their exact solid
// angle (the integral of the cube's area element, derived in lib/cube.glsl), which keeps the energy
// of the cube at every mip.

layout(set = 0, binding = 1) uniform sampler linearClamp;

layout(set = 1, binding = 0) uniform ProbeDownsampleBlock
{
    vec4 params; // x cube face, y 1 when the cube holds RGBM, z face size of the mip written, in texels
} probe;

layout(set = 1, binding = 1) uniform textureCube u_source;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    int face = int(probe.params.x);
    float sourceSize = probe.params.z * 2.0;
    vec2 firstTexel = floor(uv * probe.params.z) * 2.0;

    vec3 sum = vec3(0.0);
    float total = 0.0;

    for (int j = 0; j < 2; ++j)
    {
        for (int i = 0; i < 2; ++i)
        {
            // Looked up at the texel center, where the bilinear filter returns the texel alone
            vec2 st = (firstTexel + vec2(float(i), float(j)) + 0.5) / sourceSize;
            float weight = cubeTexelSolidAngle(st, sourceSize);
            vec4 texel = textureLod(samplerCube(u_source, linearClamp), cubeDirection(face, st), 0.0);

            sum += yupDecodeRgbm(texel, probe.params.y) * weight;
            total += weight;
        }
    }

    fragColor = yupEncodeRadiance(sum / total, probe.params.y);
}
