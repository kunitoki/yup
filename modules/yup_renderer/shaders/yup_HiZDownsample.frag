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

// Mip n of the Hi-Z pyramid (HiZPyramid) from mip n - 1, bound alone so that it is lod 0 here. Each
// texel keeps the closest (minimum) depth of the texels of mip n - 1 it covers: 2 per axis, or 3
// along an axis where mip n - 1 has an odd extent, since texel i then spans more than texels 2i and
// 2i + 1. Reads past the last texel (an extent of 1) are clamped to it.

layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform HiZBlock
{
    vec4 params; // xy the extent of mip n - 1, in texels
} hiz;

layout(set = 1, binding = 1) uniform texture2D u_source;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    ivec2 extent = ivec2(hiz.params.xy);
    ivec2 last = extent - 1;
    ivec2 first = ivec2(gl_FragCoord.xy) * 2;
    ivec2 taps = 2 + (extent & 1);

    float closest = texelFetch(sampler2D(u_source, pointClamp), min(first, last), 0).r;

    for (int j = 0; j < taps.y; ++j)
    {
        for (int i = 0; i < taps.x; ++i)
            closest = min(closest, texelFetch(sampler2D(u_source, pointClamp), min(first + ivec2(i, j), last), 0).r);
    }

    fragColor = vec4(closest, 0.0, 0.0, 0.0);
}
