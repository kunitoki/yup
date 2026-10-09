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
#extension GL_EXT_texture_shadow_lod : require

#include "lib/bsdf.glsl"
#include "lib/color.glsl"
#include "lib/view.glsl"
#include "lib/lighting.glsl"
#include "lib/lights.glsl"
#include "lib/material.glsl"

// glTF metallic-roughness shading (the material from lib/material.glsl) with KHR_lights_punctual
// lights (the draw's own list, see lib/lights.glsl) and a flat ambient term.
// The screen-space derivatives of the normal filter the highlights of smooth surfaces.
// An optional environment adds reflections from a latitude-longitude image whose mip levels are
// blurred per roughness, and diffuse light from nine spherical harmonics coefficients. The sun is
// shadowed from its cascades (lib/shadows.glsl).
// The result is exposed, then written per output mode: tone mapped (Reinhard, ACES or AgX) and
// sRGB encoded for an rgba8unorm target, reversibly tone mapped, or linear.

layout(location = 0) out vec4 fragColor;

#ifdef YUP_3D_TIER_FULL
layout(location = 1) out vec2 outVelocity;
#endif

void main()
{
    MaterialSurface m = materialEvaluate();
    vec4 baseColor = m.baseColor;
    vec3 v = normalize(view.cameraPosition.xyz - v_worldPosition);

    BsdfSurface surface = bsdfMaterialSurface(m.n, v, baseColor.rgb, m.metallic, material.extended.x, material.specular, m.alpha2);

    vec3 color = lightsShade(surface, v_worldPosition, m.geometricNormal);
    color += lightsAmbientAndEnvironment(surface, m.geometricNormal, m.occlusion);
    color += m.emissive;

    vec3 exposed = color * view.cameraPosition.w;

    if (lighting.environmentInfo.w < 0.5)
        fragColor = vec4(yupLinearToSrgb(yupToneMap(exposed, view.clipPlanes.w)), baseColor.a);
    else if (lighting.environmentInfo.w < 1.5)
        fragColor = vec4(yupReversibleToneMap(min(exposed, vec3(yupMaxHalf))), baseColor.a);
    else
        fragColor = vec4(min(exposed, vec3(yupMaxHalf)), baseColor.a);

#ifdef YUP_3D_TIER_FULL
    outVelocity = viewVelocity(v_currentClip, v_previousClip);
#endif
}
