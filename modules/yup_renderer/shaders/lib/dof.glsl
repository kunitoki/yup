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

#ifndef YUP_3D_LIB_DOF
#define YUP_3D_LIB_DOF

#include "lib/color.glsl"
#include "lib/view.glsl"

// The block and helpers the depth of field passes (DepthOfField) share. A circle of confusion (CoC)
// is a signed radius in texels of the half-resolution CoC texture: negative in front of the focus
// distance (the near field), positive behind it (the far field). Every texture is read by uv with
// sizes from textureSize, and the depth through the nearest pointClamp, since depth32float is not
// filterable everywhere.

#define YUP_3D_DOF_SAMPLES 81

layout(set = 0, binding = 1) uniform sampler linearClamp;
layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform DofBlock
{
    vec4 lens;                             // x CoC at infinity (the thin lens scale), y focus distance, z largest CoC, w 1 when the color is reversibly tone mapped
    vec4 tiles;                            // x tile size in CoC texels (the tiles pass: source texels per texel), y the tiles pass reach in source texels, z its source sign
    vec4 samples[YUP_3D_DOF_SAMPLES];      // xy the bokeh offsets over the unit aperture, z the radius of their ring
} dof;

// The texel of a texture of @p size holding @p position (uv from the top-left)
ivec2 dofTexel(vec2 position, ivec2 size)
{
    return clamp(ivec2(position * vec2(size)), ivec2(0), size - 1);
}

// A texel of the scene color, sanitized and, when reversibly tone mapped, inverted
vec4 dofLinearColor(vec4 texel)
{
    vec4 c = yupSanitize(texel);
    return dof.lens.w > 0.5 ? vec4(yupUnpremultiplyInverse(c), c.a) : c;
}

// The signed CoC of a stored depth, mirroring CameraNode::getCircleOfConfusion(): the thin lens
// scale * (1 - focus / distance), through the inverse of the view distance, which stays finite at
// depth 1 with an infinite far plane (where it is 0)
float dofCircleOfConfusion(float depth)
{
    mat4 p = view.projection;
    float inverseDistance = (depth * p[2][3] - p[2][2]) / (depth * p[3][3] - p[3][2]);
    return clamp(dof.lens.x * (1.0 - dof.lens.y * inverseDistance), -dof.lens.z, dof.lens.z);
}

#endif
