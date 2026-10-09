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

#include "lib/color.glsl"
#include "lib/view.glsl"
#include "lib/lighting.glsl"

// Shows the environment behind the scene of the raster tier, blurred to one of its roughness levels,
// exposed and written per output mode like the surfaces. The full tier shows it from LightingPass.

layout(location = 0) in vec2 v_clip;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 exposed = lightingBackground(v_clip);

    if (lighting.environmentInfo.w < 0.5)
        fragColor = vec4(yupLinearToSrgb(yupToneMap(exposed, view.clipPlanes.w)), 1.0);
    else if (lighting.environmentInfo.w < 1.5)
        fragColor = vec4(yupReversibleToneMap(min(exposed, vec3(yupMaxHalf))), 1.0);
    else
        fragColor = vec4(min(exposed, vec3(yupMaxHalf)), 1.0);
}
