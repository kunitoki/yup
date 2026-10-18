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

// The longest motion of the 3 x 3 tiles around each tile. No pixel moves farther than a tile, so
// these are all the tiles whose smear can reach this one. A diagonal tile only reaches it moving
// within 45 degrees of the diagonal, either way since a smear spans both sides of its pixels: the
// others are left out, so they do not steer the tile's samples away from its own motion. z keeps
// the length of the shortest motion of all 9, which tells the reconstruction where all move alike.

layout(set = 1, binding = 1) uniform texture2D u_tiles;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outTile;

void main()
{
    ivec2 size = textureSize(sampler2D(u_tiles, pointClamp), 0);
    ivec2 tile = ivec2(gl_FragCoord.xy);
    vec2 longest = vec2(0.0);
    float shortest = float(YUP_3D_MOTION_TILE_SIZE);

    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            ivec2 neighbor = tile + ivec2(x, y);
            if (any(lessThan(neighbor, ivec2(0))) || any(greaterThanEqual(neighbor, size)))
                continue;

            vec4 around = texelFetch(sampler2D(u_tiles, pointClamp), neighbor, 0);
            vec2 m = around.xy;
            shortest = min(shortest, around.z);

            // |cos| of the angle to the diagonal (x, y), of length sqrt (2), above 1 / sqrt (2)
            if (x != 0 && y != 0 && abs(dot(m, vec2(x, y))) <= length(m))
                continue;

            if (dot(m, m) > dot(longest, longest))
                longest = m;
        }
    }

    outTile = vec4(longest, shortest, 0.0);
}
