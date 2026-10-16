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

#include "lib/dof.glsl"

// The bokeh gather of the depth of field, at half resolution, over a fixed pattern of
// YUP_3D_DOF_SAMPLES offsets spread over the largest CoC that can reach the texel. Each offset is
// read where it lands mirrored, so the bokeh keeps the orientation of the aperture. Two fields:
// - far: the texels at or behind the focus whose CoC, limited to the CoC of this texel (what lies
//   behind it cannot spread over it), covers the distance, averaged with the weight 1 / CoC^2 of a
//   scattered disk;
// - near: the texels in front of the focus whose CoC covers the distance, each weighing the share of
//   the aperture it stands for over the area of its disk, so a texel inside a uniform near field
//   sums to 1: twice that sum, at most 1, is the near coverage.
// A source covers an offset when its CoC reaches the radius of the offset's ring, the disk radius
// before the polygon mapping, so each source spreads as the aperture's polygon at its own CoC.
// Every color is premultiplied rgba, so the coverage alpha blurs with the color.

layout(set = 1, binding = 1) uniform texture2D u_color;
layout(set = 1, binding = 2) uniform texture2D u_circleOfConfusion;
layout(set = 1, binding = 3) uniform texture2D u_tiles;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outFar;
layout(location = 1) out vec4 outNear;
layout(location = 2) out vec4 outCoverage;

// Scatter-as-gather depth of field after Jimenez 2014, "Next Generation Post Processing in Call of
// Duty: Advanced Warfare" (SIGGRAPH Advances in Real-Time Rendering)
void main()
{
    ivec2 cocSize = textureSize(sampler2D(u_circleOfConfusion, pointClamp), 0);
    ivec2 pixel = dofTexel(uv, cocSize);
    float center = texelFetch(sampler2D(u_circleOfConfusion, pointClamp), pixel, 0).r;
    vec4 centerColor = texelFetch(sampler2D(u_color, pointClamp), dofTexel(uv, textureSize(sampler2D(u_color, pointClamp), 0)), 0);

    ivec2 tile = min(pixel / int(dof.tiles.x), textureSize(sampler2D(u_tiles, pointClamp), 0) - 1);
    float radius = min(max(texelFetch(sampler2D(u_tiles, pointClamp), tile, 0).r, center), dof.lens.z);

    // Nothing within reach blurs by a texel or more: the texel is in focus
    if (radius < 0.5)
    {
        outFar = centerColor;
        outNear = vec4(0.0);
        outCoverage = vec4(0.0);
        return;
    }

    // The share of the aperture each offset stands for, as the square of a radius
    float share = radius * radius / float(YUP_3D_DOF_SAMPLES);
    vec2 stride = radius / vec2(cocSize);
    float centerFar = max(center, 0.0);

    vec4 far = vec4(0.0);
    float farWeight = 0.0;
    vec4 near = vec4(0.0);
    float nearWeight = 0.0;

    for (int i = 0; i < YUP_3D_DOF_SAMPLES; ++i)
    {
        vec4 offset = dof.samples[i];
        float reach = offset.z * radius;
        vec2 position = uv - offset.xy * stride;

        vec4 color = textureLod(sampler2D(u_color, linearClamp), position, 0.0);
        float coc = textureLod(sampler2D(u_circleOfConfusion, pointClamp), position, 0.0).r;

        float farCoc = min(max(coc, 0.0), centerFar);
        float w = step(reach, farCoc) / max(farCoc * farCoc, share);
        far += color * w;
        farWeight += w;

        float nearCoc = -coc;
        w = step(reach, nearCoc) * smoothstep(0.5, 1.5, nearCoc) * share / max(nearCoc * nearCoc, share);
        near += color * w;
        nearWeight += w;
    }

    // About half the aperture covers a texel on the silhouette of a uniform near field, inside it as
    // well as outside: doubling the coverage keeps the near field opaque up to its silhouette, so it
    // fades out over the blur outside instead of stepping down at the edge (Sousa 2013)
    float coverage = min(2.0 * nearWeight, 1.0);

    outFar = far / max(farWeight, 1.0e-6);
    outNear = near / max(nearWeight, 1.0e-6) * coverage;
    outCoverage = vec4(coverage, 0.0, 0.0, 0.0);
}
