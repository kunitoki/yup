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


#ifndef YUP_3D_LIB_SHADOWS
#define YUP_3D_LIB_SHADOWS

#include "lib/light_types.glsl"
#include "lib/sampling.glsl"
#include "lib/view.glsl"

// The shadows of the lights, declared once for the passes that shade (included by lib/lights.glsl):
// the set 3 shadow maps, compared through the set 0 shadowCompare sampler, and the ShadowBlock
// describing them. Then shadowsVisibility(), the one shadow lookup of the light loop.
// The passes including it must enable GL_EXT_texture_shadow_lod: the cascades are compared at an
// explicit level of 0, which is valid in the non-uniform control flow of the light loop on every
// backend (a constant level 0 compiles to textureGrad on GL and GLES).

// Mirrors GpuShadowRecord in yup_GpuSceneData.h: one shadow map view, a cascade, a spot light or
// one face of a point light
struct GpuShadowRecord
{
    mat4 viewProjection; // world to shadow clip space, depth 0 to 1
    vec4 atlasRect;      // u, v, size in UV units, 1 / atlas size (cascades: layer, 0, 1, 1 / resolution); size 0 = no map
    vec4 params;         // depth bias, normal offset (world units), near, far
    vec4 contact;        // contact shadow march distance (0 off), thickness, 0, 0
};

// Mirrors GpuShadowData in yup_GpuSceneData.h
struct GpuShadowData
{
    vec4 cascadeSplits;          // view-space far distance of each cascade
    vec4 cascadeTexel;           // world size of one texel of each cascade
    vec4 sunInfo;                // cascade count, unused (0), blend band fraction, 1 when receivers lie beyond the last cascade
    vec4 filterInfo;             // soft shadows (0/1), PCF taps, blocker taps, flags (1 contact shadows, 2 temporal noise)
    GpuShadowRecord records[64]; // cascades first, then 6 per point light and 1 per spot light
};

layout(set = 0, binding = 4) uniform samplerShadow shadowCompare;          // lessEqual, linear: bilinear comparisons
layout(set = 3, binding = 0) uniform texture2DArray shadowCascadesCompare; // the sun cascades, one layer each

// The shadow views of the frame, next to the shadow maps of set 3
layout(set = 3, binding = 4) uniform ShadowBlock
{
    GpuShadowData data;
} shadows;

// Two values in [0, 1) per pixel for the shadow filters, interleaved gradient noise (Jimenez 2014).
// They change every frame, over a cycle of 64 frames, only when temporal anti-aliasing averages them
// (filterInfo.w flag 2); otherwise the pattern stays still
vec2 shadowsNoise()
{
    bool temporal = mod(floor(shadows.data.filterInfo.w * 0.5), 2.0) > 0.5;
    float k = temporal ? mod(view.clipPlanes.z, 64.0) : 0.0;
    return vec2(yupInterleavedGradientNoise(gl_FragCoord.xy + 5.588238 * k),
                yupInterleavedGradientNoise(gl_FragCoord.yx + 5.588238 * k));
}

// One bilinear comparison of cascade layer `layer` at uv, against depth
float shadowsCompareCascade(vec2 uv, float layer, float depth)
{
    return textureLod(sampler2DArrayShadow(shadowCascadesCompare, shadowCompare), vec4(uv, layer, depth), 0.0);
}

// The filtered visibility of cascade c at a world position, from its record: offset along the
// geometric normal, more where the light grazes the surface, then compared with its depth bias.
// Raster: the 4x4 tent of the Phase 0 map (bilinear comparisons over a 3x3 texel footprint, after
// Reeves et al., "Rendering Antialiased Shadows with Depth Maps", SIGGRAPH 1987), in four bilinear
// lookups whose offsets and weights follow the sub-texel position (I. Castano, "Shadow Mapping
// Summary - Part 1", 2013): per axis the texels base - 1 to base + 2 weigh 1 - f, 1, 1, f.
// Full: 8 lookups on a Vogel spiral of 1.5 texels, rotated per pixel by the noise, which temporal
// anti-aliasing averages.
float shadowsCascadeVisibility(int c, vec3 worldPosition, vec3 geometricNormal, float nDotL, vec2 noise)
{
    vec4 params = shadows.data.records[c].params;
    vec3 position = worldPosition + geometricNormal * (params.y * (1.0 - nDotL));

    vec4 clip = shadows.data.records[c].viewProjection * vec4(position, 1.0);
    vec2 uv = vec2(clip.x * 0.5 + 0.5, 0.5 - clip.y * 0.5);
    float depth = clip.z - params.x;

    if (any(lessThanEqual(uv, vec2(0.0))) || any(greaterThanEqual(uv, vec2(1.0))) || depth >= 1.0)
        return 1.0;

    float layer = shadows.data.records[c].atlasRect.x;
    float texel = shadows.data.records[c].atlasRect.w;

#ifdef YUP_3D_TIER_FULL
    float angle = noise.x * 6.28318531;
    float lit = 0.0;

    for (int i = 0; i < 8; ++i)
    {
        float radius = 1.5 * sqrt((float(i) + 0.5) / 8.0);
        float theta = float(i) * 2.39996323 + angle;
        lit += shadowsCompareCascade(uv + vec2(cos(theta), sin(theta)) * (radius * texel), layer, depth);
    }

    return lit / 8.0;
#else
    // Per axis two lookups, between texels base - 1 and base (weights 1 - f and 1), and between
    // base + 1 and base + 2 (weights 1 and f), in texel units where texel i is centered on i + 0.5
    vec2 grid = uv / texel - 0.5;
    vec2 base = floor(grid);
    vec2 f = grid - base;

    vec2 w0 = 2.0 - f;
    vec2 w1 = 1.0 + f;
    vec2 p0 = (base - 0.5 + 1.0 / w0) * texel;
    vec2 p1 = (base + 1.5 + f / w1) * texel;

    float lit = w0.x * w0.y * shadowsCompareCascade(p0, layer, depth)
              + w1.x * w0.y * shadowsCompareCascade(vec2(p1.x, p0.y), layer, depth)
              + w0.x * w1.y * shadowsCompareCascade(vec2(p0.x, p1.y), layer, depth)
              + w1.x * w1.y * shadowsCompareCascade(p1, layer, depth);

    return lit / 9.0;
#endif
}

// Visibility of the sun at a world position: 1 lit, 0 shadowed. Cascaded shadow maps (W. Engel,
// "Cascaded Shadow Maps", ShaderX5, 2006): the first cascade whose far split lies beyond the view
// depth, filtered with percentage closer filtering (Reeves et al. 1987, see shadowsCascadeVisibility).
// Over the last sunInfo.z of its depth range a cascade hands over to the next one, blended linearly
// on raster and dithered by the noise on full (one lookup). The last cascade reaches every receiver,
// unless the far plane or the shadow distance cut it short (sunInfo.w): it then fades out to lit, so
// the end of the shadows never pops
float sunShadow(vec3 worldPosition, vec3 geometricNormal, vec3 toLight, float viewDepth, vec2 noise)
{
    int count = int(shadows.data.sunInfo.x);
    bool fadesOut = shadows.data.sunInfo.w > 0.5;

    int c = 0;
    for (int i = 0; i < 3; ++i)
        c += i + 1 < count && viewDepth >= shadows.data.cascadeSplits[i] ? 1 : 0;

    bool last = c + 1 >= count;
    float splitNear = c > 0 ? shadows.data.cascadeSplits[max(c - 1, 0)] : 0.0;
    float splitFar = shadows.data.cascadeSplits[c];

    if (last && fadesOut && viewDepth >= splitFar)
        return 1.0;

    float bandStart = splitFar - shadows.data.sunInfo.z * (splitFar - splitNear);
    float t = last && ! fadesOut ? 0.0 : clamp((viewDepth - bandStart) / max(splitFar - bandStart, 1.0e-6), 0.0, 1.0);

    float nDotL = clamp(dot(geometricNormal, toLight), 0.0, 1.0);

#ifdef YUP_3D_TIER_FULL
    if (! last && noise.y < t)
        return shadowsCascadeVisibility(c + 1, worldPosition, geometricNormal, nDotL, noise);

    float visibility = shadowsCascadeVisibility(c, worldPosition, geometricNormal, nDotL, noise);
    return last ? mix(visibility, 1.0, t) : visibility;
#else
    float visibility = shadowsCascadeVisibility(c, worldPosition, geometricNormal, nDotL, noise);
    if (t <= 0.0)
        return visibility;

    float next = last ? 1.0 : shadowsCascadeVisibility(c + 1, worldPosition, geometricNormal, nDotL, noise);
    return mix(visibility, next, t);
#endif
}

// The shadow of one light at a world position: 1 lit, 0 shadowed, from the first shadow record of
// the light (spotRadiusShadow.w, -1 for none), dispatched by type: a directional light whose first
// record is a cascade is the sun; any other directional record (contact shadows only) has no map,
// and neither have point and spot lights yet
float shadowsVisibility(GpuLight light, vec3 worldPosition, vec3 geometricNormal)
{
    float record = light.spotRadiusShadow.w;
    if (record < 0.0)
        return 1.0;

    if (light.positionType.w < 0.5)
    {
        if (record >= shadows.data.sunInfo.x)
            return 1.0;

        float viewDepth = -(view.view * vec4(worldPosition, 1.0)).z;
        return sunShadow(worldPosition, geometricNormal, -light.directionRange.xyz, viewDepth, shadowsNoise());
    }

    return 1.0;
}

#endif
