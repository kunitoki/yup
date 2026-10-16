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

// The raster local indirect variant reads the ambient occlusion (its specular occlusion is in
// lib/probes.glsl); the full tier's blended surfaces have none
#if defined(YUP_3D_LOCAL_INDIRECT) && ! defined(YUP_3D_TIER_FULL)
#define YUP_3D_AMBIENT_OCCLUSION
#endif

// The raster local indirect variants and the full tier's blended surfaces read the reflection probes
// and the irradiance volumes (lib/probes.glsl), the full tier from set 1 bindings 8 and 9
#if defined(YUP_3D_LOCAL_INDIRECT) || defined(YUP_3D_TIER_FULL)
#define YUP_3D_REFLECTION_PROBES
#define YUP_3D_REFLECTION_PROBES_BINDING 8
#define YUP_3D_IRRADIANCE_VOLUMES
#define YUP_3D_IRRADIANCE_VOLUMES_BINDING 9
#endif

// The clearcoat and sheen layers of the materials (lib/bsdf.glsl, lib/lights.glsl): the raster local
// indirect layers variants (YUP_3D_LAYERS, for the frames drawing a layered material) and every full
// tier variant
#if defined(YUP_3D_LAYERS) && ! defined(YUP_3D_LOCAL_INDIRECT)
#error "The layers variant is a local indirect variant: bundle it as ForwardPass:YUP_3D_LOCAL_INDIRECT+YUP_3D_LAYERS"
#endif

#if defined(YUP_3D_LAYERS) || defined(YUP_3D_TIER_FULL)
#define YUP_3D_MATERIAL_LAYERS
#endif

#include "lib/bsdf.glsl"
#include "lib/color.glsl"
#include "lib/view.glsl"
#include "lib/lighting.glsl"
#include "lib/lights.glsl"
#include "lib/material.glsl"

// glTF metallic-roughness shading (the material from lib/material.glsl) with KHR_lights_punctual
// lights (the draw's own list, see lib/lights.glsl) and a flat ambient term.
// The screen-space derivatives of the normal filter the highlights of smooth surfaces.
// An optional environment adds reflections from a cube GGX-prefiltered per roughness, one mip each
// (lib/probes.glsl), and diffuse light from nine spherical harmonics coefficients. The sun is
// shadowed from its cascades (lib/shadows.glsl).
// The result is exposed, then written per output mode: tone mapped (Reinhard, ACES or AgX) and
// sRGB encoded for an rgba8unorm target, reversibly tone mapped, or linear.

layout(location = 0) out vec4 fragColor;

#ifdef YUP_3D_TIER_FULL
layout(location = 1) out vec2 outVelocity;
#endif

#ifdef YUP_3D_AMBIENT_OCCLUSION
// Raster frames with ambient occlusion only (the variant YUP_3D_LOCAL_INDIRECT): the
// AmbientOcclusionPass output, drawn at half resolution and upsampled bilinearly here, the
// view-space bent normal * 0.5 + 0.5 in rgb and the visibility in a. The full tier's blended
// surfaces have none
layout(set = 1, binding = 13) uniform texture2D u_ambientOcclusion;
#endif

void main()
{
    MaterialSurface m = materialEvaluate();
    vec4 baseColor = m.baseColor;
    vec3 v = normalize(view.cameraPosition.xyz - v_worldPosition);

    BsdfSurface surface = bsdfMaterialSurface(m.n, v, baseColor.rgb, m.metallic, material.extended.x, material.specular, m.alpha2);

    // The environment first, added in the same order as before
    LightsLookup lookup = lightsLookup(surface);
#ifdef YUP_3D_AMBIENT_OCCLUSION
    // The indirect light only: direct light is never occluded
    vec4 ambientOcclusion = textureLod(sampler2D(u_ambientOcclusion, linearClamp), gtaoUpsampleUv(gl_FragCoord.xy, vec2(textureSize(sampler2D(u_ambientOcclusion, linearClamp), 0))), 0.0);
    vec3 bentNormal = gtaoBentNormal(ambientOcclusion.rgb, ambientOcclusion.a, transpose(mat3(view.view)), surface.n, m.geometricNormal);
    float diffuseOcclusion = min(ambientOcclusion.a, m.occlusion);
    float reflectionOcclusion = min(gtaoSpecularOcclusion(surface.nDotV, ambientOcclusion.a, sqrt(sqrt(surface.alpha2))), m.occlusion);
#ifdef YUP_3D_MATERIAL_LAYERS
    // The clearcoat and the sheen, before the light loop so it carries none of their state. Without
    // a clearcoat normal texture, the clearcoat follows the interpolated normal
    vec3 layerIrradiance;
    vec3 indirect = lightsIndirect(surface, lookup, v_worldPosition, m.geometricNormal, bentNormal, diffuseOcclusion, reflectionOcclusion, layerIrradiance);
    float baseScale;
    vec3 layered = lightsLayered(surface, material.clearcoat, material.sheen, m.emissive, v_worldPosition, m.geometricNormal, layerIrradiance, ambientOcclusion.a, m.occlusion, m.kernelRoughness, baseScale);
#else
    vec3 indirect = lightsIndirect(surface, lookup, v_worldPosition, m.geometricNormal, bentNormal, diffuseOcclusion, reflectionOcclusion);
#endif
#else
#ifdef YUP_3D_MATERIAL_LAYERS
    vec3 layerIrradiance;
    vec3 indirect = lightsIndirect(surface, lookup, v_worldPosition, m.geometricNormal, m.occlusion, m.occlusion, layerIrradiance);
    float baseScale;
    vec3 layered = lightsLayered(surface, material.clearcoat, material.sheen, m.emissive, v_worldPosition, m.geometricNormal, layerIrradiance, 1.0, m.occlusion, m.kernelRoughness, baseScale);
#else
    vec3 indirect = lightsIndirect(surface, lookup, v_worldPosition, m.geometricNormal, m.occlusion, m.occlusion);
#endif
#endif
    vec3 color = lightsShade(surface, lookup, v_worldPosition, m.geometricNormal);
#ifdef YUP_3D_MATERIAL_LAYERS
    // The layers over the base, the emission included: exactly the sum below without them
    color = (color + indirect) * baseScale + layered;
#else
    color += indirect;
    color += m.emissive;
#endif

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
