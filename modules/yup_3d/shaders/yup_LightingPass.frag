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

#ifndef YUP_3D_TIER_FULL
#error "The lighting pass is full tier only: bundle it as LightingPass:YUP_3D_TIER_FULL"
#endif

#include "lib/color.glsl"
#include "lib/view.glsl"
#include "lib/octahedral.glsl"
#include "lib/lights.glsl"
#include "lib/material_types.glsl"

// Deferred shading of the G-buffer (full tier only), one fullscreen triangle writing every pixel:
// - surfaces (depth < 1) are shaded like the forward pass does, through the same BSDF, lights,
//   ambient light and environment, from the material the G-buffer stored and the material table
//   entry of its id. The shading normal stands in for the geometric normal the G-buffer lacks;
// - texels no surface covers show the environment when it is visible, or stay transparent so
//   tone mapping composites the background color under them.
// The result is exposed radiance, premultiplied by its coverage alpha.

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

// The frame's MaterialTable, indexed by the material id in RT2
layout(set = 1, binding = 8) readonly buffer Materials { GpuMaterial materials[]; };

layout(set = 1, binding = 9) uniform texture2D u_albedoOcclusion;   // RT0
layout(set = 1, binding = 10) uniform texture2D u_normalRoughness;  // RT1
layout(set = 1, binding = 11) uniform texture2D u_emissionMaterial; // RT2
layout(set = 1, binding = 12) uniform texture2D u_depth;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec2 clip = vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    float depth = texelFetch(sampler2D(u_depth, pointClamp), pixel, 0).r;

    if (depth >= 1.0)
    {
        fragColor = lighting.ambient.w > 0.5 ? vec4(min(lightingBackground(clip), vec3(yupMaxHalf)), 1.0) : vec4(0.0);
        return;
    }

    // The jittered matrices, as the G-buffer was rasterized
    vec4 world = view.inverseViewProjection * vec4(clip, depth, 1.0);
    vec3 worldPosition = world.xyz / world.w;

    vec4 albedoOcclusion = texelFetch(sampler2D(u_albedoOcclusion, pointClamp), pixel, 0);
    vec4 normalRoughness = texelFetch(sampler2D(u_normalRoughness, pointClamp), pixel, 0);
    vec4 emissionMaterial = texelFetch(sampler2D(u_emissionMaterial, pointClamp), pixel, 0);
    GpuMaterial material = materials[int(emissionMaterial.a + 0.5)];

    vec3 n = octahedralDecode(normalRoughness.xy);
    vec3 v = normalize(view.cameraPosition.xyz - worldPosition);
    float roughness2 = normalRoughness.z * normalRoughness.z;

    BsdfSurface surface = bsdfMaterialSurface(n, v, yupSrgbToLinear(albedoOcclusion.rgb), normalRoughness.w,
                                              material.extended.x, material.specular, roughness2 * roughness2);

    vec3 color = lightsShade(surface, worldPosition, n);
    color += lightsAmbientAndEnvironment(surface, n, albedoOcclusion.a);
    color += emissionMaterial.rgb;

    fragColor = vec4(min(color * view.cameraPosition.w, vec3(yupMaxHalf)), 1.0);
}
