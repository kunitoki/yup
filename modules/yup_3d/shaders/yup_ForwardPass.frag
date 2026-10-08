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

#include "lib/bsdf.glsl"
#include "lib/color.glsl"
#include "lib/view.glsl"

// glTF metallic-roughness shading with KHR_lights_punctual lights and a flat ambient term.
// Normal mapping uses the mesh tangents when present, and otherwise builds the tangent frame from
// screen-space derivatives; the same derivatives filter the highlights of smooth surfaces.
// An optional environment adds reflections from a latitude-longitude image whose mip levels are
// blurred per roughness, and diffuse light from nine spherical harmonics coefficients. One
// directional light can be shadowed from a packed depth map, filtered with a 4x4 tent.
// Color textures are decoded from sRGB here. The result is exposed, then written per output mode:
// tone mapped (Reinhard, ACES or AgX) and sRGB encoded for an rgba8unorm target, reversibly tone
// mapped, or linear.

layout(set = 0, binding = 3) uniform sampler pointClamp;

// Must match ForwardLightingBlock in yup_ForwardPass.cpp and LightingBlock in yup_Background.frag
layout(set = 1, binding = 0) uniform LightingBlock
{
    vec4 ambient;             // rgb linear ambient, w unused (exposure is view.cameraPosition.w)
    vec4 lightInfo;           // x light count
    vec4 lightPositions[8];   // xyz world position, w type (0 directional, 1 point, 2 spot)
    vec4 lightDirections[8];  // xyz unit direction the light shines towards, w range (0 infinite)
    vec4 lightColors[8];      // rgb linear color, w intensity
    vec4 lightSpots[8];       // x cone scale, y cone offset
    mat4 shadowMatrix;        // world to shadow map clip space, depth from 0 to 1
    vec4 shadowInfo;          // shadowed light index (-1 for none), texel size, normal offset, depth bias
    vec4 environmentInfo;     // x intensity (0 for none), y last level, z background blur level, w output mode (0 ldrInline, 1 hdrReversible, 2 hdrLinear)
    vec4 irradiance[9];       // rgb coefficients of the environment diffuse light
} lighting;

layout(set = 1, binding = 1) uniform texture2D u_environment;
layout(set = 1, binding = 2) uniform sampler s_environment;
layout(set = 1, binding = 3) uniform texture2D u_shadow; // packed depth, read with texelFetch

// Must match ForwardMaterialBlock in yup_ForwardPass.cpp
layout(set = 2, binding = 1) uniform MaterialBlock
{
    vec4 baseColorFactor;
    vec4 emissiveFactor;      // rgb emissive, w alpha cutoff
    vec4 params;              // metallic, roughness, normal scale, occlusion strength
    vec4 flags;               // alpha mode (0 opaque, 1 mask, 2 blend), base color sRGB, emissive sRGB, has normal texture
    vec4 specular;            // rgb specular color, w specular factor (KHR_materials_specular); GpuMaterial from here
    vec4 extended;            // x ior (KHR_materials_ior), y emissive strength (KHR_materials_emissive_strength), zw reserved
} material;

layout(set = 2, binding = 2) uniform texture2D u_baseColor;
layout(set = 2, binding = 3) uniform texture2D u_metallicRoughness;
layout(set = 2, binding = 4) uniform texture2D u_normal;
layout(set = 2, binding = 5) uniform texture2D u_occlusion;
layout(set = 2, binding = 6) uniform texture2D u_emissive;
layout(set = 2, binding = 7) uniform sampler s_baseColor;
layout(set = 2, binding = 8) uniform sampler s_metallicRoughness;
layout(set = 2, binding = 9) uniform sampler s_normal;
layout(set = 2, binding = 10) uniform sampler s_occlusion;
layout(set = 2, binding = 11) uniform sampler s_emissive;

layout(location = 0) in vec3 v_worldPosition;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec4 v_color;
layout(location = 6) in vec4 v_tangent;

#ifdef YUP_3D_TIER_FULL
layout(location = 4) in vec4 v_currentClip;
layout(location = 5) in vec4 v_previousClip;
#endif

layout(location = 0) out vec4 fragColor;

#ifdef YUP_3D_TIER_FULL
layout(location = 1) out vec2 outVelocity;
#endif

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

vec3 environmentRadiance(vec3 d, float roughness)
{
    // Around the vertical axis from +Z, and from the zenith down; the level follows the roughness
    // atan is undefined at the poles, where both arguments are zero
    vec2 uv = vec2(atan(d.x, d.z + 1.0e-6) / (2.0 * yupPi), acos(clamp(d.y, -1.0, 1.0)) / yupPi);
    return textureLod(sampler2D(u_environment, s_environment), uv, roughness * lighting.environmentInfo.y).rgb;
}

float unpackDepth(vec4 rgba)
{
    return dot(rgba, vec4(1.0, 1.0 / 255.0, 1.0 / 65025.0, 1.0 / 16581375.0));
}

float shadowVisibility(vec3 worldPosition, vec3 geometricNormal, vec3 l)
{
    // Offset along the normal, more where the light grazes the surface
    float nDotL = clamp(dot(geometricNormal, l), 0.0, 1.0);
    vec3 position = worldPosition + geometricNormal * (lighting.shadowInfo.z * (1.0 - nDotL));

    vec4 clip = lighting.shadowMatrix * vec4(position, 1.0);
    vec2 coords = vec2(clip.x * 0.5 + 0.5, 0.5 - clip.y * 0.5);
    if (coords.x <= 0.0 || coords.y <= 0.0 || coords.x >= 1.0 || coords.y >= 1.0 || clip.z >= 1.0)
        return 1.0;

    float texel = lighting.shadowInfo.y;
    float depth = clip.z - lighting.shadowInfo.w;

    // Bilinear comparisons over a 3x3 texel footprint: a 4x4 tent
    vec2 grid = coords / texel - 0.5;
    vec2 base = floor(grid);
    vec2 f = grid - base;

    int size = int(1.0 / texel + 0.5);
    ivec2 origin = ivec2(base) - ivec2(1);

    float lit = 0.0;
    for (int j = 0; j < 4; ++j)
    {
        float wy = j == 0 ? 1.0 - f.y : (j == 3 ? f.y : 1.0);

        for (int i = 0; i < 4; ++i)
        {
            float wx = i == 0 ? 1.0 - f.x : (i == 3 ? f.x : 1.0);
            ivec2 position = clamp(origin + ivec2(i, j), ivec2(0), ivec2(size - 1));
            float stored = unpackDepth(texelFetch(sampler2D(u_shadow, pointClamp), position, 0));
            lit += wx * wy * (depth <= stored ? 1.0 : 0.0);
        }
    }

    return lit / 9.0;
}

vec3 perturbNormal(vec3 n, vec3 tangentNormal, float side)
{
    // The derivatives come first: they are undefined once neighboring fragments take another branch
    vec3 dpdx = dFdx(v_worldPosition);
    vec3 dpdy = dFdy(v_worldPosition);
    vec2 duvdx = dFdx(v_uv);
    vec2 duvdy = dFdy(v_uv);

    // Mesh tangents, when present and not parallel to the normal
    if (abs(v_tangent.w) > 0.5)
    {
        vec3 projected = v_tangent.xyz - n * dot(n, v_tangent.xyz);
        float projectedLength = length(projected);

        if (projectedLength > 1.0e-6)
        {
            vec3 t = projected / projectedLength;
            vec3 b = cross(n, t) * sign(v_tangent.w);
            return normalize(mat3(t * side, b * side, n * side) * tangentNormal);
        }
    }

    float det = duvdx.x * duvdy.y - duvdy.x * duvdx.y;
    if (abs(det) < 1.0e-12)
        return n * side;

    vec3 t = (duvdy.y * dpdx - duvdx.y * dpdy) / det;
    vec3 dpdv = (duvdx.x * dpdy - duvdy.x * dpdx) / det;
    t = t - n * dot(n, t);

    float tangentLength = length(t);
    if (tangentLength < 1.0e-12)
        return n * side;

    // Back faces flip the whole tangent frame, not just the normal
    t = t / tangentLength;
    vec3 b = cross(n, t);

    // glTF bitangents point towards decreasing v: mirrored texture coordinates or transforms flip them
    if (dot(b, dpdv) > 0.0)
        b = -b;

    return normalize(mat3(t * side, b * side, n * side) * tangentNormal);
}

void main()
{
    vec4 baseColorSample = texture(sampler2D(u_baseColor, s_baseColor), v_uv);
    if (material.flags.y > 0.5)
        baseColorSample.rgb = yupSrgbToLinear(baseColorSample.rgb);

    vec4 baseColor = material.baseColorFactor * baseColorSample * v_color;

    // The normal uses screen-space derivatives: compute it before any fragment is discarded
    float side = gl_FrontFacing ? 1.0 : -1.0;
    vec3 n = normalize(v_normal);
    vec3 geometricNormal = n * side;

    if (material.flags.w > 0.5)
    {
        vec3 tangentNormal = texture(sampler2D(u_normal, s_normal), v_uv).xyz * 2.0 - 1.0;
        tangentNormal.xy = tangentNormal.xy * material.params.z;
        n = perturbNormal(n, tangentNormal, side);
    }
    else
    {
        n = n * side;
    }

    // Specular antialiasing: widen the highlight where the normal varies within the pixel
    float kernelRoughness = bsdfKernelRoughness(n);

    if (material.flags.x < 0.5)
    {
        baseColor.a = 1.0;
    }
    else if (material.flags.x < 1.5)
    {
        if (baseColor.a < material.emissiveFactor.w)
            discard;

        baseColor.a = 1.0;
    }

    vec4 metallicRoughness = texture(sampler2D(u_metallicRoughness, s_metallicRoughness), v_uv);
    float metallic = clamp(material.params.x * metallicRoughness.b, 0.0, 1.0);
    float roughness = clamp(material.params.y * metallicRoughness.g, 0.04, 1.0);
    float occlusion = 1.0 + material.params.w * (texture(sampler2D(u_occlusion, s_occlusion), v_uv).r - 1.0);

    vec3 emissive = texture(sampler2D(u_emissive, s_emissive), v_uv).rgb;
    if (material.flags.z > 0.5)
        emissive = yupSrgbToLinear(emissive);

    emissive = emissive * material.emissiveFactor.rgb * material.extended.y;

    // KHR_materials_specular scales the whole dielectric reflectance, grazing incidence included
    vec3 v = normalize(view.cameraPosition.xyz - v_worldPosition);
    vec3 dielectricF0 = bsdfDielectricF0(material.extended.x, material.specular.rgb, material.specular.w);
    float dielectricF90 = material.specular.w;
    vec3 f0 = mix(dielectricF0, baseColor.rgb, metallic);
    vec3 f90 = mix(vec3(dielectricF90), vec3(1.0), metallic);
    vec3 diffuseColor = baseColor.rgb * (1.0 - metallic);
    float alpha = roughness * roughness;
    float alpha2 = bsdfFilteredAlpha2(alpha, kernelRoughness);
    float nDotV = max(dot(n, v), 0.0001);

    BsdfSurface surface = BsdfSurface(n, v, diffuseColor, f0, f90, dielectricF0, dielectricF90, alpha2, nDotV);

    vec3 color = vec3(0.0);
    int lightCount = int(lighting.lightInfo.x);

    for (int i = 0; i < 8; ++i)
    {
        if (i >= lightCount)
            break;

        vec4 positionType = lighting.lightPositions[i];
        vec4 directionRange = lighting.lightDirections[i];
        vec4 colorIntensity = lighting.lightColors[i];

        vec3 l = -directionRange.xyz;
        float attenuation = 1.0;

        if (positionType.w > 0.5)
        {
            vec3 toLight = positionType.xyz - v_worldPosition;
            float distanceSquared = max(dot(toLight, toLight), 0.0001);
            float lightDistance = sqrt(distanceSquared);

            l = toLight / lightDistance;
            attenuation = 1.0 / distanceSquared;

            if (directionRange.w > 0.0)
            {
                float ratio = lightDistance / directionRange.w;
                float ratio2 = ratio * ratio;
                attenuation = attenuation * clamp(1.0 - ratio2 * ratio2, 0.0, 1.0);
            }

            if (positionType.w > 1.5)
            {
                vec4 spot = lighting.lightSpots[i];
                float cone = clamp(dot(directionRange.xyz, -l) * spot.x + spot.y, 0.0, 1.0);
                attenuation = attenuation * cone * cone;
            }
        }

        float nDotL = dot(n, l);
        if (nDotL <= 0.0 || attenuation <= 0.0)
            continue;

        if (float(i) == lighting.shadowInfo.x)
        {
            attenuation = attenuation * shadowVisibility(v_worldPosition, geometricNormal, l);
            if (attenuation <= 0.0)
                continue;
        }

        color += bsdfEvaluate(surface, l) * colorIntensity.rgb * (colorIntensity.w * attenuation);
    }

    color += lighting.ambient.rgb * (diffuseColor + f0) * occlusion;

    if (lighting.environmentInfo.x > 0.0)
    {
        // The filtered roughness, so the reflections blur where the highlights do
        float environmentRoughness = sqrt(sqrt(alpha2));
        vec3 r = reflect(-v, n);

        vec2 brdf = bsdfEnvironmentBrdf(environmentRoughness, nDotV);
        vec3 specularWeight = f0 * brdf.x + f90 * brdf.y;

        // Reflections pointing into the surface are hidden by it
        float horizon = clamp(1.0 + dot(r, geometricNormal), 0.0, 1.0);

        vec3 diffuse = diffuseColor * environmentIrradiance(n) * bsdfEnvironmentDiffuseWeight(surface, brdf);
        vec3 specular = environmentRadiance(r, environmentRoughness) * specularWeight * (horizon * horizon);

        color += (diffuse + specular) * (occlusion * lighting.environmentInfo.x);
    }

    color += emissive;

    vec3 exposed = color * view.cameraPosition.w;

    if (lighting.environmentInfo.w < 0.5)
        fragColor = vec4(yupLinearToSrgb(yupToneMap(exposed, view.clipPlanes.w)), baseColor.a);
    else if (lighting.environmentInfo.w < 1.5)
        fragColor = vec4(yupReversibleToneMap(min(exposed, vec3(yupMaxHalf))), baseColor.a);
    else
        fragColor = vec4(min(exposed, vec3(yupMaxHalf)), baseColor.a);

#ifdef YUP_3D_TIER_FULL
    outVelocity = (v_currentClip.xy / v_currentClip.w - v_previousClip.xy / v_previousClip.w) * vec2(0.5, -0.5);
#endif
}
