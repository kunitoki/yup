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


#ifndef YUP_3D_LIB_MATERIAL
#define YUP_3D_LIB_MATERIAL

#include "lib/bsdf.glsl"
#include "lib/color.glsl"

// The fragment stage of the passes drawing meshes (ForwardPass, GBufferPass): the varyings of
// lib/geometry.glsl, the set 2 MaterialBlock with the material textures, and materialEvaluate(),
// which samples the glTF metallic-roughness material of the fragment. Fragment stage only.
// Normal mapping uses the mesh tangents when present, and otherwise builds the tangent frame from
// screen-space derivatives. Color textures are decoded from sRGB here.

// Mirrors GeometryBindings::MaterialBlock in yup_GeometryBindings.h
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

// The material of a fragment, as materialEvaluate() samples it
struct MaterialSurface
{
    vec4 baseColor;          // linear rgb within [0, 1]; alpha 1 unless the material blends
    vec3 n;                  // unit shading normal after normal mapping, flipped on back faces
    vec3 geometricNormal;    // unit interpolated normal, flipped on back faces
    float metallic;
    float roughness;         // perceptual, from 0.04 to 1
    float kernelRoughness;   // bsdfKernelRoughness of n
    float alpha2;            // GGX alpha squared widened by the kernel roughness, from bsdfFilteredAlpha2
    float occlusion;         // 1 + strength * (texture - 1)
    vec3 emissive;           // linear, times the emissive strength
};

vec3 materialPerturbNormal(vec3 n, vec3 tangentNormal, float side)
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

// Samples the material of the fragment, discarding it when an alpha masked material cuts it out.
// The screen-space derivatives (normal mapping, kernel roughness) are taken before the discard
MaterialSurface materialEvaluate()
{
    MaterialSurface s;

    vec4 baseColorSample = texture(sampler2D(u_baseColor, s_baseColor), v_uv);
    if (material.flags.y > 0.5)
        baseColorSample.rgb = yupSrgbToLinear(baseColorSample.rgb);

    s.baseColor = material.baseColorFactor * baseColorSample * v_color;
    s.baseColor.rgb = clamp(s.baseColor.rgb, 0.0, 1.0);

    // The normal uses screen-space derivatives: compute it before any fragment is discarded
    float side = gl_FrontFacing ? 1.0 : -1.0;
    vec3 n = normalize(v_normal);
    s.geometricNormal = n * side;

    if (material.flags.w > 0.5)
    {
        vec3 tangentNormal = texture(sampler2D(u_normal, s_normal), v_uv).xyz * 2.0 - 1.0;
        tangentNormal.xy = tangentNormal.xy * material.params.z;
        s.n = materialPerturbNormal(n, tangentNormal, side);
    }
    else
    {
        s.n = n * side;
    }

    // Specular antialiasing: widen the highlight where the normal varies within the pixel
    s.kernelRoughness = bsdfKernelRoughness(s.n);

    if (material.flags.x < 0.5)
    {
        s.baseColor.a = 1.0;
    }
    else if (material.flags.x < 1.5)
    {
        if (s.baseColor.a < material.emissiveFactor.w)
            discard;

        s.baseColor.a = 1.0;
    }

    vec4 metallicRoughness = texture(sampler2D(u_metallicRoughness, s_metallicRoughness), v_uv);
    s.metallic = clamp(material.params.x * metallicRoughness.b, 0.0, 1.0);
    s.roughness = clamp(material.params.y * metallicRoughness.g, 0.04, 1.0);
    s.alpha2 = bsdfFilteredAlpha2(s.roughness * s.roughness, s.kernelRoughness);
    s.occlusion = 1.0 + material.params.w * (texture(sampler2D(u_occlusion, s_occlusion), v_uv).r - 1.0);

    vec3 emissive = texture(sampler2D(u_emissive, s_emissive), v_uv).rgb;
    if (material.flags.z > 0.5)
        emissive = yupSrgbToLinear(emissive);

    s.emissive = emissive * material.emissiveFactor.rgb * material.extended.y;

    return s;
}

#endif
