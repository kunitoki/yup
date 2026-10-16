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

#include "lib/gbuffer.glsl"

// Writes the depth and the G-buffer RT1 of the opaque and masked surfaces for the raster
// screen-space effects with gbufferEncodeNormalRoughness(), as yup_GBufferPass.frag does.
// materialEvaluate() cuts out the masked texels.

layout(location = 0) out vec4 outNormalRoughness;

void main()
{
    outNormalRoughness = gbufferEncodeNormalRoughness(materialEvaluate());
}
