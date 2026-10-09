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
#include "lib/material.glsl"
#include "lib/octahedral.glsl"
#include "lib/view.glsl"

// Writes the material of opaque and masked surfaces for the lighting pass (full tier only):
// - RT0 rgba8: sRGB-encoded base color, material occlusion;
// - RT1 rgba16f: octahedral shading normal (Cigolle et al. 2014, "A Survey of Efficient
//   Representations for Independent Unit Vectors"), the roughness widened by the normal variance
//   within the pixel (Tokuyoshi and Kaplanyan 2019, "Improved Geometric Specular Antialiasing"),
//   which needs the derivatives the lighting pass doesn't have, and metallic;
// - RT2 rgba16f: unexposed emission, the material id in the frame's MaterialTable;
// - RT3 rg16f: the screen-space motion, current uv minus previous uv.

layout(location = 7) flat in float v_materialId;

layout(location = 0) out vec4 outAlbedoOcclusion;
layout(location = 1) out vec4 outNormalRoughness;
layout(location = 2) out vec4 outEmissionMaterial;
layout(location = 3) out vec2 outVelocity;

void main()
{
    MaterialSurface m = materialEvaluate();

    float filteredRoughness = sqrt(sqrt(m.alpha2));

    outAlbedoOcclusion = vec4(yupLinearToSrgb(m.baseColor.rgb), m.occlusion);
    outNormalRoughness = vec4(octahedralEncode(m.n), filteredRoughness, m.metallic);
    outEmissionMaterial = vec4(min(m.emissive, vec3(yupMaxHalf)), v_materialId);
    outVelocity = viewVelocity(v_currentClip, v_previousClip);
}
