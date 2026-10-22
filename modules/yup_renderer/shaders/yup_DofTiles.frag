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

// The tiles of the depth of field, by steps: each texel stores the largest near CoC (as a positive
// radius) of the dof.tiles.x x dof.tiles.x source texels it covers and of dof.tiles.y more around
// them. dof.tiles.z is -1 when the source holds signed CoC, 1 when it holds such radii already. Two
// 4 x 4 reductions make the tiles, and a 3 x 3 one spreads them over their neighbors, so a near
// field blurring over in-focus texels is gathered there too: small windows keep every texel cheap.

layout(set = 1, binding = 1) uniform texture2D u_source;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outTile;

void main()
{
    ivec2 size = textureSize(sampler2D(u_source, pointClamp), 0);
    int scale = int(dof.tiles.x);
    int reach = int(dof.tiles.y);

    // A reduction: the texel's own source block, by construction of the ceil-sized targets, clamped to the source
    ivec2 first = max(ivec2(gl_FragCoord.xy) * scale - reach, ivec2(0));
    ivec2 last = min(ivec2(gl_FragCoord.xy) * scale + (scale - 1 + reach), size - 1);
    float nearest = 0.0;

    for (int y = first.y; y <= last.y; ++y)
    {
        for (int x = first.x; x <= last.x; ++x)
            nearest = max(nearest, dof.tiles.z * texelFetch(sampler2D(u_source, pointClamp), ivec2(x, y), 0).r);
    }

    outTile = vec4(nearest, 0.0, 0.0, 0.0);
}
