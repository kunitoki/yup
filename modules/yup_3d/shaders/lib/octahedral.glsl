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

#endif
