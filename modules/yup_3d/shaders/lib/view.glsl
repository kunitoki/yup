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


#ifndef YUP_3D_LIB_VIEW
#define YUP_3D_LIB_VIEW

// The per-view uniforms shared by every pass, at set 0, binding 0. Fields are append-only.
layout(set = 0, binding = 0) uniform ViewBlock
{
    mat4 view;
    mat4 projection;               // jittered on the full tier
    mat4 viewProjection;           // jittered
    mat4 inverseViewProjection;    // jittered
    mat4 unjitteredViewProjection;
    mat4 previousViewProjection;   // unjittered, previous frame
    vec4 cameraPosition;           // xyz, w = exposure
    vec4 viewport;                 // width, height, 1/width, 1/height
    vec4 jitter;                   // xy current, zw previous, in NDC units
    vec4 clipPlanes;               // near, far, frame index, tone mapping operator (0 Reinhard, 1 ACES, 2 AgX)
} view;

#endif
