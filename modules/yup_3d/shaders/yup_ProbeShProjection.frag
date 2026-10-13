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

// Projects the radiance of an irradiance volume probe's capture cube on the first two bands of the
// real spherical harmonics and convolves them into irradiance (Ramamoorthi and Hanrahan 2001), as
// SphericalHarmonics::project() and toIrradiance() do on the CPU: four coefficients, in the order
// 1, y, z, x, which evaluate as l0 + l1y n.y + l1z n.z + l1x n.x. The pass is drawn with a 1 x 4
// viewport on the probe's four texels of the irradiance atlas, coefficient c in row c.
// Every texel of one cube mip is summed, weighted by its exact solid angle (lib/cube.glsl): the
// mip whose faces are 8 x 8 texels, 384 samples, which the two bands need no more than.
// Below the float floor the atlas holds RGBM: coefficient 0 in RGBM, the others as their ratio to
// it per channel, remapped from [-2, 2] to [0, 1] (SphericalHarmonics::encodeRatio()), since they
// can be negative and RGBM cannot hold that.

layout(set = 0, binding = 1) uniform sampler linearClamp;

layout(set = 1, binding = 0) uniform ProbeShBlock
{
    vec4 params; // x unused, y 1 when the textures hold RGBM, z face size of the mip read, in texels
} probe;

// The mip read, bound alone so that it is lod 0 here
layout(set = 1, binding = 1) uniform textureCube u_source;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    int coefficient = clamp(int(uv.y * 4.0), 0, 3);
    float size = probe.params.z;
    int texels = int(size);

    // The basis constants of the two bands times the clamped cosine convolution over pi:
    // 0.282095 * 0.282095 for the first, 0.488603 * 0.488603 * 2 / 3 for the second
    vec3 l0 = vec3(0.0);
    vec3 lc = vec3(0.0);

    for (int face = 0; face < 6; ++face)
    {
        for (int j = 0; j < texels; ++j)
        {
            for (int i = 0; i < texels; ++i)
            {
                // Looked up at the texel center, where the bilinear filter returns the texel alone
                vec2 st = (vec2(float(i), float(j)) + 0.5) / size;
                vec3 d = cubeDirection(face, st);
                vec3 radiance = yupDecodeRgbm(textureLod(samplerCube(u_source, linearClamp), d, 0.0), probe.params.y) * cubeTexelSolidAngle(st, size);
                float basis = coefficient == 1 ? d.y : (coefficient == 2 ? d.z : d.x);

                l0 += radiance;
                lc += radiance * basis;
            }
        }
    }

    l0 *= 0.282095 * 0.282095;
    lc *= 0.488603 * 0.488603 * 2.0 / 3.0;

    if (coefficient == 0)
    {
        fragColor = yupEncodeRadiance(l0, probe.params.y);
        return;
    }

    vec3 ratio = vec3(l0.r > 0.0 ? lc.r / (l0.r * 2.0) : 0.0, l0.g > 0.0 ? lc.g / (l0.g * 2.0) : 0.0, l0.b > 0.0 ? lc.b / (l0.b * 2.0) : 0.0);
    // rgba8unorm holds no exact 0.5: a zero ratio comes back as about 0.8% of coefficient 0
    fragColor = probe.params.y > 0.5 ? vec4(clamp(ratio * 0.5 + 0.5, 0.0, 1.0), 1.0) : vec4(clamp(lc, vec3(-yupMaxHalf), vec3(yupMaxHalf)), 1.0);
}
