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

// Packs the depth into the four 8-bit channels of an rgba8unorm target, which every backend can
// render to and sample with plain filtering.

layout(location = 0) in float v_depth;

layout(location = 0) out vec4 fragColor;

void main()
{
    float depth = clamp(v_depth, 0.0, 0.99999);

    vec4 packed = fract(vec4(1.0, 255.0, 65025.0, 16581375.0) * depth);
    packed -= packed.yzww * vec4(1.0 / 255.0, 1.0 / 255.0, 1.0 / 255.0, 0.0);

    fragColor = packed;
}
