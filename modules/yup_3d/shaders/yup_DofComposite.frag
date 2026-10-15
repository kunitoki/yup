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

// The last depth of field pass, at the size of the scene color: the far field, upsampled bilinearly,
// replaces the sharp color as the CoC of the pixel grows past a texel, then the near field covers the
// result by its coverage. All four channels mix, so silhouettes keep their blend with the background.

layout(set = 1, binding = 1) uniform texture2D u_color;
layout(set = 1, binding = 2) uniform texture2D u_depth;
layout(set = 1, binding = 3) uniform texture2D u_far;
layout(set = 1, binding = 4) uniform texture2D u_near;
layout(set = 1, binding = 5) uniform texture2D u_coverage;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec4 sharp = dofLinearColor(texelFetch(sampler2D(u_color, pointClamp), dofTexel(uv, textureSize(sampler2D(u_color, pointClamp), 0)), 0));
    float depth = texelFetch(sampler2D(u_depth, pointClamp), dofTexel(uv, textureSize(sampler2D(u_depth, pointClamp), 0)), 0).r;

    vec4 far = textureLod(sampler2D(u_far, linearClamp), uv, 0.0);
    vec4 near = textureLod(sampler2D(u_near, linearClamp), uv, 0.0);
    float coverage = textureLod(sampler2D(u_coverage, linearClamp), uv, 0.0).r;

    vec4 color = mix(sharp, far, smoothstep(0.5, 1.5, abs(dofCircleOfConfusion(depth))));
    fragColor = color * (1.0 - coverage) + near;
}
