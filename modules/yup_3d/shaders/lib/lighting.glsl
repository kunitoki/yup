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


#ifndef YUP_3D_LIB_LIGHTING
#define YUP_3D_LIB_LIGHTING

// The lighting inputs of the passes that shade, declared once: the set 1 LightingBlock (mirrored
// by LightingInputs::Block in yup_LightingInputs.h) and the set 0 linear and point samplers. The
// environment and its lookups are declared by lib/probes.glsl, the shadows by lib/shadows.glsl, and
// the lookup tables by lib/lights.glsl.

layout(set = 0, binding = 1) uniform sampler linearClamp;
layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform LightingBlock
{
    vec4 ambient;             // rgb linear ambient, w 1 when the environment shows behind the scene (exposure is view.cameraPosition.w)
    vec4 environmentInfo;     // x intensity (0 for none), y last level, z background blur level, w output mode (0 ldrInline, 1 hdrReversible, 2 hdrLinear)
    vec4 irradiance[9];       // rgb coefficients of the environment diffuse light
    vec4 clusterGrid;         // full tier: tiles x, tiles y, depth slices, number of lights
    vec4 clusterDepth;        // full tier: near, far, number of directional lights, 1 when the last slice is open (infinite far plane)
    vec4 probeInfo;           // x 1 when the environment, probe and volume textures hold RGBM; y, z reserved; w 1 when the frame has ambient occlusion (both tiers; LightingPass reads it)
} lighting;

#endif
