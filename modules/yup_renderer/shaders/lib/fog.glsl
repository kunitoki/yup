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

#ifndef YUP_3D_LIB_FOG
#define YUP_3D_LIB_FOG

#include "lib/color.glsl"
#include "lib/view.glsl"

// The analytic world fog (WorldFog) of the FogApply pass, the only bundle including this file. The
// extinction at a height y is density * exp (-heightFalloff * (y - baseHeight)), its exponent clamped
// at 40 so it stays finite, lit by single scattering with no shadowing.

#define YUP_3D_FOG_MAX_EXPONENT 40.0

layout(set = 1, binding = 0) uniform FogBlock
{
    vec4 medium;       // x density (extinction per world unit at the base height), y heightFalloff, z baseHeight, w anisotropy (Henyey-Greenstein g)
    vec4 albedo;       // rgb the scattering albedo (WorldFog::color)
    vec4 ambient;      // rgb the ambient radiance: the environment's spherical harmonics L0 times its intensity, plus the ambient color
    vec4 sunRadiance;  // rgb the first directional light's color times its intensity, 0 without one
    vec4 sunDirection; // xyz the unit direction towards that light
    vec4 options;      // x 1 when the scene color is reversibly tone mapped (the raster mode), y 1 for an infinite far plane
} fog;

float fogExponent(float height)
{
    return -fog.medium.y * (height - fog.medium.z);
}

// The optical depth (integrated extinction) along a segment: the GLSL mirror of
// WorldFog::getOpticalDepth(), integrated from the denser endpoint so the exponential decays, the part
// past the clamp uniform. Without expm1, (1 - e^-drop) / drop takes its series below 0.01, where single
// precision cancellation would lose digits. The factors are capped so no product overflows
float fogOpticalDepth(vec3 from, vec3 to)
{
    float segmentLength = min(length(to - from), 1.0e12);
    float a = fogExponent(from.y);
    float b = fogExponent(to.y);
    float thin = min(a, b);
    float dense = max(a, b);

    if (thin >= YUP_3D_FOG_MAX_EXPONENT)
        return min(fog.medium.x * exp(YUP_3D_FOG_MAX_EXPONENT), 1.0e20) * segmentLength;

    float clampedFraction = dense > YUP_3D_FOG_MAX_EXPONENT ? (dense - YUP_3D_FOG_MAX_EXPONENT) / (dense - thin) : 0.0;
    float clampedDense = min(dense, YUP_3D_FOG_MAX_EXPONENT);
    float drop = clampedDense - thin;
    float decay = drop < 0.01 ? 1.0 - drop * (0.5 - drop / 6.0) : (1.0 - exp(-drop)) / drop;

    return min(fog.medium.x * exp(clampedDense), 1.0e20) * segmentLength * mix(decay, 1.0, clampedFraction);
}

// The optical depth from @p origin along the unit @p direction to infinity, the limit of
// fogOpticalDepth() as the segment grows: finite only while the exponent drops along the ray, at the
// rate k = heightFalloff * direction.y > 0 (climbing out of fog thinning upwards, or descending out of
// fog thinning downwards), past a uniform stretch while the clamp holds. -1 for rays that never leave
// the fog (uniform fog, horizontal rays, rays into denser fog), whose transmittance is 0: the limit is
// handled here, so no infinity is ever formed
float fogOpticalDepthToInfinity(vec3 origin, vec3 direction)
{
    float k = fog.medium.y * direction.y;
    if (k <= 0.0)
        return -1.0;

    float exponent = fogExponent(origin.y);
    float inverseRate = 1.0 / max(k, 1.0e-12);
    float pathLength = min((max(exponent - YUP_3D_FOG_MAX_EXPONENT, 0.0) + 1.0) * inverseRate, 1.0e12);

    return min(fog.medium.x * exp(min(exponent, YUP_3D_FOG_MAX_EXPONENT)), 1.0e20) * pathLength;
}

// The Henyey-Greenstein phase function (Henyey and Greenstein 1941, "Diffuse radiation in the
// galaxy"), normalized over the sphere, so isotropic fog (g = 0) gives 1 / (4 pi): @p cosTheta is the
// cosine between the view direction and the direction towards the light, so g > 0 scatters forward,
// brightest looking at the light
float hgPhase(float cosTheta, float g)
{
    float denominator = 1.0 + g * g - 2.0 * g * cosTheta;
    return (1.0 - g * g) / (4.0 * 3.14159265 * denominator * sqrt(denominator));
}

// The fog in front of a point at @p opticalDepth (negative: infinitely far in fog it never leaves),
// seen along the unit @p direction: rgb the exposed in-scattered radiance S, already multiplied by
// 1 - T, and a the transmittance T. Single scattering with no shadowing: the ambient radiance and the
// sun reach every point of the fog, behind objects too. The sun's color times its intensity is the
// irradiance it brings, scattered by the phase; the ambient radiance comes from every direction, over
// which the phase integrates to 1 (preflight R25). The radiance is bounded before the coverage scales
// it, so dst * T + S stays within the half float range too
vec4 heightFog(float opticalDepth, vec3 direction)
{
    float transmittance = opticalDepth < 0.0 ? 0.0 : exp(-min(opticalDepth, 1.0e4));
    vec3 light = fog.ambient.rgb + fog.sunRadiance.rgb * hgPhase(dot(direction, fog.sunDirection.xyz), fog.medium.w);
    vec3 scattered = (1.0 - transmittance) * min(fog.albedo.rgb * light * view.cameraPosition.w, vec3(yupMaxHalf));
    return vec4(scattered, transmittance);
}

#endif
