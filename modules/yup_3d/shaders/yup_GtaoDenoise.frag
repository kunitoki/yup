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

#include "lib/gtao.glsl"

// One 1D pass of the separable denoise of the ambient occlusion trace, along gtao.denoise.xy:
// a bilateral mean of 5 texels centered on the texel, the two ends weighing half, so the pair of
// passes averages every rotation of the trace's 4x4 pattern once with no shift. A neighbor counts
// by how close it lies to the tangent plane of the texel, relative to the texel's distance from the
// camera, and by how little its normal turns from the texel's, so the occlusion does not bleed
// across depth or normal edges. The mean bent normal is renormalized.

layout(set = 1, binding = 3) uniform texture2D u_trace;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    ivec2 outputPixel = ivec2(gl_FragCoord.xy);
    ivec2 pixel = gtaoInputPixel(outputPixel);
    vec4 center = texelFetch(sampler2D(u_trace, pointClamp), outputPixel, 0);
    float depth = gtaoDepth(pixel);

    if (depth >= 1.0)
    {
        fragColor = center;
        return;
    }

    // The normals compare in world space, where they are stored, the tangent plane in view space
    vec3 position = gtaoViewPosition(pixel, depth);
    vec3 normal = gtaoNormal(pixel);
    vec3 viewNormal = normalize(mat3(view.view) * normal);
    float tolerance = max(0.02 * length(position), 1.0e-6);
    ivec2 size = textureSize(sampler2D(u_trace, pointClamp), 0);
    ivec2 direction = ivec2(gtao.denoise.xy);

    // The texel itself weighs 1, so the weights never sum to 0
    vec4 sum = vec4(0.0);
    float weights = 0.0;

    for (int i = -2; i <= 2; ++i)
    {
        ivec2 neighbor = clamp(outputPixel + direction * i, ivec2(0), size - 1);
        ivec2 neighborPixel = gtaoInputPixel(neighbor);
        float neighborDepth = gtaoDepth(neighborPixel);
        if (neighborDepth >= 1.0)
            continue;

        vec3 toNeighbor = gtaoViewPosition(neighborPixel, neighborDepth) - position;
        float plane = clamp(1.0 - abs(dot(toNeighbor, viewNormal)) / tolerance, 0.0, 1.0);
        float facing = max(dot(gtaoNormal(neighborPixel), normal), 0.0);
        facing *= facing;
        facing *= facing;
        facing *= facing;

        float weight = (abs(i) == 2 ? 0.5 : 1.0) * plane * facing;
        sum += texelFetch(sampler2D(u_trace, pointClamp), neighbor, 0) * weight;
        weights += weight;
    }

    vec4 result = sum / weights;
    vec3 bentNormal = result.rgb * 2.0 - 1.0;
    float bentLength = length(bentNormal);
    fragColor = vec4(bentLength > 1.0e-4 ? bentNormal / bentLength * 0.5 + 0.5 : center.rgb, result.a);
}
