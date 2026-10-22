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
#error "The lighting classification is full tier only: bundle it as LightingClassify:YUP_3D_TIER_FULL"
#endif

// The local indirect variant (YUP_3D_LOCAL_INDIRECT) classifies the frames with reflection probes
// or irradiance volumes, reading them at the bindings of the lighting pass
#ifdef YUP_3D_LOCAL_INDIRECT
#define YUP_3D_REFLECTION_PROBES
#define YUP_3D_REFLECTION_PROBES_BINDING 14
#define YUP_3D_IRRADIANCE_VOLUMES
#define YUP_3D_IRRADIANCE_VOLUMES_BINDING 15
#endif

#include "lib/probes.glsl"
#include "lib/material_types.glsl"

// The variant of LightingPass each pixel of a classified frame shades with, written as the depth
// the variant's draw tests equal (LightingPass::classDepth):
// - 0.75, the layers variant, where the material has a clearcoat or a sheen (lightsLayered's test);
// - 0.5, the local indirect variant, where a reflection probe or an irradiance volume weighs
//   anything at the surface (the weights lib/probes.glsl shades with);
// - 0.25, the default variant, elsewhere and where no surface covers the pixel.
// Where every probe and volume weighs 0, the local indirect variant shades exactly as the default one

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor; // Masked out by the pipeline

layout(set = 1, binding = 8) readonly buffer Materials { GpuMaterial materials[]; };
layout(set = 1, binding = 11) uniform texture2D u_emissionMaterial; // RT2
layout(set = 1, binding = 12) uniform texture2D u_depth;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    float depth = texelFetch(sampler2D(u_depth, pointClamp), pixel, 0).r;

    fragColor = vec4(0.0);
    gl_FragDepth = 0.25;

    if (depth >= 1.0)
        return;

    int materialId = int(texelFetch(sampler2D(u_emissionMaterial, pointClamp), pixel, 0).a + 0.5);
    vec4 sheen = materials[materialId].sheen;
    if (materials[materialId].clearcoat.x > 0.0 || max(max(sheen.r, sheen.g), sheen.b) > 0.0)
    {
        gl_FragDepth = 0.75;
        return;
    }

#ifdef YUP_3D_LOCAL_INDIRECT
    // The world position as the lighting pass reconstructs it, through the jittered matrices
    vec2 clip = vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    vec4 world = view.inverseViewProjection * vec4(clip, depth, 1.0);
    vec3 worldPosition = world.xyz / world.w;

    int probes = int(lighting.probeInfo.y + 0.5);
    for (int i = 0; i < probes; ++i)
    {
        if (reflectionProbeWeight(reflectionProbes[i], worldPosition) > 0.0)
        {
            gl_FragDepth = 0.5;
            return;
        }
    }

    int volumes = int(lighting.probeInfo.z + 0.5);
    for (int i = 0; i < volumes; ++i)
    {
        if (irradianceVolumeWeight(irradianceVolumes[i], worldPosition) > 0.0)
        {
            gl_FragDepth = 0.5;
            return;
        }
    }
#endif
}
