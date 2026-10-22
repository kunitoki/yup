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

#ifndef YUP_3D_LIB_GBUFFER
#define YUP_3D_LIB_GBUFFER

#include "lib/material.glsl"
#include "lib/octahedral.glsl"

// The G-buffer encodings shared by the passes writing them (GBufferPass, DepthNormalPrepass).
// Fragment stage only. Kept out of lib/material.glsl so the other passes compile without it.

// The RT1 of a surface: octahedral shading normal (Cigolle et al. 2014, "A Survey of Efficient
// Representations for Independent Unit Vectors"), the roughness widened by the normal variance
// within the pixel (Tokuyoshi and Kaplanyan 2019, "Improved Geometric Specular Antialiasing"),
// metallic
vec4 gbufferEncodeNormalRoughness(MaterialSurface s)
{
    return vec4(octahedralEncode(s.n), sqrt(sqrt(s.alpha2)), s.metallic);
}

#endif
