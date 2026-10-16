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

#ifndef YUP_3D_LIB_OCTAHEDRAL
#define YUP_3D_LIB_OCTAHEDRAL

// Octahedral unit vector encoding (Cigolle et al. 2014, "A Survey of Efficient Representations for
// Independent Unit Vectors"). The sphere is projected on the octahedron |x| + |y| + |z| = 1, whose
// lower half is folded over the upper one into the [-1, 1] square.
// Mirrors GpuSceneData::encodeOctahedral and decodeOctahedral in yup_GpuSceneData.cpp.

vec2 octahedralSignNotZero(vec2 v)
{
    return vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

// Unit vector to [-1, 1]^2
vec2 octahedralEncode(vec3 n)
{
    vec2 p = n.xy / (abs(n.x) + abs(n.y) + abs(n.z));
    if (n.z < 0.0)
        p = (1.0 - abs(p.yx)) * octahedralSignNotZero(p);

    return p;
}

// [-1, 1]^2 to unit vector
vec3 octahedralDecode(vec2 e)
{
    vec3 n = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (n.z < 0.0)
        n.xy = (1.0 - abs(e.yx)) * octahedralSignNotZero(e);

    return normalize(n);
}

// Atlas uv of a unit direction in a square octahedral tile whose mips are aligned to it: mip m of
// the tile is @p tile (x, y, width, height at mip 0, in texels) with every coordinate shifted right
// by m. The texel centers of each mip span the octahedron edge to edge, a half-texel inset on every
// side, so a bilinear lookup at that whole mip never leaves the tile. The inset differs per mip, so
// a lod between mips must not use one uv for both: read it with octahedralTileTaps. Mirrors the tile
// writes of ProbeRenderer.
vec2 octahedralTileUv(vec3 direction, vec4 tile, vec2 atlasSize, int mip)
{
    float scale = exp2(float(mip));
    float size = tile.z / scale;
    vec2 texel = 0.5 + (octahedralEncode(direction) * 0.5 + 0.5) * (size - 1.0);
    return (tile.xy + texel * scale) / atlasSize;
}

// A lod of an octahedral tile of @p numMips mips, between whole mips: two taps at the whole mips
// around it, each with the uv of its own mip, mixed by the fraction. Read it as
// mix (textureLod (atlas, uv0, lod0), textureLod (atlas, uv1, lod1), weight), decoding each tap
// first when the atlas holds RGBM.
struct OctahedralTileTaps
{
    vec2 uv0;     // at the whole mip lod0
    vec2 uv1;     // at the whole mip lod1
    float lod0;
    float lod1;
    float weight; // of the second tap
};

OctahedralTileTaps octahedralTileTaps(vec3 direction, vec4 tile, vec2 atlasSize, float lod, int numMips)
{
    float clamped = clamp(lod, 0.0, float(numMips - 1));
    int mip0 = int(floor(clamped));
    int mip1 = min(mip0 + 1, numMips - 1);

    OctahedralTileTaps taps;
    taps.uv0 = octahedralTileUv(direction, tile, atlasSize, mip0);
    taps.uv1 = octahedralTileUv(direction, tile, atlasSize, mip1);
    taps.lod0 = float(mip0);
    taps.lod1 = float(mip1);
    taps.weight = clamped - float(mip0);
    return taps;
}

#endif
