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

#ifndef YUP_3D_LIB_MOTION
#define YUP_3D_LIB_MOTION

#include "lib/color.glsl"
#include "lib/view.glsl"

// The block and helpers the motion blur passes (MotionBlur) share. The motion of a pixel is its
// velocity in pixels of the scene color, scaled by the shutter and halved: the smear spans it on
// both sides of the pixel, so it is the radius of the smear, at most the tile size. Every texture
// is read by uv with sizes from textureSize, and the depth through the nearest pointClamp, since
// depth32float is not filterable everywhere.

// MotionBlur::tileSize: the pixels of the color per side of a tile, and the largest motion
#define YUP_3D_MOTION_TILE_SIZE 16

// MotionBlur::numSamples: the taps of the reconstruction besides the pixel itself
#define YUP_3D_MOTION_SAMPLES 12

layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform MotionBlurBlock
{
    vec4 options; // x the shutter, y 1 when the tiles pass reads the velocity, z 1 when u_blurAmount holds the depth of field's share
} motion;

// The texel of a texture of @p size holding @p position (uv from the top-left)
ivec2 motionTexel(vec2 position, ivec2 size)
{
    return clamp(ivec2(position * vec2(size)), ivec2(0), size - 1);
}

// The motion of a velocity texel (current uv minus previous uv) in pixels of a color of
// @p colorSize: non-finite velocities do not move, and the length is clamped, keeping the direction
vec2 motionOf(vec2 velocity, vec2 colorSize)
{
    vec2 finite = mix(velocity, vec2(0.0), yupIsNonFinite(vec4(velocity, 0.0, 0.0)).xy);
    vec2 m = finite * colorSize * (0.5 * motion.options.x);
    float len = length(m);
    return len > float(YUP_3D_MOTION_TILE_SIZE) ? m * (float(YUP_3D_MOTION_TILE_SIZE) / len) : m;
}

#endif
