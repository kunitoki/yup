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

// Mip 0 of the Hi-Z pyramid (HiZPyramid): the depth of the texel the pixel covers, read with
// texelFetch through the nearest pointClamp, since depth32float is not filterable everywhere.

layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform texture2D u_depth;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(texelFetch(sampler2D(u_depth, pointClamp), ivec2(gl_FragCoord.xy), 0).r, 0.0, 0.0, 0.0);
}
