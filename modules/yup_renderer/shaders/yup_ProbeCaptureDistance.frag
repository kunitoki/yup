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

// Writes the radial distance of an irradiance volume probe's capture, drawn by the ForwardPass into
// a 2D target with its depth, into one face of the probe's distance cube: the depth is linearized
// with the capture projection, then scaled by the length of the ray through the texel center, so
// it measures the distance from the probe instead of along the face's axis. Texels nothing covered
// keep the far plane. Mirrored left to right like the color, see yup_ProbeCaptureResolve.frag.

layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform ProbeDistanceBlock
{
    vec4 projection; // the capture projection's m22, m32, m23 and m33 (column, row), linearizing its depth
} capture;

layout(set = 1, binding = 1) uniform texture2D u_depth;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    ivec2 size = textureSize(sampler2D(u_depth, pointClamp), 0);
    vec2 st = vec2(1.0 - uv.x, uv.y);
    ivec2 texel = clamp(ivec2(st * vec2(size)), ivec2(0), size - 1);
    float depth = texelFetch(sampler2D(u_depth, pointClamp), texel, 0).r;

    vec4 p = capture.projection;
    float alongAxis = abs((p.y - depth * p.w) / (depth * p.z - p.x));

    // A 90 degree face: the ray through a texel center leaves the axis by its face coordinates
    vec2 c = (vec2(texel) + 0.5) / vec2(size) * 2.0 - 1.0;
    float distance = alongAxis * length(vec3(c, 1.0));

    fragColor = vec4(distance, 0.0, 0.0, 1.0);
}
