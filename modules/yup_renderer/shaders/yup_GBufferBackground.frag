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

#include "lib/view.glsl"

// The G-buffer texels no surface covered: no material, and the motion of the environment, which
// only the camera moves. Drawn after the surfaces at the far plane with a lessEqual depth test, so
// it only reaches texels whose depth is still cleared.

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outAlbedoOcclusion;
layout(location = 1) out vec4 outNormalRoughness;
layout(location = 2) out vec4 outEmissionMaterial;
layout(location = 3) out vec2 outVelocity;

void main()
{
    outAlbedoOcclusion = vec4(0.0);
    outNormalRoughness = vec4(0.0);
    outEmissionMaterial = vec4(0.0);
    outVelocity = viewBackgroundVelocity(vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0));
}
