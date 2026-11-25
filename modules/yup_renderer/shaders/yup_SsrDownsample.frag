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

// One level of the radiance pyramid of the screen-space reflections (ReflectionPass) from the level
// above it, bound alone so that it is lod 0 here, or the first one from the previous scene color.
// Each texel is the mean of the 2 x 2 texels it covers, one bilinear tap where they meet: the color
// is premultiplied by its coverage, so the mean weighs each texel by it.

layout(set = 0, binding = 1) uniform sampler linearClamp;

layout(set = 1, binding = 1) uniform texture2D u_source;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = textureLod(sampler2D(u_source, linearClamp), uv, 0.0);
}
