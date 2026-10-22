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

// Copies the single mip level the source view exposes, bilinearly filtered, into the next level.

layout(set = 0, binding = 0) uniform texture2D u_source;
layout(set = 0, binding = 1) uniform sampler s_source;

// The destination pixel maps to the same fractional position in the source on every backend, since
// the framebuffer origin and the first texture row follow the same convention.
layout(set = 0, binding = 2) uniform MipData
{
    vec4 info; // xy reciprocal of the destination level size
} mip;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = textureLod(sampler2D(u_source, s_source), gl_FragCoord.xy * mip.info.xy, 0.0);
}
