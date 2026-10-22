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

#ifndef YUP_3D_LIB_VOLUMES
#define YUP_3D_LIB_VOLUMES

#include "lib/octahedral.glsl"

// The octahedral visibility tiles of the irradiance volume probes (McGuire et al. 2019, "Dynamic
// Diffuse Global Illumination with Ray-Traced Irradiance Fields", JCGT 8(2)): @p size x @p size
// texels covering the octahedron edge to edge, their centers at (i + 0.5) / size, inside a one texel
// border. Each border texel holds the interior texel across the octahedron edge it lies on, mirrored
// along that edge (the diagonally opposite corner for a corner), so a bilinear lookup next to an edge
// blends the directions that surround it there. ProbeRenderer writes the tiles with
// volumeTileDirection(), the shading reads them with volumeTileUv().

// The unit direction a texel of a bordered tile holds, @p texel from (0, 0) to (size + 1, size + 1):
// the direction of its own center for an interior texel, of the interior texel it mirrors for a
// border texel
vec3 volumeTileDirection(ivec2 texel, int size)
{
    int last = size + 1;

    if (texel.y == 0 || texel.y == last)
        texel = ivec2(last - texel.x, texel.y == 0 ? 1 : size);

    if (texel.x == 0 || texel.x == last)
        texel = ivec2(texel.x == 0 ? 1 : size, last - texel.y);

    return octahedralDecode((vec2(texel - 1) + 0.5) / float(size) * 2.0 - 1.0);
}

// The atlas uv of a unit direction in the bordered tile whose top-left border texel is @p origin,
// in texels, with @p size texels inside the border
vec2 volumeTileUv(vec3 direction, vec2 origin, float size, vec2 atlasSize)
{
    return (origin + 1.0 + (octahedralEncode(direction) * 0.5 + 0.5) * size) / atlasSize;
}

#endif
