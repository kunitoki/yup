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

#ifndef YUP_3D_TIER_FULL
#error "The lighting pass is full tier only: bundle it as LightingPass:YUP_3D_TIER_FULL"
#endif

// The shadow filter variant (YUP_3D_SOFT_SHADOWS) of the lighting pass holds the contact shadows
// too: its lights march them through the scene depth (lib/shadows.glsl)
#ifdef YUP_3D_SOFT_SHADOWS
#define YUP_3D_CONTACT_SHADOWS
#endif

// The opaque surfaces read the ambient occlusion (lib/probes.glsl)
#define YUP_3D_AMBIENT_OCCLUSION

#include "lib/color.glsl"
#include "lib/view.glsl"
#include "lib/octahedral.glsl"

layout(set = 1, binding = 12) uniform texture2D u_depth;

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

// AmbientOcclusionPass, at half resolution: the view-space bent normal * 0.5 + 0.5 in rgb, the
// visibility in a, read when lighting.probeInfo.w is 1. A 1x1 white texture without ambient occlusion
layout(set = 1, binding = 13) uniform texture2D u_ambientOcclusion;

// The view depth of a stored depth, for perspective and orthographic projections (as lib/gtao.glsl)
float lightingViewDepth(float depth)
{
    mat4 p = view.projection;
    return (p[3][2] - depth * p[3][3]) / (depth * p[2][3] - p[2][2]);
}

// The ambient occlusion of a pixel (encoded bent normal in rgb, visibility in a), upsampled from
// half resolution: the four bilinear taps around it, each weighted by how close the depth and the
// normal its texel was traced from are to the pixel's, so the occlusion does not bleed across depth
// or normal edges. Taps traced from the sky (depth 1, at infinity with an infinite far plane) do not
// count; without any, nothing occludes. Blended relative to the first valid tap, so equal taps (the
// white texture) give their value exactly
vec4 lightingAmbientOcclusion(ivec2 pixel, float viewDepth, vec3 n)
{
    ivec2 size = textureSize(sampler2D(u_ambientOcclusion, pointClamp), 0);
    vec2 position = gtaoUpsampleUv(vec2(pixel) + 0.5, vec2(size)) * vec2(size) - 0.5;
    ivec2 base = ivec2(floor(position));
    vec2 f = position - vec2(base);

    bool found = false;
    vec4 first = vec4(1.0);
    vec4 sum = vec4(0.0);
    float weights = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        ivec2 offset = ivec2(i & 1, i >> 1);
        ivec2 texel = clamp(base + offset, ivec2(0), size - 1);
        ivec2 source = min(texel * 2, ivec2(view.viewport.xy) - 1);

        float tapStored = texelFetch(sampler2D(u_depth, pointClamp), source, 0).r;
        if (tapStored >= 1.0)
            continue;

        float tapDepth = lightingViewDepth(tapStored);
        vec3 tapNormal = octahedralDecode(texelFetch(sampler2D(u_normalRoughness, pointClamp), source, 0).xy);
        float facing = max(dot(tapNormal, n), 0.0);
        facing *= facing;
        facing *= facing;
        facing *= facing;

        vec2 bilinear = mix(1.0 - f, f, vec2(offset));
        float similar = clamp(1.0 - abs(tapDepth - viewDepth) / (0.05 * abs(viewDepth)), 0.0, 1.0) * facing;
        float weight = bilinear.x * bilinear.y * (similar + 1.0e-3);

        vec4 occlusion = texelFetch(sampler2D(u_ambientOcclusion, pointClamp), texel, 0);
        if (! found)
            first = occlusion;

        found = true;
        sum += (occlusion - first) * weight;
        weights += weight;
    }

    return found ? first + sum / max(weights, 1.0e-6) : vec4(1.0);
}

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

    // The indirect light only: direct light is never occluded
    vec4 ambientOcclusion = lighting.probeInfo.w > 0.5 ? lightingAmbientOcclusion(pixel, lightingViewDepth(depth), n) : vec4(1.0);
    vec3 bentNormal = gtaoBentNormal(ambientOcclusion.rgb, ambientOcclusion.a, transpose(mat3(view.view)), surface.n, n);
    float diffuseOcclusion = min(ambientOcclusion.a, albedoOcclusion.a);
    float reflectionOcclusion = min(gtaoSpecularOcclusion(surface.nDotV, ambientOcclusion.a, normalRoughness.z), albedoOcclusion.a);

    // The environment first, added in the same order as before
    LightsLookup lookup = lightsLookup(surface);
    vec3 indirect = lightsIndirect(surface, lookup, worldPosition, n, bentNormal, diffuseOcclusion, reflectionOcclusion);
    vec3 color = lightsShade(surface, lookup, worldPosition, n);
    color += indirect;
    color += emissionMaterial.rgb;

    fragColor = vec4(min(color * view.cameraPosition.w, vec3(yupMaxHalf)), 1.0);
}
