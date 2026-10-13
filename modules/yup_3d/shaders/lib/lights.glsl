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


#ifndef YUP_3D_LIB_LIGHTS
#define YUP_3D_LIB_LIGHTS

#include "lib/bsdf.glsl"
#include "lib/lighting.glsl"
#include "lib/probes.glsl"

// Punctual lights after KHR_lights_punctual (Khronos glTF 2.0 extension): inverse square falloff,
// windowed by the range, and a smooth spot cone. Lights with a radius are area lights, shaded with
// linearly transformed cosines (lightsEvaluateArea), compiled only with YUP_3D_AREA_LIGHTS: without
// it every light is punctual, and the passes pick that variant for frames whose lights all have a
// radius of 0. Each light is shadowed through lib/shadows.glsl (shadowsVisibility), from its first
// shadow record.
// The raster tier loops over the lights each draw selected. The full tier loops over every
// directional light, then over the lights of the fragment's cluster (Olsson et al., "Clustered
// Deferred and Forward Shading", HPG 2012), listed by the LightClusters compute pass.

#include "lib/light_types.glsl"
#include "lib/shadows.glsl"

// The lookup tables of LookupTables, a set 0 lookup read through the shared linear sampler: layer 0
// holds the inverse LTC matrices of the area lights, m00, m02, m11, m20 (m22 = 1), and layer 1 the
// GGX lobe magnitude, its Fresnel part, the sheen albedo and 1, the split sum of the environment too
layout(set = 0, binding = 7) uniform texture2DArray lookupLuts;

// The coordinates of a surface in the lookup tables: columns sqrt (1 - n.v), rows sqrt (alpha),
// sampled at the texel centers
vec2 lightsLookupCoordinates(BsdfSurface s)
{
    return (vec2(sqrt(1.0 - s.nDotV), sqrt(sqrt(s.alpha2))) * 63.0 + 0.5) / 64.0;
}

// A surface's place in the lookup tables, read once per pixel before the light loop: its coordinates
// and layer 1 there, shared by the environment and, in the area light variants, by every area light.
// The punctual variants read layer 1 only when the environment, the probes or the volumes need it
struct LightsLookup
{
    vec2 uv;   // lightsLookupCoordinates()
    vec2 lobe; // layer 1: the GGX lobe magnitude and its Fresnel part (0 when not read)
};

LightsLookup lightsLookup(BsdfSurface s)
{
    LightsLookup lookup = LightsLookup(lightsLookupCoordinates(s), vec2(0.0));

#ifndef YUP_3D_AREA_LIGHTS
#if defined(YUP_3D_IRRADIANCE_VOLUMES)
    if (lighting.environmentInfo.x > 0.0 || lighting.probeInfo.y > 0.0 || lighting.probeInfo.z > 0.0)
#elif defined(YUP_3D_REFLECTION_PROBES)
    if (lighting.environmentInfo.x > 0.0 || lighting.probeInfo.y > 0.0)
#else
    if (lighting.environmentInfo.x > 0.0)
#endif
#endif
    {
        lookup.lobe = textureLod(sampler2DArray(lookupLuts, linearClamp), vec3(lookup.uv, 1.0), 0.0).xy;
    }

    return lookup;
}

#ifdef YUP_3D_TIER_FULL
// Every light, directional ones first (GpuSceneData::packLights), and the light lists of the
// clusters: 64 uints per cluster, the count then up to 63 ascending indices into lights
layout(set = 1, binding = 6) readonly buffer Lights { GpuLight lights[]; };
layout(set = 1, binding = 7) readonly buffer ClusterLights { uint clusterLights[]; };
#else
// The lights of the draw, most influential first (SceneDrawList::selectLights). Mirrors
// ForwardLightListBlock in yup_ForwardPass.cpp
layout(set = 2, binding = 12) uniform LightListBlock
{
    GpuLight lights[8];
    vec4 count;            // x number of lights
} lightList;
#endif

// Radiance arriving from one light, without the BSDF: color * intensity * attenuation, and the
// unit vector to the light. Point and spot lights beyond abs (range) are culled, dark ones (a range
// of 0) included
vec3 lightIncoming(GpuLight light, vec3 worldPosition, out vec3 l)
{
    l = -light.directionRange.xyz;
    float attenuation = 1.0;

    if (light.positionType.w > 0.5)
    {
        vec3 toLight = light.positionType.xyz - worldPosition;
        float distanceSquared = max(dot(toLight, toLight), 0.0001);
        float lightDistance = sqrt(distanceSquared);

        l = toLight / lightDistance;

        if (lightDistance > abs(light.directionRange.w))
            return vec3(0.0);

        attenuation = 1.0 / distanceSquared;

        if (light.directionRange.w > 0.0)
        {
            float ratio = lightDistance / light.directionRange.w;
            float ratio2 = ratio * ratio;
            attenuation = attenuation * clamp(1.0 - ratio2 * ratio2, 0.0, 1.0);
        }

        if (light.positionType.w > 1.5)
        {
            float cone = clamp(dot(light.directionRange.xyz, -l) * light.spotRadiusShadow.x + light.spotRadiusShadow.y, 0.0, 1.0);
            attenuation = attenuation * cone * cone;
        }
    }

    return light.colorIntensity.rgb * (light.colorIntensity.w * attenuation);
}

#ifdef YUP_3D_AREA_LIGHTS
// The clamped cosine integrated over a sphere cap, divided by pi, from its unclipped form factor
// len = sin^2 of its angular radius and the cosine z of its axis with the normal. Exact: cos z len
// while the cap lies above the horizon, then the closed form of the cap clipped by it (J. Snyder,
// "Area Light Sources for Real-Time Graphics", Microsoft Research MSR-TR-96-11, 1996; S. Lagarde and
// C. de Rousiers, "Moving Frostbite to Physically Based Rendering", SIGGRAPH 2014 course)
float lightsClippedCap(float len, float z)
{
    len = min(len, 0.9999);
    if (z * z >= len)
        return len * max(z, 0.0);

    float sinTheta = sqrt(1.0 - z * z);
    float x = sqrt(1.0 / len - 1.0);
    float y = clamp(-x * z / sinTheta, -1.0, 1.0);
    float s = sinTheta * sqrt(1.0 - y * y);
    return max((z * acos(y) - x * s) * len + atan(s, x), 0.0) / yupPi;
}

// The clamped cosine integrated over an ellipse of center c and conjugate semi-axes a1 and a2, in a
// frame where the cosine points up z. The cone from the origin through the ellipse has its
// form-factor vector F along its principal axis, of length ab / sqrt ((1 + a^2) (1 + b^2)) from the
// semi-axes a and b of its section at unit distance. Both come from the cone's negative eigenvalue,
// the root in [-1, 0] of its characteristic cubic, which Laguerre's method reaches monotonically
// from -1. The sphere cap of the same F is then clipped by the horizon (lightsClippedCap), after
// E. Heitz and S. Hill, "Real-Time Line- and Disk-Light Shading with Linearly Transformed Cosines",
// SIGGRAPH 2017 course.
float lightsEllipseIntegral(vec3 c, vec3 a1, vec3 a2)
{
    if (dot(c, c) <= 0.0)
        return 0.0;

    // Scale invariant: with the center at unit distance the cubic stays well conditioned
    float scale = inversesqrt(dot(c, c));
    c *= scale;
    a1 *= scale;
    a2 *= scale;

    vec3 n = cross(a1, a2);
    float n2 = dot(n, n);
    float c3 = dot(c, n);
    if (n2 <= 0.0 || c3 == 0.0)
        return 0.0;

    vec3 w = n * (sign(c3) * inversesqrt(n2));
    c3 = abs(c3) * inversesqrt(n2);
    if (c3 < 1.0e-6)
        return 0.0;

    // The principal semi-axes, from the eigenvectors of the Gram matrix of a1 and a2
    float d11 = dot(a1, a1);
    float d22 = dot(a2, a2);
    float d12 = dot(a1, a2);
    float aa = 0.5 * (d11 + d22) + sqrt(0.25 * (d11 - d22) * (d11 - d22) + d12 * d12);
    float bb = n2 / aa;

    vec2 g = d11 >= d22 ? vec2(aa - d22, d12) : vec2(d12, aa - d11);
    vec3 u1 = g.x * a1 + g.y * a2;
    u1 = dot(u1, u1) > 0.0 ? normalize(u1) : normalize(a1);
    vec3 u2 = cross(w, u1);

    float c1 = dot(c, u1);
    float c2 = dot(c, u2);
    float c33 = c3 * c3;

    // The characteristic cubic of the cone, times a^2 b^2: n2 e^3 - k2 e^2 + k1 e + k0
    float k2 = bb * (c1 * c1 + c33) + aa * (c2 * c2 + c33) - n2;
    float k1 = c33 * (1.0 - aa - bb);
    float k0 = c33 * c33;

    float e = -1.0;
    for (int i = 0; i < 6; ++i)
    {
        float p = ((n2 * e - k2) * e + k1) * e + k0;
        float dp = (3.0 * n2 * e - 2.0 * k2) * e + k1;
        float ddp = 6.0 * n2 * e - 2.0 * k2;
        float denominator = dp + sqrt(max(4.0 * dp * dp - 6.0 * p * ddp, 0.0));
        e = denominator > 0.0 ? clamp(e - 3.0 * p / denominator, -1.0, 0.0) : e;
    }

    float len = sqrt(n2 * e * e / max(k1 - 2.0 * e * k2 + 3.0 * n2 * e * e, 1.0e-30));
    vec3 axis = u1 * (c1 * c3 / (c33 - aa * e)) + u2 * (c2 * c3 / (c33 - bb * e)) + w;

    return lightsClippedCap(len, normalize(axis).z);
}

// Radiance from a light with radius > 0: specular through the LTC fit (Heitz et al., "Real-Time
// Polygonal-Light Shading with Linearly Transformed Cosines", SIGGRAPH 2016, tables from LtcTables),
// the disk and sphere integration of Heitz and Hill 2017 (lightsEllipseIntegral), and a
// clamped-cosine (identity LTC) diffuse.
// Point and spot lights are spheres of radiance I / (pi r^2), integrated as the disk of their
// silhouette facing the shading point; spot lights keep the cone falloff of their center.
// Directional lights are disks at infinity of angular radius `radius` and radiance
// I / (pi sin^2 radius). So radius -> 0 converges to the punctual light.
// The specular weighs the lobe magnitude x and its Fresnel part y as f0 (x - y) + f90 y; the diffuse
// is dimmed by the dielectric Fresnel towards the light's center, as bsdfEvaluate does.
// Known limitation of the tables: on smooth surfaces (roughness below 0.12) seen at grazing angles
// (n.v below 0.03) the lobe is misplaced, the fit jumping between the last two columns.
vec3 lightsEvaluateArea(GpuLight light, BsdfSurface s, LightsLookup lookup, vec3 worldPosition)
{
    vec3 l;
    vec3 incoming = lightIncoming(light, worldPosition, l);
    if (incoming == vec3(0.0))
        return vec3(0.0);

    float radius = light.spotRadiusShadow.z;
    vec3 center;
    vec3 facing;
    float diskRadius;

    if (light.positionType.w < 0.5)
    {
        // At unit distance, the scale not mattering
        float angle = min(radius, 1.5);
        float sine = sin(angle);
        incoming /= yupPi * sine * sine;
        center = l;
        facing = -l;
        diskRadius = tan(angle);
    }
    else
    {
        // lightIncoming divided by the squared distance, which the solid angle of the disk now counts
        center = light.positionType.xyz - worldPosition;
        float distanceSquared = max(dot(center, center), 0.0001);
        incoming *= distanceSquared / (yupPi * radius * radius);

        // At the light's center lightIncoming has no direction: the light then lies over the surface
        if (dot(l, l) == 0.0)
        {
            l = s.n;
            center = s.n * sqrt(distanceSquared);
        }

        // The silhouette cone has a sine of r / d, a shading point inside the sphere sees a hemisphere
        facing = -l;
        diskRadius = radius * inversesqrt(max(1.0 - radius * radius / distanceSquared, 1.0e-4));
    }

    vec3 ex = normalize(cross(facing, abs(facing.z) < 0.9 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0))) * diskRadius;
    vec3 ey = cross(facing, ex);

    // The frame of the tables: the normal up z, the view in the xz plane towards +x
    vec3 t1 = s.v - s.n * dot(s.v, s.n);
    t1 = dot(t1, t1) > 1.0e-8 ? normalize(t1) : normalize(cross(s.n, abs(s.n.z) < 0.9 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0)));
    mat3 toLocal = transpose(mat3(t1, cross(s.n, t1), s.n));

    vec3 c = toLocal * center;
    vec3 a1 = toLocal * ex;
    vec3 a2 = toLocal * ey;

    vec4 m = textureLod(sampler2DArray(lookupLuts, linearClamp), vec3(lookup.uv, 0.0), 0.0);
    mat3 inverseMatrix = mat3(vec3(m.x, 0.0, m.w), vec3(0.0, m.z, 0.0), vec3(m.y, 0.0, 1.0));

    vec3 specular = (s.f0 * (lookup.lobe.x - lookup.lobe.y) + s.f90 * lookup.lobe.y)
                  * lightsEllipseIntegral(inverseMatrix * c, inverseMatrix * a1, inverseMatrix * a2);

    vec3 h = s.v + l;
    float vDotH = max(dot(s.v, h) * inversesqrt(max(dot(h, h), 1.0e-8)), 0.0);
    vec3 dielectricFresnel = bsdfFresnelSchlick(s.dielectricF0, vec3(s.dielectricF90), vDotH);
    vec3 diffuse = s.diffuseColor * (bsdfDiffuseWeight(dielectricFresnel) * lightsEllipseIntegral(c, a1, a2));

    return incoming * (diffuse + specular);
}
#endif

// What one light reflects: the BSDF times its incoming radiance and its shadow, looked up once towards
// its center (shadowsVisibility), and its contact shadow in the passes defining YUP_3D_CONTACT_SHADOWS
// (shadowsContact), marched only where the light reaches the surface: after the range, cone and
// horizon exits of the punctual path (before its shadow, which measured faster than after it), and
// after the shadow of the area path. Lights with a radius take the area path (YUP_3D_AREA_LIGHTS
// only), the others the punctual one
vec3 lightsShadeLight(BsdfSurface s, LightsLookup lookup, GpuLight light, vec3 worldPosition, vec3 geometricNormal)
{
#ifdef YUP_3D_AREA_LIGHTS
    if (light.spotRadiusShadow.z > 0.0)
    {
        vec3 reflected = lightsEvaluateArea(light, s, lookup, worldPosition);
        if (reflected != vec3(0.0))
            reflected = reflected * shadowsVisibility(light, worldPosition, geometricNormal);
#ifdef YUP_3D_CONTACT_SHADOWS
        if (reflected != vec3(0.0))
            reflected = reflected * shadowsContact(light, worldPosition, geometricNormal);
#endif

        return reflected;
    }
#endif

    vec3 l;
    vec3 incoming = lightIncoming(light, worldPosition, l);
    if (dot(s.n, l) <= 0.0 || incoming == vec3(0.0))
        return vec3(0.0);

#ifdef YUP_3D_CONTACT_SHADOWS
    float contact = shadowsContact(light, worldPosition, geometricNormal);
    if (contact <= 0.0)
        return vec3(0.0);
#endif

    incoming = incoming * shadowsVisibility(light, worldPosition, geometricNormal);
#ifdef YUP_3D_CONTACT_SHADOWS
    incoming = incoming * contact;
#endif
    if (incoming == vec3(0.0))
        return vec3(0.0);

    return bsdfEvaluate(s, l) * incoming;
}

#ifdef YUP_3D_TIER_FULL
// The cluster of a fragment, as LightClusters::getClusterIndex computes it: the screen tile from
// the top-left pixel, then the exponential depth slice of its view depth
uint lightsClusterIndex(vec3 worldPosition)
{
    vec2 tiles = lighting.clusterGrid.xy;
    vec2 tile = clamp(floor(gl_FragCoord.xy * tiles * view.viewport.zw), vec2(0.0), tiles - 1.0);

    float nearPlane = lighting.clusterDepth.x;
    float slices = lighting.clusterGrid.z;
    float viewDepth = -(view.view * vec4(worldPosition, 1.0)).z;
    float slice = viewDepth > nearPlane ? min(floor(log(viewDepth / nearPlane) * slices / log(lighting.clusterDepth.y / nearPlane)), slices - 1.0) : 0.0;

    return uint(tile.x + tiles.x * (tile.y + tiles.y * slice));
}

// The light loop: every directional light, then the lights of the fragment's cluster
vec3 lightsShade(BsdfSurface s, LightsLookup lookup, vec3 worldPosition, vec3 geometricNormal)
{
    vec3 color = vec3(0.0);
    int directionalLights = int(lighting.clusterDepth.z);

    for (int i = 0; i < directionalLights; ++i)
        color += lightsShadeLight(s, lookup, lights[i], worldPosition, geometricNormal);

    uint slot = lightsClusterIndex(worldPosition) * 64u;
    uint count = min(clusterLights[slot], 63u);

    for (uint i = 0u; i < count; ++i)
        color += lightsShadeLight(s, lookup, lights[clusterLights[slot + 1u + i]], worldPosition, geometricNormal);

    return color;
}
#else
// The light loop over the draw's lights
vec3 lightsShade(BsdfSurface s, LightsLookup lookup, vec3 worldPosition, vec3 geometricNormal)
{
    vec3 color = vec3(0.0);
    int lightCount = int(lightList.count.x);

    for (int i = 0; i < 8; ++i)
    {
        if (i >= lightCount)
            break;

        color += lightsShadeLight(s, lookup, lightList.lights[i], worldPosition, geometricNormal);
    }

    return color;
}
#endif

// The flat ambient light and, when there is one, the environment: its diffuse light from the
// spherical harmonics and its reflections blurred by the filtered roughness, weighted by the split
// sum of the lookup tables plus the multiple scattering it misses. The diffuse terms are darkened by
// @p diffuseOcclusion and the specular ones by @p specularOcclusion. The ambient light reflects f0
// and leaves the diffuse what the dielectric layer does not reflect, as the lights and the
// environment do. The passes evaluate it before the light loop, so its lookups and temporaries do
// not live across the loop.
// The shaders reading the ambient occlusion (YUP_3D_AMBIENT_OCCLUSION) also pass the world-space
// @p bentNormal, along which the diffuse light of the environment is evaluated (the surface normal
// without ambient occlusion). The others keep the signature without it, so their compiled code is
// exactly what it was before ambient occlusion.
// The shaders shading the reflection probes (YUP_3D_REFLECTION_PROBES) take the reflections at
// @p worldPosition from the probes first (specularIndirect, lib/probes.glsl), the environment
// taking the share they leave. The shaders shading the irradiance volumes
// (YUP_3D_IRRADIANCE_VOLUMES) take the diffuse light at @p worldPosition from the volume first
// (diffuseIndirect, lib/probes.glsl), offset along @p geometricNormal and evaluated along the bent
// normal, the environment taking the share it leaves; the specular light does not change
// The shaders compiling the material layers (YUP_3D_MATERIAL_LAYERS) also return the irradiance the
// base diffuse receives, without its occlusion, in @p layerIrradiance, which the sheen reflects
// (lightsLayered)
#if defined(YUP_3D_AMBIENT_OCCLUSION) && defined(YUP_3D_MATERIAL_LAYERS)
vec3 lightsIndirect(BsdfSurface s, LightsLookup lookup, vec3 worldPosition, vec3 geometricNormal, vec3 bentNormal, float diffuseOcclusion, float specularOcclusion, out vec3 layerIrradiance)
#elif defined(YUP_3D_AMBIENT_OCCLUSION)
vec3 lightsIndirect(BsdfSurface s, LightsLookup lookup, vec3 worldPosition, vec3 geometricNormal, vec3 bentNormal, float diffuseOcclusion, float specularOcclusion)
#elif defined(YUP_3D_MATERIAL_LAYERS)
vec3 lightsIndirect(BsdfSurface s, LightsLookup lookup, vec3 worldPosition, vec3 geometricNormal, float diffuseOcclusion, float specularOcclusion, out vec3 layerIrradiance)
#else
vec3 lightsIndirect(BsdfSurface s, LightsLookup lookup, vec3 worldPosition, vec3 geometricNormal, float diffuseOcclusion, float specularOcclusion)
#endif
{
#ifdef YUP_3D_IRRADIANCE_VOLUMES
    // The irradiance volume takes its share of the diffuse light first, with its own intensity, and
    // the environment, scaled by its intensity below, the rest. Without a volume the shares are
    // exactly 0 and 1, so the frame shades as without them. Evaluated first, while little else is live
    float environmentDiffuseWeight = 1.0;
    vec3 volume = vec3(0.0);
#ifdef YUP_3D_AMBIENT_OCCLUSION
    if (lighting.probeInfo.z > 0.0)
        volume = diffuseIndirect(worldPosition, geometricNormal, bentNormal, environmentDiffuseWeight);
#else
    if (lighting.probeInfo.z > 0.0)
        volume = diffuseIndirect(worldPosition, geometricNormal, s.n, environmentDiffuseWeight);
#endif
#endif

    vec3 color = lighting.ambient.rgb * (s.diffuseColor * bsdfDiffuseWeight(s.dielectricF0) * diffuseOcclusion + s.f0 * specularOcclusion);
#ifdef YUP_3D_MATERIAL_LAYERS
    layerIrradiance = lighting.ambient.rgb;
#endif

#if defined(YUP_3D_IRRADIANCE_VOLUMES)
    if (lighting.environmentInfo.x > 0.0 || lighting.probeInfo.y > 0.0 || lighting.probeInfo.z > 0.0)
#elif defined(YUP_3D_REFLECTION_PROBES)
    if (lighting.environmentInfo.x > 0.0 || lighting.probeInfo.y > 0.0)
#else
    if (lighting.environmentInfo.x > 0.0)
#endif
    {
        // The filtered roughness, so the reflections blur where the highlights do
        float environmentRoughness = sqrt(sqrt(s.alpha2));
        vec3 r = reflect(-s.v, s.n);

        vec3 specularWeight = bsdfSplitSum(s.f0, s.f90, lookup.lobe);

        // Reflections pointing into the surface are hidden by it
        float horizon = clamp(1.0 + dot(r, geometricNormal), 0.0, 1.0);

        vec3 irradiance = environmentIrradiance(s.n);
#ifdef YUP_3D_AMBIENT_OCCLUSION
        vec3 diffuse = s.diffuseColor * environmentIrradiance(bentNormal) * bsdfEnvironmentDiffuseWeight(s, lookup.lobe);
#else
        vec3 diffuse = s.diffuseColor * irradiance * bsdfEnvironmentDiffuseWeight(s, lookup.lobe);
#endif
#ifdef YUP_3D_IRRADIANCE_VOLUMES
        diffuse *= environmentDiffuseWeight;
        color += s.diffuseColor * volume * bsdfEnvironmentDiffuseWeight(s, lookup.lobe) * diffuseOcclusion;
#endif
#if defined(YUP_3D_MATERIAL_LAYERS) && defined(YUP_3D_AMBIENT_OCCLUSION)
        layerIrradiance += volume + environmentIrradiance(bentNormal) * (environmentDiffuseWeight * lighting.environmentInfo.x);
#elif defined(YUP_3D_MATERIAL_LAYERS)
        layerIrradiance += volume + irradiance * (environmentDiffuseWeight * lighting.environmentInfo.x);
#endif
#ifdef YUP_3D_REFLECTION_PROBES
        // The reflection probes take their share of the specular light first, in selection order,
        // with their own intensity; the environment, scaled by its intensity below, takes the rest.
        // Both keep the multiple scattering compensation of their share, the probes' over their
        // prefiltered radiance (preflight R23). Without a probe the shares are exactly 1 and 0, so
        // the frame shades as without them
        float environmentWeight = 1.0;
        vec3 probes = vec3(0.0);
        if (lighting.probeInfo.y > 0.0)
            probes = specularIndirect(worldPosition, r, environmentRoughness, environmentWeight);

        vec3 multiple = bsdfMultipleScattering(s, lookup.lobe, specularWeight);
        vec3 specular = environmentRadiance(r, environmentRoughness) * environmentWeight * specularWeight * (horizon * horizon)
                      + irradiance * multiple * environmentWeight;

        color += probes * (specularWeight * (horizon * horizon) + multiple) * specularOcclusion;
#else
        vec3 specular = environmentRadiance(r, environmentRoughness) * specularWeight * (horizon * horizon)
                      + irradiance * bsdfMultipleScattering(s, lookup.lobe, specularWeight);
#endif

        color += (diffuse * diffuseOcclusion + specular * specularOcclusion) * lighting.environmentInfo.x;
    }

    return color;
}

#ifdef YUP_3D_MATERIAL_LAYERS
#ifndef YUP_3D_IRRADIANCE_VOLUMES
#error "The material layers are shaded with the probes and volumes: define YUP_3D_REFLECTION_PROBES and YUP_3D_IRRADIANCE_VOLUMES too"
#endif

// The clearcoat (KHR_materials_clearcoat) and sheen (KHR_materials_sheen) layers of the materials,
// over the base the functions above shade (preflight R26), compiled only into the shaders defining
// YUP_3D_MATERIAL_LAYERS. lightsLayered() evaluates them on their own, after the base indirect light
// and before the base light loop, and returns what they reflect and how much of the base they leave:
// so neither the base loop nor lightsIndirect carry any layer state, and a material without layers
// shades exactly as in the default variants. Layered pixels walk the lights twice, shadows included.
// Every light, the indirect light and the emission go under the layers as one: the sheen dims them by
// 1 - max3 (sheenColor) E (n.v), the Khronos albedo scaling without its n.l term, which would need the
// base of each light again in the layers' loop, and the clearcoat by 1 - clearcoat Fc (n.v)

// The directional albedo E of a white sheen (layer 1 z of the tables) for a cosine with the shading
// normal and a perceptual sheen roughness
float lightsSheenAlbedo(float cosine, float roughness)
{
    vec2 uv = (vec2(sqrt(1.0 - cosine), roughness) * 63.0 + 0.5) / 64.0;
    return textureLod(sampler2DArray(lookupLuts, linearClamp), vec3(uv, 1.0), 0.0).z;
}

// The layers of a surface from the clearcoat and sheen rows of its GpuMaterial, the clearcoat over
// @p clearcoatNormal, each read from the tables at its own roughness: the clearcoat's lobe (layer 1
// xy) and the sheen albedo E (n.v) (layer 1 z). Nothing is read without a layer
BsdfLayers lightsLayers(BsdfSurface s, vec4 clearcoat, vec4 sheen, vec3 clearcoatNormal)
{
    BsdfLayers layers = bsdfNoLayers();

    if (clearcoat.x > 0.0)
    {
        layers.clearcoat = clearcoat.x;
        layers.clearcoatNormal = clearcoatNormal;
        layers.clearcoatRoughness = clamp(clearcoat.y, 0.04, 1.0);
        float alpha = layers.clearcoatRoughness * layers.clearcoatRoughness;
        layers.clearcoatAlpha2 = alpha * alpha;
        layers.clearcoatNDotV = max(dot(clearcoatNormal, s.v), 0.0001);
        layers.clearcoatBase = 1.0 - clearcoat.x * bsdfClearcoatFresnel(layers.clearcoatNDotV);
        layers.clearcoatUv = (vec2(sqrt(1.0 - layers.clearcoatNDotV), layers.clearcoatRoughness) * 63.0 + 0.5) / 64.0;
        layers.clearcoatLobe = textureLod(sampler2DArray(lookupLuts, linearClamp), vec3(layers.clearcoatUv, 1.0), 0.0).xy;
    }

    float sheenMax = max(max(sheen.r, sheen.g), sheen.b);
    if (sheenMax > 0.0)
    {
        layers.sheenColor = sheen.rgb;
        layers.sheenMax = sheenMax;
        layers.sheenRoughness = clamp(sheen.w, 5.0 / 63.0, 1.0); // LookupTables::minSheenRoughness, the center of row 5
        layers.sheenAlbedo = lightsSheenAlbedo(s.nDotV, layers.sheenRoughness);
    }

    return layers;
}

#ifdef YUP_3D_AREA_LIGHTS
// What the layers reflect of a light with radius > 0, per lightsEvaluateArea's disk: the sheen BRDF
// towards the center times pi times the clamped cosine over the disk, as for a small light (no LTC
// fit of the Charlie lobe), and the clearcoat through the LTC tables at its own roughness, in the
// frame of its own normal. The disk is the one lightsEvaluateArea builds
vec3 lightsEvaluateLayersArea(GpuLight light, BsdfSurface s, BsdfLayers layers, vec3 worldPosition)
{
    vec3 l;
    vec3 incoming = lightIncoming(light, worldPosition, l);
    if (incoming == vec3(0.0))
        return vec3(0.0);

    float radius = light.spotRadiusShadow.z;
    vec3 center;
    vec3 facing;
    float diskRadius;

    if (light.positionType.w < 0.5)
    {
        float angle = min(radius, 1.5);
        float sine = sin(angle);
        incoming /= yupPi * sine * sine;
        center = l;
        facing = -l;
        diskRadius = tan(angle);
    }
    else
    {
        center = light.positionType.xyz - worldPosition;
        float distanceSquared = max(dot(center, center), 0.0001);
        incoming *= distanceSquared / (yupPi * radius * radius);

        if (dot(l, l) == 0.0)
        {
            l = s.n;
            center = s.n * sqrt(distanceSquared);
        }

        facing = -l;
        diskRadius = radius * inversesqrt(max(1.0 - radius * radius / distanceSquared, 1.0e-4));
    }

    vec3 ex = normalize(cross(facing, abs(facing.z) < 0.9 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0))) * diskRadius;
    vec3 ey = cross(facing, ex);
    vec3 reflected = vec3(0.0);

    if (layers.sheenMax > 0.0)
    {
        vec3 t1 = s.v - s.n * dot(s.v, s.n);
        t1 = dot(t1, t1) > 1.0e-8 ? normalize(t1) : normalize(cross(s.n, abs(s.n.z) < 0.9 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0)));
        mat3 toLocal = transpose(mat3(t1, cross(s.n, t1), s.n));

        float formFactor = lightsEllipseIntegral(toLocal * center, toLocal * ex, toLocal * ey);
        if (formFactor > 0.0)
            reflected = bsdfEvaluateSheen(s, layers, l, max(dot(s.n, l), 1.0e-4)) * (yupPi * formFactor * layers.clearcoatBase);
    }

    if (layers.clearcoat > 0.0)
    {
        vec3 n = layers.clearcoatNormal;
        vec3 t1 = s.v - n * dot(s.v, n);
        t1 = dot(t1, t1) > 1.0e-8 ? normalize(t1) : normalize(cross(n, abs(n.z) < 0.9 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0)));
        mat3 toLocal = transpose(mat3(t1, cross(n, t1), n));

        vec4 m = textureLod(sampler2DArray(lookupLuts, linearClamp), vec3(layers.clearcoatUv, 0.0), 0.0);
        mat3 inverseMatrix = mat3(vec3(m.x, 0.0, m.w), vec3(0.0, m.z, 0.0), vec3(m.y, 0.0, 1.0)) * toLocal;

        vec2 lobe = layers.clearcoatLobe;
        reflected += vec3(layers.clearcoat * (bsdfClearcoatF0 * (lobe.x - lobe.y) + lobe.y)
                          * lightsEllipseIntegral(inverseMatrix * center, inverseMatrix * ex, inverseMatrix * ey));
    }

    return incoming * reflected;
}
#endif

// What the layers reflect of one light, shadowed as lightsShadeLight shadows the base: the sheen,
// under the clearcoat, then the clearcoat, which reflects the lights its own normal faces
vec3 lightsShadeLayersLight(BsdfSurface s, BsdfLayers layers, GpuLight light, vec3 worldPosition, vec3 geometricNormal)
{
#ifdef YUP_3D_AREA_LIGHTS
    if (light.spotRadiusShadow.z > 0.0)
    {
        vec3 reflected = lightsEvaluateLayersArea(light, s, layers, worldPosition);
        if (reflected != vec3(0.0))
            reflected = reflected * shadowsVisibility(light, worldPosition, geometricNormal);
#ifdef YUP_3D_CONTACT_SHADOWS
        if (reflected != vec3(0.0))
            reflected = reflected * shadowsContact(light, worldPosition, geometricNormal);
#endif

        return reflected;
    }
#endif

    vec3 l;
    vec3 incoming = lightIncoming(light, worldPosition, l);
    float nDotL = layers.sheenMax > 0.0 ? dot(s.n, l) : 0.0;
    float coatDotL = layers.clearcoat > 0.0 ? dot(layers.clearcoatNormal, l) : 0.0;
    if ((nDotL <= 0.0 && coatDotL <= 0.0) || incoming == vec3(0.0))
        return vec3(0.0);

#ifdef YUP_3D_CONTACT_SHADOWS
    float contact = shadowsContact(light, worldPosition, geometricNormal);
    if (contact <= 0.0)
        return vec3(0.0);
#endif

    incoming = incoming * shadowsVisibility(light, worldPosition, geometricNormal);
#ifdef YUP_3D_CONTACT_SHADOWS
    incoming = incoming * contact;
#endif
    if (incoming == vec3(0.0))
        return vec3(0.0);

    vec3 reflected = coatDotL > 0.0 ? vec3(bsdfEvaluateClearcoat(s, layers, l)) : vec3(0.0);
    if (nDotL > 0.0)
        reflected += bsdfEvaluateSheen(s, layers, l, nDotL) * (nDotL * layers.clearcoatBase);

    return reflected * incoming;
}

// What the layers reflect of the lights the base loop (lightsShade) walks, in the same order
vec3 lightsShadeLayers(BsdfSurface s, BsdfLayers layers, vec3 worldPosition, vec3 geometricNormal)
{
    vec3 color = vec3(0.0);
#ifdef YUP_3D_TIER_FULL
    int directionalLights = int(lighting.clusterDepth.z);

    for (int i = 0; i < directionalLights; ++i)
        color += lightsShadeLayersLight(s, layers, lights[i], worldPosition, geometricNormal);

    uint slot = lightsClusterIndex(worldPosition) * 64u;
    uint count = min(clusterLights[slot], 63u);

    for (uint i = 0u; i < count; ++i)
        color += lightsShadeLayersLight(s, layers, lights[clusterLights[slot + 1u + i]], worldPosition, geometricNormal);
#else
    int lightCount = int(lightList.count.x);

    for (int i = 0; i < 8; ++i)
    {
        if (i >= lightCount)
            break;

        color += lightsShadeLayersLight(s, layers, lightList.lights[i], worldPosition, geometricNormal);
    }
#endif

    return color;
}

// What the layers reflect of the indirect light, the sheen under the clearcoat. The sheen reflects
// sheenColor E (n.v) of @p irradiance, what the base diffuse receives in lightsIndirect (the ambient
// light, the irradiance volume and the environment's share), darkened by @p diffuseOcclusion. The
// clearcoat reflects the ambient light by its f0, then the reflection probes and the environment for
// their leftover share, along the reflection about its normal at its roughness, weighted by the
// split sum at f0 = 0.04, the horizon and @p clearcoatOcclusion, its specular occlusion
vec3 lightsIndirectLayers(BsdfSurface s, BsdfLayers layers, vec3 worldPosition, vec3 geometricNormal, vec3 irradiance, float diffuseOcclusion, float clearcoatOcclusion)
{
    vec3 reflected = vec3(0.0);

    if (layers.sheenMax > 0.0)
        reflected = layers.sheenColor * (layers.sheenAlbedo * diffuseOcclusion * layers.clearcoatBase) * irradiance;

    if (layers.clearcoat > 0.0)
    {
        vec3 radiance = lighting.ambient.rgb * bsdfClearcoatF0;
        if (lighting.environmentInfo.x > 0.0 || lighting.probeInfo.y > 0.0 || lighting.probeInfo.z > 0.0)
        {
            vec3 r = reflect(-s.v, layers.clearcoatNormal);
            float horizon = clamp(1.0 + dot(r, geometricNormal), 0.0, 1.0);
            float weight = bsdfSplitSum(vec3(bsdfClearcoatF0), vec3(1.0), layers.clearcoatLobe).x;

            float environmentWeight = 1.0;
            vec3 probes = vec3(0.0);
            if (lighting.probeInfo.y > 0.0)
                probes = specularIndirect(worldPosition, r, layers.clearcoatRoughness, environmentWeight);

            radiance += (probes + environmentRadiance(r, layers.clearcoatRoughness) * (environmentWeight * lighting.environmentInfo.x)) * (weight * horizon * horizon);
        }

        reflected += radiance * (layers.clearcoat * clearcoatOcclusion);
    }

    return reflected;
}

// The layers of a surface (lightsLayers) and what they reflect of the lights and of the indirect
// light, plus the @p emission they dim, with the base under them: the pass adds the base light and
// the base indirect light times @p baseScale to the result. Without layers this is the emission
// and a scale of exactly 1. The clearcoat lies over the surface's @p geometricNormal (the
// interpolated normal on raster, the shading normal of the G-buffer on full); @p irradiance is the
// layerIrradiance of lightsIndirect. The occlusions follow the base's: the diffuse one is
// min (@p visibility, @p materialOcclusion), and the clearcoat's specular one is taken at its own
// view angle and roughness in the shaders reading the ambient occlusion (1 for @p visibility
// without it)
vec3 lightsLayered(BsdfSurface s, vec4 clearcoat, vec4 sheen, vec3 emission, vec3 worldPosition, vec3 geometricNormal,
                   vec3 irradiance, float visibility, float materialOcclusion, out float baseScale)
{
    baseScale = 1.0;
    if (clearcoat.x <= 0.0 && max(max(sheen.r, sheen.g), sheen.b) <= 0.0)
        return emission;

    BsdfLayers layers = lightsLayers(s, clearcoat, sheen, geometricNormal);
    baseScale = (1.0 - layers.sheenMax * layers.sheenAlbedo) * layers.clearcoatBase;

#ifdef YUP_3D_AMBIENT_OCCLUSION
    float clearcoatOcclusion = min(gtaoSpecularOcclusion(layers.clearcoatNDotV, visibility, layers.clearcoatRoughness), materialOcclusion);
#else
    float clearcoatOcclusion = materialOcclusion;
#endif

    return emission * layers.clearcoatBase
         + lightsIndirectLayers(s, layers, worldPosition, geometricNormal, irradiance, min(visibility, materialOcclusion), clearcoatOcclusion)
         + lightsShadeLayers(s, layers, worldPosition, geometricNormal);
}
#endif

#endif
