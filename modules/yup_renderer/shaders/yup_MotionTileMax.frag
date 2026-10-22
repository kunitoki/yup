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

#include "lib/motion.glsl"

// The tiles of the motion blur, by steps: each texel stores the longest motion in xy and the
// length of the shortest in z, of the 4 x 4 pixels of the scene color it covers, read from the
// velocity by uv at their centers (motion.options.y 1), or of the 4 x 4 texels of the previous step
// it covers. Drawn twice, one texel ends up covering YUP_3D_MOTION_TILE_SIZE x
// YUP_3D_MOTION_TILE_SIZE pixels: small windows keep every texel cheap. The scene color is bound
// for its size only, which sets the tile grid.

layout(set = 1, binding = 1) uniform texture2D u_source;
layout(set = 1, binding = 2) uniform texture2D u_color;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outTile;

void main()
{
    ivec2 sourceSize = textureSize(sampler2D(u_source, pointClamp), 0);
    ivec2 colorSize = textureSize(sampler2D(u_color, pointClamp), 0);
    bool readsVelocity = motion.options.y > 0.5;

    // The texel's own block, by construction of the ceil-sized targets, clamped to its source
    ivec2 first = ivec2(gl_FragCoord.xy) * 4;
    ivec2 last = min(first + 3, (readsVelocity ? colorSize : sourceSize) - 1);
    vec2 longest = vec2(0.0);
    float shortest = float(YUP_3D_MOTION_TILE_SIZE);

    for (int y = first.y; y <= last.y; ++y)
    {
        for (int x = first.x; x <= last.x; ++x)
        {
            vec4 source;
            if (readsVelocity)
            {
                ivec2 texel = motionTexel((vec2(x, y) + 0.5) / vec2(colorSize), sourceSize);
                source = vec4(motionOf(texelFetch(sampler2D(u_source, pointClamp), texel, 0).xy, vec2(colorSize)), 0.0, 0.0);
            }
            else
            {
                source = texelFetch(sampler2D(u_source, pointClamp), ivec2(x, y), 0);
            }

            if (dot(source.xy, source.xy) > dot(longest, longest))
                longest = source.xy;

            shortest = min(shortest, readsVelocity ? length(source.xy) : source.z);
        }
    }

    outTile = vec4(longest, shortest, 0.0);
}
