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

// The first depth of field pass, at half the depth's size: each texel averages the 2x2 footprint of
// the scene color, premultiplied by its coverage, sanitized texel by texel so one bad value cannot
// spread through the blur, and stores the CoC of the closest of the four depths.

layout(set = 1, binding = 1) uniform texture2D u_color;
layout(set = 1, binding = 2) uniform texture2D u_depth;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outCircleOfConfusion;

void main()
{
    ivec2 colorSize = textureSize(sampler2D(u_color, pointClamp), 0);
    ivec2 depthSize = textureSize(sampler2D(u_depth, pointClamp), 0);

    vec4 color = vec4(0.0);
    float closest = 1.0;

    for (int i = 0; i < 4; ++i)
    {
        vec2 offset = vec2(float(i & 1), float(i >> 1)) - 0.5;
        color += dofLinearColor(texelFetch(sampler2D(u_color, pointClamp), clamp(ivec2(floor(uv * vec2(colorSize) + offset)), ivec2(0), colorSize - 1), 0));
        closest = min(closest, texelFetch(sampler2D(u_depth, pointClamp), clamp(ivec2(floor(uv * vec2(depthSize) + offset)), ivec2(0), depthSize - 1), 0).r);
    }

    outColor = color * 0.25;
    outCircleOfConfusion = vec4(dofCircleOfConfusion(closest), 0.0, 0.0, 0.0);
}
