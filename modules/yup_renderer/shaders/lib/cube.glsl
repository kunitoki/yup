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

#ifndef YUP_3D_LIB_CUBE
#define YUP_3D_LIB_CUBE

// Cube map faces in the order and orientation every GPU API samples them: +X, -X, +Y, -Y, +Z, -Z,
// with st running 0..1 across a face, s along its columns and t along its rows (the cube map face
// selection table of the OpenGL specification).

// The unit direction through @p st of a face
vec3 cubeDirection(int face, vec2 st)
{
    vec2 c = st * 2.0 - 1.0;

    if (face == 0)
        return normalize(vec3(1.0, -c.y, -c.x));

    if (face == 1)
        return normalize(vec3(-1.0, -c.y, c.x));

    if (face == 2)
        return normalize(vec3(c.x, 1.0, c.y));

    if (face == 3)
        return normalize(vec3(c.x, -1.0, -c.y));

    if (face == 4)
        return normalize(vec3(c.x, -c.y, 1.0));

    return normalize(vec3(-c.x, -c.y, -1.0));
}

// The face a unit direction falls in and its st there, the inverse of cubeDirection()
vec2 cubeFaceCoordinates(vec3 d, out int face)
{
    vec3 a = abs(d);
    vec2 c;
    float major;

    if (a.x >= a.y && a.x >= a.z)
    {
        face = d.x > 0.0 ? 0 : 1;
        major = a.x;
        c = d.x > 0.0 ? vec2(-d.z, -d.y) : vec2(d.z, -d.y);
    }
    else if (a.y >= a.z)
    {
        face = d.y > 0.0 ? 2 : 3;
        major = a.y;
        c = d.y > 0.0 ? vec2(d.x, d.z) : vec2(d.x, -d.z);
    }
    else
    {
        face = d.z > 0.0 ? 4 : 5;
        major = a.z;
        c = d.z > 0.0 ? vec2(d.x, -d.y) : vec2(-d.x, -d.y);
    }

    return c / major * 0.5 + 0.5;
}

// The solid angle seen from the center of the cube by the face rectangle from (0, 0) to (x, y), in
// face coordinates -1..1. Derived from the area element: a face point (u, v, 1) lies at distance
// r = sqrt (1 + u^2 + v^2) and is seen at cosine 1 / r, so dw = du dv / r^3, and integrating it over
// [0, x] x [0, y] gives atan (x y / sqrt (1 + x^2 + y^2))
float cubeAreaElement(float x, float y)
{
    return atan(x * y, sqrt(x * x + y * y + 1.0));
}

// The solid angle of the texel centered at @p st on faces of @p size texels
float cubeTexelSolidAngle(vec2 st, float size)
{
    vec2 a = st * 2.0 - 1.0 - 1.0 / size;
    vec2 b = a + 2.0 / size;
    return cubeAreaElement(a.x, a.y) - cubeAreaElement(a.x, b.y) - cubeAreaElement(b.x, a.y) + cubeAreaElement(b.x, b.y);
}

#endif
