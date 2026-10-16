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

// The irradiance volumes (ProbeBaker), compiled only into the shaders shading with them, which define
// YUP_3D_IRRADIANCE_VOLUMES with YUP_3D_REFLECTION_PROBES before their first include: the raster
// local indirect variants of the forward pass, its full tier variants and the lighting pass. The
// full tier shaders also define YUP_3D_IRRADIANCE_VOLUMES_BINDING, the set 1 binding of their
// storage buffer of volumes. The raster tier reads each draw's volume from its ProbeListBlock
#ifdef YUP_3D_IRRADIANCE_VOLUMES

#ifndef YUP_3D_REFLECTION_PROBES
#error "The irradiance volumes are shaded with the reflection probes: define YUP_3D_REFLECTION_PROBES too"
#endif

#include "lib/volumes.glsl"

// One irradiance volume, mirrored by GpuIrradianceVolume in yup_GpuSceneData.h (112 bytes in std140
// and std430). An empty volume (all zero) weighs nothing anywhere
struct GpuIrradianceVolume
{
    mat4 worldToUnit;     // world to the volume box mapped onto [0, 1]^3
    vec4 resolution;      // xyz probes along each axis, w intensity
    vec4 shBlock;         // xy texel of the SH block in the irradiance atlas, z falloff in world units
    vec4 visibilityBlock; // xy texel of the visibility block in the visibility atlas, z tile size without its border (0: no visibility), w normal bias
};

// The irradiance of every volume probe, four texels each, the first band (in RGBM when
// lighting.probeInfo.x is 1) above the three coefficients of the second (as ratios to the first
// then); black without volumes
layout(set = 0, binding = 9) uniform texture2D u_irradianceAtlas;

// The visibility moments of every volume probe in bordered octahedral tiles (lib/volumes.glsl), the
// mean distance and mean squared distance in units of the cell diagonal; black without volumes
layout(set = 0, binding = 10) uniform texture2D u_visibilityAtlas;

#ifdef YUP_3D_TIER_FULL
// The volumes of the frame, lighting.probeInfo.z of them, the smallest first
layout(set = 1, binding = YUP_3D_IRRADIANCE_VOLUMES_BINDING) readonly buffer IrradianceVolumes { GpuIrradianceVolume irradianceVolumes[]; };
#endif

#endif

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
// The two probes taking priority over the draw (SceneDrawList::selectReflectionProbes), count.x of
// them, and the irradiance volume lighting it (SceneDrawList::selectIrradianceVolume) when count.y is 1
layout(set = 2, binding = 13) uniform ProbeListBlock
{
    GpuReflectionProbe probes[2];
#ifdef YUP_3D_IRRADIANCE_VOLUMES
    GpuIrradianceVolume volume;
#endif
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

// The diffuse light of the irradiance volumes (McGuire et al. 2019, "Dynamic Diffuse Global
// Illumination with Ray-Traced Irradiance Fields", JCGT 8(2)), with the probes of each volume
// storing two bands of spherical harmonics (ProbeBaker) instead of octahedral irradiance
#ifdef YUP_3D_IRRADIANCE_VOLUMES

// How much a volume lights a world position: 1 inside the box, fading out over its falloff to 0 at
// the border, the distance to each face measured in world units (the unit coordinate over its rate
// of change along the world). 0 on the border itself and for an empty volume
float irradianceVolumeWeight(GpuIrradianceVolume volume, vec3 worldPosition)
{
    if (volume.resolution.x < 1.5)
        return 0.0;

    vec3 u = (volume.worldToUnit * vec4(worldPosition, 1.0)).xyz;
    mat3 a = mat3(volume.worldToUnit);
    vec3 rate = vec3(length(vec3(a[0][0], a[1][0], a[2][0])), length(vec3(a[0][1], a[1][1], a[2][1])), length(vec3(a[0][2], a[1][2], a[2][2])));
    vec3 inside = min(u, 1.0 - u) / rate;
    float border = min(inside.x, min(inside.y, inside.z));

    if (! (border > 0.0))
        return 0.0;

    return volume.shBlock.z > 0.0 ? min(border / volume.shBlock.z, 1.0) : 1.0;
}

// The irradiance a volume's probes give a surface at @p worldPosition with the normal @p n, its
// spherical harmonics evaluated along @p direction (the bent normal with ambient occlusion), and in
// @p found whether any probe weighs anything there. The 8 probes around the position, offset along
// the normal by the volume's normal bias, are weighted by:
// - their trilinear weight;
// - a smooth backface weight, ((dot (towards the probe, n) + 1) / 2)^2 + 0.2;
// - the Chebyshev bound of their visibility moments in the direction of the position, cubed to
//   sharpen it, where the position lies beyond the mean distance; skipped below the float floor,
//   where the volume has no visibility.
// The sum is normalized by the weights when they add up to more than 1e-4
vec3 irradianceVolumeIrradiance(GpuIrradianceVolume volume, vec3 worldPosition, vec3 n, vec3 direction, out bool found)
{
    vec3 resolution = volume.resolution.xyz;

    // Unit to world for the vectors between the probes: the inverse of the linear part, from its adjugate
    mat3 a = mat3(volume.worldToUnit);
    vec3 r0 = cross(a[1], a[2]);
    vec3 r1 = cross(a[2], a[0]);
    vec3 r2 = cross(a[0], a[1]);
    mat3 unitToWorld = transpose(mat3(r0, r1, r2)) / dot(a[0], r0);

    vec3 biased = worldPosition + n * volume.visibilityBlock.w;
    vec3 u = (volume.worldToUnit * vec4(biased, 1.0)).xyz;
    vec3 grid = u * resolution - 0.5;
    vec3 base = clamp(floor(grid), vec3(0.0), resolution - 2.0);
    vec3 f = clamp(grid - base, vec3(0.0), vec3(1.0));
    float diagonal = length(unitToWorld * (1.0 / resolution));

    vec2 visibilitySize = vec2(textureSize(sampler2D(u_visibilityAtlas, linearClamp), 0));
    float tile = volume.visibilityBlock.z;

    vec3 sum = vec3(0.0);
    float total = 0.0;

    for (int i = 0; i < 8; ++i)
    {
        vec3 corner = vec3(float(i & 1), float((i >> 1) & 1), float((i >> 2) & 1));
        vec3 probe = base + corner;
        vec3 trilinear = mix(1.0 - f, f, corner);
        float weight = trilinear.x * trilinear.y * trilinear.z;

        vec3 toProbe = unitToWorld * ((probe + 0.5) / resolution - u);
        float distance = length(toProbe);
        vec3 towardsProbe = distance > 1.0e-6 ? toProbe / distance : n;

        float backface = max(0.0001, (dot(towardsProbe, n) + 1.0) * 0.5);
        weight *= backface * backface + 0.2;

        // Probe (i, j, k) sits at column i + k nx of the blocks, row j
        vec2 cell = vec2(probe.x + probe.z * resolution.x, probe.y);

        if (tile > 0.0)
        {
            vec2 origin = volume.visibilityBlock.xy + cell * (tile + 2.0);
            vec2 moments = textureLod(sampler2D(u_visibilityAtlas, linearClamp), volumeTileUv(-towardsProbe, origin, tile, visibilitySize), 0.0).xy;
            float d = distance / diagonal;

            if (d > moments.x)
            {
                float variance = abs(moments.y - moments.x * moments.x);
                float delta = d - moments.x;
                float chebyshev = variance / (variance + delta * delta);
                weight *= chebyshev * chebyshev * chebyshev;
            }
        }

        ivec2 texel = ivec2(volume.shBlock.xy + cell * vec2(1.0, 4.0));
        vec4 t0 = texelFetch(sampler2D(u_irradianceAtlas, pointClamp), texel, 0);
        vec4 t1 = texelFetch(sampler2D(u_irradianceAtlas, pointClamp), texel + ivec2(0, 1), 0);
        vec4 t2 = texelFetch(sampler2D(u_irradianceAtlas, pointClamp), texel + ivec2(0, 2), 0);
        vec4 t3 = texelFetch(sampler2D(u_irradianceAtlas, pointClamp), texel + ivec2(0, 3), 0);

        // Below the float floor the second band holds ratios to the first, remapped to [0, 1]
        // (SphericalHarmonics::encodeRatio())
        float rgbm = lighting.probeInfo.x;
        vec3 l0 = yupDecodeRgbm(t0, rgbm);
        vec3 l1y = mix(t1.rgb, (t1.rgb - 0.5) * 4.0 * l0, rgbm);
        vec3 l1z = mix(t2.rgb, (t2.rgb - 0.5) * 4.0 * l0, rgbm);
        vec3 l1x = mix(t3.rgb, (t3.rgb - 0.5) * 4.0 * l0, rgbm);

        sum += max(l0 + l1y * direction.y + l1z * direction.z + l1x * direction.x, vec3(0.0)) * weight;
        total += weight;
    }

    found = total > 1.0e-4;
    return found ? sum / total : vec3(0.0);
}

// The diffuse light of the irradiance volume at @p worldPosition, with its own intensity, for the
// normal @p n and the direction its spherical harmonics are evaluated along, weighted by its share,
// and in @p environmentWeight the share it leaves to the environment (1 without a volume, or where
// no probe of the volume weighs anything). Raster: the draw's volume. Full: the first of the frame's
// volumes, the smallest first, weighing anything there
vec3 diffuseIndirect(vec3 worldPosition, vec3 n, vec3 direction, out float environmentWeight)
{
    environmentWeight = 1.0;

#ifdef YUP_3D_TIER_FULL
    int count = int(lighting.probeInfo.z + 0.5);

    for (int i = 0; i < count; ++i)
    {
        float weight = irradianceVolumeWeight(irradianceVolumes[i], worldPosition);
        if (weight <= 0.0)
            continue;

        bool found;
        vec3 irradiance = irradianceVolumeIrradiance(irradianceVolumes[i], worldPosition, n, direction, found);
        if (! found)
            return vec3(0.0);

        environmentWeight = 1.0 - weight;
        return irradiance * (irradianceVolumes[i].resolution.w * weight);
    }
#else
    if (probeList.count.y > 0.5)
    {
        float weight = irradianceVolumeWeight(probeList.volume, worldPosition);
        if (weight <= 0.0)
            return vec3(0.0);

        bool found;
        vec3 irradiance = irradianceVolumeIrradiance(probeList.volume, worldPosition, n, direction, found);
        if (! found)
            return vec3(0.0);

        environmentWeight = 1.0 - weight;
        return irradiance * (probeList.volume.resolution.w * weight);
    }
#endif

    return vec3(0.0);
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
