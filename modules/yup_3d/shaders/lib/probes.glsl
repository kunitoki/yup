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

#ifndef YUP_3D_LIB_PROBES
#define YUP_3D_LIB_PROBES

#include "lib/color.glsl"
#include "lib/lighting.glsl"
#include "lib/view.glsl"

// The light around the scene, shared by every pass that shades or shows it: the environment cube,
// GGX-prefiltered by EnvironmentMap with one roughness per mip and read through the trilinear
// linearClamp, and its diffuse light from the nine spherical harmonics of the LightingBlock. The
// cube holds RGBM when lighting.probeInfo.x is 1, below the float floor.

layout(set = 0, binding = 6) uniform textureCube u_environment;

// The environment towards a direction, at a mip level
vec3 environmentSample(vec3 d, float level)
{
    return yupDecodeRgbm(textureLod(samplerCube(u_environment, linearClamp), d, level), lighting.probeInfo.x);
}

// The environment reflected towards a direction, prefiltered for the roughness through the mip levels
vec3 environmentRadiance(vec3 r, float roughness)
{
    return environmentSample(r, roughness * lighting.environmentInfo.y);
}

// The diffuse light of the environment around a normal, from nine spherical harmonics coefficients
vec3 environmentIrradiance(vec3 n)
{
    vec3 result = lighting.irradiance[0].rgb
                + lighting.irradiance[1].rgb * n.y
                + lighting.irradiance[2].rgb * n.z
                + lighting.irradiance[3].rgb * n.x
                + lighting.irradiance[4].rgb * (n.x * n.y)
                + lighting.irradiance[5].rgb * (n.y * n.z)
                + lighting.irradiance[6].rgb * (3.0 * n.z * n.z - 1.0)
                + lighting.irradiance[7].rgb * (n.x * n.z)
                + lighting.irradiance[8].rgb * (n.x * n.x - n.y * n.y);

    return max(result, vec3(0.0));
}

// The exposed environment behind the scene through a clip position, blurred to the background level
vec3 lightingBackground(vec2 clip)
{
    vec3 color = environmentSample(viewDirection(clip), lighting.environmentInfo.z);
    return color * (lighting.environmentInfo.x * view.cameraPosition.w);
}

// The reflection probes (ProbeBaker), compiled only into the shaders shading with them, which define
// YUP_3D_REFLECTION_PROBES before their first include: the raster local indirect variants of the
// forward pass, its full tier variants and the lighting pass. The full tier shaders also define
// YUP_3D_REFLECTION_PROBES_BINDING, the set 1 binding of their storage buffer of probes
#ifdef YUP_3D_REFLECTION_PROBES

#include "lib/octahedral.glsl"

// One reflection probe, mirrored by GpuReflectionProbe in yup_GpuSceneData.h (128 bytes in std140
// and std430). An empty influence (all zero) weighs nothing anywhere
struct GpuReflectionProbe
{
    mat4 worldToLocal;  // the inverse of the entity world matrix
    vec4 positionShape; // xyz capture position in world space, w shape (0 sphere, 1 box)
    vec4 influence;     // xyz half extents in entity space (a sphere's radius in x), w falloff in entity space
    vec4 atlasRect;     // uv of the level 0 tile origin (xy) and size (zw) in the reflection atlas
    vec4 params;        // x intensity, y 1 for parallax correction, z number of mips
};

// The probes' GGX-prefiltered octahedral tiles, one roughness per mip, holding RGBM when
// lighting.probeInfo.x is 1; black without probes
layout(set = 0, binding = 8) uniform texture2D u_reflectionAtlas;

#ifdef YUP_3D_TIER_FULL
// Every probe of the frame, lighting.probeInfo.y of them, in the priority order of the raster selection
layout(set = 1, binding = YUP_3D_REFLECTION_PROBES_BINDING) readonly buffer ReflectionProbes { GpuReflectionProbe reflectionProbes[]; };
#else
// The two probes taking priority over the draw (SceneDrawList::selectReflectionProbes), count.x of them
layout(set = 2, binding = 13) uniform ProbeListBlock
{
    GpuReflectionProbe probes[2];
    vec4 count;
} probeList;
#endif

// How much a probe reflects at a world position: 1 inside its influence, fading out over the falloff
// to 0 at the border, measured in entity space for both shapes (along the radius of a sphere, to the
// nearest face of a box). 0 on the border itself, so an empty influence weighs nothing
float reflectionProbeWeight(GpuReflectionProbe probe, vec3 worldPosition)
{
    vec3 p = (probe.worldToLocal * vec4(worldPosition, 1.0)).xyz;
    vec3 inside = probe.influence.xyz - abs(p);
    float border = probe.positionShape.w > 0.5 ? min(inside.x, min(inside.y, inside.z)) : probe.influence.x - length(p);

    if (! (border > 0.0))
        return 0.0;

    return probe.influence.w > 0.0 ? min(border / probe.influence.w, 1.0) : 1.0;
}

// The direction to read a probe's capture along for the reflection ray from @p worldPosition along
// @p r. With parallax correction, the direction from the capture position to where the ray leaves the
// influence shape, intersected in entity space (Lagarde and Zanuttini 2012, "Local Image-based Lighting
// With Parallax-corrected Cubemap"); without it, or from outside the shape, the ray's own direction
vec3 parallaxDirection(GpuReflectionProbe probe, vec3 worldPosition, vec3 r)
{
    if (probe.params.y < 0.5)
        return r;

    vec3 p = (probe.worldToLocal * vec4(worldPosition, 1.0)).xyz;
    vec3 d = mat3(probe.worldToLocal) * r;
    float t;

    if (probe.positionShape.w > 0.5)
    {
        // The ray leaves a box through the nearest of the faces it heads towards
        vec3 towards = mix(vec3(-1.0), vec3(1.0), greaterThanEqual(d, vec3(0.0)));
        vec3 exits = (probe.influence.xyz - towards * p) / max(abs(d), vec3(1.0e-6));
        t = min(exits.x, min(exits.y, exits.z));
    }
    else
    {
        float a = dot(d, d);
        float b = dot(p, d);
        float c = dot(p, p) - probe.influence.x * probe.influence.x;
        t = (sqrt(max(b * b - a * c, 0.0)) - b) / max(a, 1.0e-12);
    }

    vec3 toHit = worldPosition + r * max(t, 0.0) - probe.positionShape.xyz;
    float distance = length(toHit);
    return t > 0.0 && distance > 1.0e-6 ? toHit / distance : r;
}

// A probe's capture towards @p direction, prefiltered for the perceptual @p roughness through the
// mips of its octahedral tile (two taps at the whole mips around the lod, octahedralTileTaps, so no
// lookup reads a neighboring tile), decoded from RGBM and scaled by its intensity
vec3 sampleReflectionProbe(GpuReflectionProbe probe, vec3 direction, float roughness)
{
    vec2 atlasSize = vec2(textureSize(sampler2D(u_reflectionAtlas, linearClamp), 0));
    int numMips = int(probe.params.z + 0.5);
    OctahedralTileTaps taps = octahedralTileTaps(direction, probe.atlasRect * atlasSize.xyxy, atlasSize, roughness * float(numMips - 1), numMips);

    vec3 first = yupDecodeRgbm(textureLod(sampler2D(u_reflectionAtlas, linearClamp), taps.uv0, taps.lod0), lighting.probeInfo.x);
    vec3 second = yupDecodeRgbm(textureLod(sampler2D(u_reflectionAtlas, linearClamp), taps.uv1, taps.lod1), lighting.probeInfo.x);
    return mix(first, second, taps.weight) * probe.params.x;
}

// Adds a probe of weight @p weight to @p radiance in selection order: it takes that share of what the
// probes before it left (@p rest), so the first probe weighs w0, the second (1 - w0) w1, and the
// environment what is left. No weight is ever divided by a sum of weights
void reflectionProbeBlend(GpuReflectionProbe probe, float weight, vec3 worldPosition, vec3 r, float roughness, inout vec3 radiance, inout float rest)
{
    float share = weight * rest;
    if (share <= 0.0)
        return;

    radiance += sampleReflectionProbe(probe, parallaxDirection(probe, worldPosition, r), roughness) * share;
    rest -= share;
}

// The specular light the reflection probes give a surface at @p worldPosition reflecting along @p r
// with the perceptual @p roughness, and in @p environmentWeight the share they leave to the
// environment (1 without a probe). Raster: the draw's probes. Full: the first two of the frame's
// probes weighing anything there, in the same priority order, so both tiers blend alike
vec3 specularIndirect(vec3 worldPosition, vec3 r, float roughness, out float environmentWeight)
{
    vec3 radiance = vec3(0.0);
    float rest = 1.0;

#ifdef YUP_3D_TIER_FULL
    int count = int(lighting.probeInfo.y + 0.5);
    int found = 0;

    for (int i = 0; i < count && found < 2; ++i)
    {
        float weight = reflectionProbeWeight(reflectionProbes[i], worldPosition);
        if (weight <= 0.0)
            continue;

        reflectionProbeBlend(reflectionProbes[i], weight, worldPosition, r, roughness, radiance, rest);
        ++found;
    }
#else
    int count = min(int(probeList.count.x + 0.5), 2);

    for (int i = 0; i < count; ++i)
        reflectionProbeBlend(probeList.probes[i], reflectionProbeWeight(probeList.probes[i], worldPosition), worldPosition, r, roughness, radiance, rest);
#endif

    environmentWeight = rest;
    return radiance;
}

#endif

// The specular occlusion of the ambient occlusion and the texture coordinates of its upsample,
// compiled only into the shaders reading it, which define YUP_3D_AMBIENT_OCCLUSION before their
// first include. Self-contained: no bindings, no includes
#ifdef YUP_3D_AMBIENT_OCCLUSION

// Where a full resolution fragment reads the half resolution ambient occlusion of a texture of
// @p size texels: texel i was traced at the input texel 2i, whose center lies half a full
// resolution texel up-left of the center of the 2 x 2 texels it covers, and its denoise is centered
vec2 gtaoUpsampleUv(vec2 fragCoord, vec2 size)
{
    return (fragCoord + 0.5) * 0.5 / size;
}

// The world-space bent normal of an ambient occlusion texel: @p encoded is the view-space bent
// normal * 0.5 + 0.5, @p viewToWorld the rotation from view to world space. The surface normal
// @p n stands in where nothing occludes (visibility 1, which the white stand-in also reads), where
// the decoded direction is degenerate, or where it points below the geometric surface
vec3 gtaoBentNormal(vec3 encoded, float visibility, mat3 viewToWorld, vec3 n, vec3 geometricNormal)
{
    if (visibility >= 1.0)
        return n;

    vec3 bent = viewToWorld * (encoded * 2.0 - 1.0);
    float bentLength = length(bent);
    if (bentLength < 1.0e-3)
        return n;

    bent /= bentLength;
    return dot(bent, geometricNormal) > 0.0 ? bent : n;
}

// Specular occlusion from the ambient visibility, the view angle and the GGX roughness (Lagarde and
// de Rousiers 2014, "Moving Frostbite to Physically Based Rendering", section 4.10.2). It is
// continuous in the visibility and keeps the grazing reflections of open surfaces, whose
// visibility falls a little short of 1, where a cone-cone intersection (Jimenez et al. 2016) cuts
// them off. A visibility of 1 occludes nothing, exactly
float gtaoSpecularOcclusion(float nDotV, float visibility, float roughness)
{
    if (visibility >= 1.0)
        return 1.0;

    float alpha = roughness * roughness;
    return clamp(pow(max(nDotV, 0.0) + visibility, exp2(-16.0 * alpha - 1.0)) - 1.0 + visibility, 0.0, 1.0);
}

#endif

#endif
