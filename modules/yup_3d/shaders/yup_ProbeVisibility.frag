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

#include "lib/bsdf.glsl"
#include "lib/sampling.glsl"
#include "lib/volumes.glsl"

// Writes the visibility moments of an irradiance volume probe into its bordered octahedral tile
// (lib/volumes.glsl): per texel direction, the mean distance and mean squared distance to the
// surrounding geometry, read from the probe's distance cube and scaled to the cell diagonal, under a
// cosine power lobe around the direction (McGuire et al. 2019, "Dynamic Diffuse Global Illumination
// with Ray-Traced Irradiance Fields", JCGT 8(2), whose rays are weighted by cos^50). The lobe is
// importance sampled with 64 Hammersley points, so each sample weighs the same. The border texels
// compute the interior texel they mirror, so they match it exactly. The pass is drawn with the
// viewport on the tile. Distances are kept within 200 cell diagonals, so the squared distance fits
// a half float.

layout(set = 0, binding = 1) uniform sampler linearClamp;

layout(set = 1, binding = 0) uniform ProbeVisibilityBlock
{
    vec4 params; // x tile size without its border, in texels, y 1 / the cell diagonal, z samples, w cosine power
} probe;

// The radial distance from the probe along each direction, in world units, in x
layout(set = 1, binding = 1) uniform textureCube u_distance;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    int size = int(probe.params.x);
    ivec2 texel = clamp(ivec2(uv * float(size + 2)), ivec2(0), ivec2(size + 1));
    vec3 n = volumeTileDirection(texel, size);
    mat3 frame = yupTangentFrame(n);

    int numSamples = int(probe.params.z);
    float exponent = 1.0 / (probe.params.w + 1.0);
    vec2 moments = vec2(0.0);

    for (int i = 0; i < numSamples; ++i)
    {
        // pdf proportional to cos^power: cos theta = u^(1 / (power + 1)), with u.x at the centers of
        // its n strata, (i + 0.5) / n: yupHammersley's i / n would put the first sample at u.x = 0,
        // 40 degrees off the lobe's axis
        vec2 u = vec2((float(i) + 0.5) / float(numSamples), yupHammersley(i, numSamples).y);
        float cosTheta = pow(u.x, exponent);
        float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
        float phi = 2.0 * yupPi * u.y;
        vec3 d = frame * vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

        float distance = min(textureLod(samplerCube(u_distance, linearClamp), d, 0.0).x * probe.params.y, 200.0);
        moments += vec2(distance, distance * distance);
    }

    fragColor = vec4(moments / float(max(numSamples, 1)), 0.0, 1.0);
}
