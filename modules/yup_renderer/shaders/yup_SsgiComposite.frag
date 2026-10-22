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

// For the clearcoat Fresnel of lib/bsdf.glsl
#define YUP_3D_MATERIAL_LAYERS

#include "lib/color.glsl"
#include "lib/view.glsl"
#include "lib/octahedral.glsl"
#include "lib/bsdf.glsl"
#include "lib/material_types.glsl"

// The screen-space diffuse GI of the full tier (AmbientOcclusionPass::addIndirectDiffuse()), added
// to the scene color right after LightingPass with additive blending: the light the horizon search
// gathered from the occluded arcs, which the probes, the volumes and the environment, darkened by
// the visibility, do not light. Each G-buffer texel adds its diffuse color times the gathered
// irradiance, weighted as lightsIndirect() weighs the environment's diffuse light: by what the
// dielectric specular leaves (bsdfEnvironmentDiffuseWeight), by the material occlusion and, for a
// layered material, by the base scale of its clearcoat and sheen, its sheen reflecting the gathered
// light too as lightsIndirectLayers() does. The gathered light and the result are exposed, and alpha
// stays as it is. Both this and LightingPass stop at yupMaxHalfAddend, so the blended sum fits a half float.

layout(set = 0, binding = 1) uniform sampler linearClamp;
layout(set = 0, binding = 3) uniform sampler pointClamp;

// The lookup tables of LookupTables: layer 1 holds the GGX lobe magnitude, its Fresnel part and the sheen albedo
layout(set = 0, binding = 7) uniform texture2DArray lookupLuts;

// The frame's MaterialTable, indexed by the material id in RT2
layout(set = 1, binding = 8) readonly buffer Materials { GpuMaterial materials[]; };

layout(set = 1, binding = 9) uniform texture2D u_albedoOcclusion;   // RT0
layout(set = 1, binding = 10) uniform texture2D u_normalRoughness;  // RT1
layout(set = 1, binding = 11) uniform texture2D u_emissionMaterial; // RT2
layout(set = 1, binding = 12) uniform texture2D u_depth;

// AmbientOcclusionPass::getIndirectDiffuse(), at half resolution: the gathered irradiance in rgb,
// the share of the hemisphere it came from in a
layout(set = 1, binding = 13) uniform texture2D u_indirectDiffuse;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

// The view depth of a stored depth, for perspective and orthographic projections (as lib/gtao.glsl)
float ssgiViewDepth(float depth)
{
    mat4 p = view.projection;
    return (p[3][2] - depth * p[3][3]) / (depth * p[2][3] - p[2][2]);
}

// The gathered light of a pixel, upsampled from half resolution with the depth- and normal-aware
// weights LightingPass reads the ambient occlusion with (lightingAmbientOcclusion), so the light
// does not bleed across depth or normal edges either. Without any tap traced from a surface, none
vec4 ssgiIndirectDiffuse(ivec2 pixel, float viewDepth, vec3 n)
{
    ivec2 size = textureSize(sampler2D(u_indirectDiffuse, pointClamp), 0);
    // gtaoUpsampleUv() of lib/probes.glsl: texel i was traced at the input texel 2i
    vec2 position = (vec2(pixel) + 1.0) * 0.5 - 0.5;
    ivec2 base = ivec2(floor(position));
    vec2 f = position - vec2(base);

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

        float tapDepth = ssgiViewDepth(tapStored);
        vec3 tapNormal = octahedralDecode(texelFetch(sampler2D(u_normalRoughness, pointClamp), source, 0).xy);
        float facing = max(dot(tapNormal, n), 0.0);
        facing *= facing;
        facing *= facing;
        facing *= facing;

        vec2 bilinear = mix(1.0 - f, f, vec2(offset));
        float similar = clamp(1.0 - abs(tapDepth - viewDepth) / (0.05 * abs(viewDepth)), 0.0, 1.0) * facing;
        float weight = bilinear.x * bilinear.y * (similar + 1.0e-3);

        sum += texelFetch(sampler2D(u_indirectDiffuse, pointClamp), texel, 0) * weight;
        weights += weight;
    }

    return weights > 0.0 ? sum / weights : vec4(0.0);
}

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    float depth = texelFetch(sampler2D(u_depth, pointClamp), pixel, 0).r;
    fragColor = vec4(0.0);

    if (depth >= 1.0)
        return;

    vec4 normalRoughness = texelFetch(sampler2D(u_normalRoughness, pointClamp), pixel, 0);
    vec3 n = octahedralDecode(normalRoughness.xy);

    // Only where the trace gathered something: elsewhere the pixel keeps exactly what LightingPass wrote
    vec4 gathered = ssgiIndirectDiffuse(pixel, ssgiViewDepth(depth), n);
    if (gathered.a <= 0.0)
        return;

    // The jittered matrices, as the G-buffer was rasterized
    vec4 world = view.inverseViewProjection * vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    vec3 v = normalize(view.cameraPosition.xyz - world.xyz / world.w);

    vec4 albedoOcclusion = texelFetch(sampler2D(u_albedoOcclusion, pointClamp), pixel, 0);
    int materialId = int(texelFetch(sampler2D(u_emissionMaterial, pointClamp), pixel, 0).a + 0.5);
    float roughness2 = normalRoughness.z * normalRoughness.z;

    BsdfSurface s = bsdfMaterialSurface(n, v, yupSrgbToLinear(albedoOcclusion.rgb), normalRoughness.w,
                                        materials[materialId].extended.x, materials[materialId].specular, roughness2 * roughness2);

    // lightsLookup(): layer 1 at sqrt (1 - n.v) and sqrt (alpha)
    vec2 lookupUv = (vec2(sqrt(1.0 - s.nDotV), sqrt(sqrt(s.alpha2))) * 63.0 + 0.5) / 64.0;
    vec2 lobe = textureLod(sampler2DArray(lookupLuts, linearClamp), vec3(lookupUv, 1.0), 0.0).xy;

    // Material occlusion darkens it, the visibility does not: it is the light of the occluded arcs
    vec3 irradiance = gathered.rgb * albedoOcclusion.a;
    vec3 color = s.diffuseColor * irradiance * bsdfEnvironmentDiffuseWeight(s, lobe);

    // lightsLayered(): the clearcoat and the sheen dim the base, and the sheen reflects the light too
    vec4 clearcoat = materials[materialId].clearcoat;
    vec4 sheen = materials[materialId].sheen;
    float sheenMax = max(max(sheen.r, sheen.g), sheen.b);

    if (clearcoat.x > 0.0 || sheenMax > 0.0)
    {
        float clearcoatBase = clearcoat.x > 0.0 ? 1.0 - clearcoat.x * bsdfClearcoatFresnel(s.nDotV) : 1.0;
        float sheenAlbedo = 0.0;

        if (sheenMax > 0.0)
        {
            // lightsSheenAlbedo(), from LookupTables::minSheenRoughness, the center of row 5
            vec2 sheenUv = (vec2(sqrt(1.0 - s.nDotV), clamp(sheen.w, 5.0 / 63.0, 1.0)) * 63.0 + 0.5) / 64.0;
            sheenAlbedo = textureLod(sampler2DArray(lookupLuts, linearClamp), vec3(sheenUv, 1.0), 0.0).z;
        }

        color = color * ((1.0 - sheenMax * sheenAlbedo) * clearcoatBase) + sheen.rgb * (sheenAlbedo * clearcoatBase) * irradiance;
    }

    fragColor = vec4(min(color, vec3(yupMaxHalfAddend)), 0.0);
}
