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

// The single-scattering specular reflectance under environment light, the split sum (Karis 2013,
// "Real Shading in Unreal Engine 4"), from the GGX lobe magnitude x and its Fresnel part y of the
// lookup tables: f0 scales x - y, and f90 the bias y
vec3 bsdfSplitSum(vec3 f0, vec3 f90, vec2 lobe)
{
    return f0 * (lobe.x - lobe.y) + f90 * lobe.y;
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

#ifdef YUP_3D_MATERIAL_LAYERS
// The clearcoat (KHR_materials_clearcoat) and sheen (KHR_materials_sheen) layers over the glTF
// BSDF, compiled only into the shaders defining YUP_3D_MATERIAL_LAYERS: the local indirect layers
// variants (YUP_3D_LAYERS) and the full tier's forward pass. A layer dims what lies under it by what it reflects:
// the clearcoat by 1 - clearcoat Fc (n.v), Fc its Schlick Fresnel with f0 = 0.04, and the sheen by
// the Khronos albedo scaling from its directional albedo E (lib/lights.glsl). A material without
// layers skips them by a branch, which leaves the base exactly as it is
struct BsdfLayers
{
    float clearcoat;          // the clearcoat factor, 0 for none
    vec3 clearcoatNormal;     // unit: the interpolated normal on raster, the shading normal on full
    float clearcoatRoughness; // perceptual, from 0.04 to 1
    float clearcoatAlpha2;    // GGX alpha squared of the clearcoat
    float clearcoatNDotV;     // dot(clearcoatNormal, v), kept above zero
    float clearcoatBase;      // 1 - clearcoat Fc (n.v), what the clearcoat leaves to the layers under it: 1 without it
    vec2 clearcoatUv;         // the coordinates of the clearcoat in the lookup tables
    vec2 clearcoatLobe;       // its layer 1 there: the GGX lobe magnitude and its Fresnel part
    vec3 sheenColor;          // linear, black for none
    float sheenMax;           // the largest channel of sheenColor, 0 for none
    float sheenRoughness;     // perceptual, from LookupTables::minSheenRoughness (5 / 63) to 1
    float sheenAlbedo;        // the directional albedo E (n.v) of a white sheen, 0 for none
};

// The layers of a material without clearcoat and sheen
BsdfLayers bsdfNoLayers()
{
    return BsdfLayers(0.0, vec3(0.0, 0.0, 1.0), 1.0, 1.0, 1.0, 1.0, vec2(0.0), vec2(0.0), vec3(0.0), 0.0, 1.0, 0.0);
}
#endif

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

// The surface of a glTF material, shared by the forward and the deferred shading. specular is the
// KHR_materials_specular color (rgb) and factor (w), which scales the whole dielectric reflectance,
// grazing incidence included
BsdfSurface bsdfMaterialSurface(vec3 n, vec3 v, vec3 baseColor, float metallic, float ior, vec4 specular, float alpha2)
{
    vec3 dielectricF0 = bsdfDielectricF0(ior, specular.rgb, specular.w);
    float dielectricF90 = specular.w;
    vec3 f0 = mix(dielectricF0, baseColor, metallic);
    vec3 f90 = mix(vec3(dielectricF90), vec3(1.0), metallic);
    vec3 diffuseColor = baseColor * (1.0 - metallic);
    float nDotV = max(dot(n, v), 0.0001);

    return BsdfSurface(n, v, diffuseColor, f0, f90, dielectricF0, dielectricF90, alpha2, nDotV);
}

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

// The diffuse weight under environment light, from the split sum of the dielectric reflectance,
// as bsdfEvaluate does per light. lobe is the GGX lobe magnitude and its Fresnel part (bsdfSplitSum)
float bsdfEnvironmentDiffuseWeight(BsdfSurface s, vec2 lobe)
{
    return bsdfDiffuseWeight(bsdfSplitSum(s.dielectricF0, vec3(s.dielectricF90), lobe));
}

// Multiple-scattering energy compensation of the split sum (Fdez-Aguera 2019, "A Multiple-Scattering
// Microfacet Model for Real-Time Image-based Lighting", JCGT 8 (1)). The single-scattering lobe
// reflects Ess = lobe.x of a white environment, its magnitude without Fresnel, and misses
// Ems = 1 - Ess. That energy bounces again, attenuated by the cosine-weighted average of the Schlick
// Fresnel, Favg = f0 + (f90 - f0) / 21, and leaves uniformly diffused: Fms Ems, returned here, weights
// the environment irradiance, as a diffuse albedo does. singleScattering is FssEss, from bsdfSplitSum
vec3 bsdfMultipleScattering(BsdfSurface s, vec2 lobe, vec3 singleScattering)
{
    float missing = 1.0 - lobe.x;
    vec3 averageFresnel = s.f0 + (s.f90 - s.f0) / 21.0;
    return singleScattering * averageFresnel * missing / max(1.0 - averageFresnel * missing, vec3(1.0e-4));
}

#ifdef YUP_3D_MATERIAL_LAYERS
// The reflectance of the clearcoat at normal incidence, an index of refraction of 1.5
const float bsdfClearcoatF0 = 0.04;

// Schlick's Fresnel of the clearcoat, from 0.04 to 1
float bsdfClearcoatFresnel(float cosine)
{
    return bsdfClearcoatF0 + (1.0 - bsdfClearcoatF0) * pow(1.0 - cosine, 5.0);
}

// The Charlie sheen distribution (Estevez and Kulla 2017, "Production Friendly Microfacet Sheen
// BRDF"), (2 + 1 / alpha) sin^(1 / alpha) (theta_h) / (2 pi), of alpha the sheen roughness squared
float bsdfDistributionCharlie(float nDotH, float alpha)
{
    float inverseAlpha = 1.0 / alpha;
    float sin2 = max(1.0 - nDotH * nDotH, 0.0);
    return (2.0 + inverseAlpha) * pow(sin2, inverseAlpha * 0.5) / (2.0 * yupPi);
}

// The sheen visibility of Neubelt and Pettineo 2013 ("Crafting a Next-Gen Material Pipeline for
// The Order: 1886"), which Filament calls Ashikhmin's. Not energy conserving at grazing angles,
// where LookupTables limits the sheen albedo to 1
float bsdfVisibilityNeubelt(float nDotV, float nDotL)
{
    return 1.0 / (4.0 * (nDotL + nDotV - nDotL * nDotV));
}

// The sheen BRDF towards the unit light direction l, sheenColor D V, for n.l = @p nDotL above zero
vec3 bsdfEvaluateSheen(BsdfSurface s, BsdfLayers layers, vec3 l, float nDotL)
{
    float nDotH = max(dot(s.n, normalize(s.v + l)), 0.0);
    float alpha = layers.sheenRoughness * layers.sheenRoughness;
    return layers.sheenColor * (bsdfDistributionCharlie(nDotH, alpha) * bsdfVisibilityNeubelt(s.nDotV, nDotL));
}

// The clearcoat lobe for a unit light direction l and unit incoming radiance, times its factor and
// the cosine with its normal: GGX with height-correlated Smith visibility and the Schlick Fresnel
// of bsdfClearcoatFresnel, 0 when the light is behind the clearcoat
float bsdfEvaluateClearcoat(BsdfSurface s, BsdfLayers layers, vec3 l)
{
    float nDotL = dot(layers.clearcoatNormal, l);
    if (nDotL <= 0.0)
        return 0.0;

    vec3 h = normalize(s.v + l);
    float nDotH = max(dot(layers.clearcoatNormal, h), 0.0);
    float vDotH = max(dot(s.v, h), 0.0);

    float distribution = bsdfDistributionGgx(nDotH, layers.clearcoatAlpha2);
    float visibility = bsdfVisibilitySmithGgxCorrelated(layers.clearcoatNDotV, nDotL, layers.clearcoatAlpha2);
    return layers.clearcoat * bsdfClearcoatFresnel(vDotH) * distribution * visibility * nDotL;
}
#endif

#endif
