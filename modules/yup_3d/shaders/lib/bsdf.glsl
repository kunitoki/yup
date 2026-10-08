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

#ifndef YUP_3D_LIB_BSDF
#define YUP_3D_LIB_BSDF

// glTF metallic-roughness BSDF: Lambert diffuse plus a GGX microfacet specular lobe with
// height-correlated Smith visibility and Schlick Fresnel. Roughness is perceptual; alpha is its
// square and alpha2 the square of alpha.

const float yupPi = 3.14159265359;

// GGX (Trowbridge-Reitz) normal distribution
float bsdfDistributionGgx(float nDotH, float alpha2)
{
    float d = nDotH * nDotH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (yupPi * d * d);
}

// Height-correlated Smith visibility, the masking-shadowing term divided by 4 nDotV nDotL
float bsdfVisibilitySmithGgxCorrelated(float nDotV, float nDotL, float alpha2)
{
    float ggxV = nDotL * sqrt(nDotV * nDotV * (1.0 - alpha2) + alpha2);
    float ggxL = nDotV * sqrt(nDotL * nDotL * (1.0 - alpha2) + alpha2);
    return 0.5 / max(ggxV + ggxL, 0.0001);
}

// Schlick's Fresnel approximation from the reflectance at normal and grazing incidence
vec3 bsdfFresnelSchlick(vec3 f0, vec3 f90, float vDotH)
{
    return f0 + (f90 - f0) * pow(1.0 - vDotH, 5.0);
}

// Split-sum scale and bias of the specular reflectance, fitted analytically (Karis, mobile)
vec2 bsdfEnvironmentBrdf(float roughness, float nDotV)
{
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * nDotV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
}

// Specular antialiasing: the variance of the shading normal within the pixel, added to alpha2 by
// bsdfFilteredAlpha2 so very smooth curved surfaces don't sparkle (Tokuyoshi and Kaplanyan 2019,
// "Improved Geometric Specular Antialiasing"). Fragment stage only (uses derivatives): call it in
// uniform control flow, before any fragment is discarded.
float bsdfKernelRoughness(vec3 n)
{
    vec3 dndx = dFdx(n);
    vec3 dndy = dFdy(n);
    return min(0.3 * (dot(dndx, dndx) + dot(dndy, dndy)), 0.2);
}

// The GGX alpha2 widened by the kernel roughness from bsdfKernelRoughness
float bsdfFilteredAlpha2(float alpha, float kernelRoughness)
{
    return clamp(alpha * alpha + kernelRoughness, 0.0, 1.0);
}

// Dielectric reflectance at normal incidence from the index of refraction, tinted and scaled by the
// specular color and factor (KHR_materials_ior, KHR_materials_specular). 0.04 for the defaults, and
// 1 for an ior of 0, which KHR_materials_ior defines as infinite. The matching reflectance at
// grazing incidence is the specular factor.
vec3 bsdfDielectricF0(float ior, vec3 specularColor, float specularFactor)
{
    float r = (ior - 1.0) / max(ior + 1.0, 1.0e-4);
    return min(vec3(r * r) * specularColor, vec3(1.0)) * specularFactor;
}

// What a dielectric reflectance leaves to the diffuse lobe under it: one minus its largest channel,
// so a colored specular dims the diffuse evenly (KHR_materials_specular). The reflectance already
// carries the specular factor, through the dielectric f0 and f90, so the factor counts once
float bsdfDiffuseWeight(vec3 reflectance)
{
    return 1.0 - max(max(reflectance.r, reflectance.g), reflectance.b);
}

// The glTF material mixes a dielectric BRDF (diffuse under a specular layer) and a metal BRDF
// (specular only) by metallic. The specular of both is one lobe with the mixed f0 and f90, while
// the diffuse is dimmed by the dielectric layer alone: partially metallic materials keep more
// diffuse than a weight from the mixed reflectance would leave them
struct BsdfSurface
{
    vec3 n;              // unit shading normal
    vec3 v;              // unit direction towards the viewer
    vec3 diffuseColor;   // Lambert albedo, the base color times 1 - metallic
    vec3 f0;             // reflectance at normal incidence, dielectric and metal mixed by metallic
    vec3 f90;            // reflectance at grazing incidence, mixed the same way (1 for metals)
    vec3 dielectricF0;   // reflectance at normal incidence of the dielectric alone, from bsdfDielectricF0
    float dielectricF90; // reflectance at grazing incidence of the dielectric alone, the specular factor
    float alpha2;        // GGX alpha squared, usually from bsdfFilteredAlpha2
    float nDotV;         // dot(n, v), kept above zero
};

// Reflected radiance for a unit light direction l and unit incoming radiance: the diffuse and
// specular lobes times nDotL, 0 when the light is behind the surface
vec3 bsdfEvaluate(BsdfSurface s, vec3 l)
{
    float nDotL = dot(s.n, l);
    if (nDotL <= 0.0)
        return vec3(0.0);

    vec3 h = normalize(s.v + l);
    float nDotH = max(dot(s.n, h), 0.0);
    float vDotH = max(dot(s.v, h), 0.0);

    float distribution = bsdfDistributionGgx(nDotH, s.alpha2);
    float visibility = bsdfVisibilitySmithGgxCorrelated(s.nDotV, nDotL, s.alpha2);
    vec3 fresnel = bsdfFresnelSchlick(s.f0, s.f90, vDotH);
    vec3 dielectricFresnel = bsdfFresnelSchlick(s.dielectricF0, vec3(s.dielectricF90), vDotH);

    vec3 diffuse = bsdfDiffuseWeight(dielectricFresnel) * s.diffuseColor / yupPi;
    vec3 specular = fresnel * (distribution * visibility);

    return (diffuse + specular) * nDotL;
}

// The diffuse weight under environment light, from the split-sum scale and bias of
// bsdfEnvironmentBrdf applied to the dielectric reflectance, as bsdfEvaluate does per light
float bsdfEnvironmentDiffuseWeight(BsdfSurface s, vec2 brdf)
{
    return bsdfDiffuseWeight(s.dielectricF0 * brdf.x + s.dielectricF90 * brdf.y);
}

#endif
