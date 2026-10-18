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

#include "lib/color.glsl"

// Doubles the resolution of the bloom chain with a tent filter, scaled and added onto the target by
// the blend state: onto the level above within the chain, then into the cleared full resolution
// glow, scaled by the intensity there.

layout(set = 0, binding = 1) uniform sampler linearClamp;

layout(set = 1, binding = 0) uniform BloomUpsampleBlock
{
    vec4 params; // xy the tent step in uv (a source texel scaled by the radius), z the scale of the result
} bloom;

layout(set = 1, binding = 1) uniform texture2D u_source;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

vec3 fetch(vec2 offset)
{
    return textureLod(sampler2D(u_source, linearClamp), uv + offset * bloom.params.xy, 0.0).rgb;
}

// Bloom upsample after Jimenez 2014, "Next Generation Post Processing in Call of Duty: Advanced
// Warfare" (SIGGRAPH Advances in Real-Time Rendering): a 3x3 tent whose footprint follows the radius
void main()
{
    vec3 glow = fetch(vec2(0.0, 0.0)) * 4.0
              + (fetch(vec2(-1.0, 0.0)) + fetch(vec2(1.0, 0.0)) + fetch(vec2(0.0, -1.0)) + fetch(vec2(0.0, 1.0))) * 2.0
              + fetch(vec2(-1.0, -1.0)) + fetch(vec2(1.0, -1.0)) + fetch(vec2(-1.0, 1.0)) + fetch(vec2(1.0, 1.0));

    // A large intensity would overflow the half float target to infinity
    fragColor = vec4(min(glow * (bloom.params.z / 16.0), vec3(yupMaxHalf)), 0.0);
}
