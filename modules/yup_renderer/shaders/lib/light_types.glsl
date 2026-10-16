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


#ifndef YUP_3D_LIB_LIGHT_TYPES
#define YUP_3D_LIB_LIGHT_TYPES

// Mirrors GpuLight in yup_GpuSceneData.h
struct GpuLight
{
    vec4 positionType;     // xyz world position, w type (0 directional, 1 point, 2 spot)
    vec4 directionRange;   // xyz unit direction the light shines towards, w range: 0 unbounded (directional), > 0 the node range, < 0 minus the effective range
    vec4 colorIntensity;   // rgb linear color, w intensity
    vec4 spotRadiusShadow; // x cone scale, y cone offset, z radius, w first shadow record (lib/shadows.glsl) or -1
};

#endif
