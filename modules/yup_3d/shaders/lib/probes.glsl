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
