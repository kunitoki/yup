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

#ifndef YUP_3D_LIB_FORWARD
#define YUP_3D_LIB_FORWARD

#include "lib/color.glsl"
#include "lib/view.glsl"
#include "lib/lighting.glsl"
#include "lib/material.glsl"

// The outputs of the fragment stages ForwardPass draws meshes with (yup_ForwardPass.frag, and
// yup_MaterialShader.frag for the meshes of a material shader), and forwardWrite(), which writes
// them from the color of the surface. Fragment stage only.

layout(location = 0) out vec4 fragColor;

#ifdef YUP_3D_TIER_FULL
layout(location = 1) out vec2 outVelocity;
#endif

// Writes the fragment from its linear, scene-referred `color`: exposed, then per output mode
// (LightingBlock.environmentInfo.w, ForwardPass::Output) tone mapped (Reinhard, ACES or AgX) and
// sRGB encoded for an rgba8unorm target, reversibly tone mapped, or linear, the last two within the
// half float range. `alpha` is written as it is, and on the full tier the screen-space motion of
// the fragment with it
void forwardWrite(vec3 color, float alpha)
{
    vec3 exposed = color * view.cameraPosition.w;

    if (lighting.environmentInfo.w < 0.5)
        fragColor = vec4(yupLinearToSrgb(yupToneMap(exposed, view.clipPlanes.w)), alpha);
    else if (lighting.environmentInfo.w < 1.5)
        fragColor = vec4(yupReversibleToneMap(min(exposed, vec3(yupMaxHalf))), alpha);
    else
        fragColor = vec4(min(exposed, vec3(yupMaxHalf)), alpha);

#ifdef YUP_3D_TIER_FULL
    outVelocity = viewVelocity(v_currentClip, v_previousClip);
#endif
}

#endif
