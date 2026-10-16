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
#include "lib/cube.glsl"
#include "lib/octahedral.glsl"
#include "lib/sampling.glsl"

// Filters a radiance cube for the GGX reflections of one perceptual roughness, into one face of a
// cube mip or into one mip of an octahedral atlas tile. The pass is drawn with the viewport on the
// face or the tile, so uv runs across it.

layout(set = 0, binding = 1) uniform sampler linearClamp;

layout(set = 1, binding = 0) uniform ProbePrefilterBlock
{
    vec4 params; // x output (0 a cube face, 1 an octahedral tile), y 1 when the textures hold RGBM, z cube face, w perceptual roughness
    vec4 sizes;  // x size of the face or tile written, in texels; y source face size at mip 0; z highest source mip; w samples, 1 for a copy
} probe;

layout(set = 1, binding = 1) uniform textureCube u_source;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

// One mip of an RGBM source, filtered bilinearly after decoding: the sampler is nearest then, so
// each tap is one texel. Taps past a face edge point into the neighboring face
vec3 sourceRgbmBilinear(vec3 direction, float mip)
{
    int face;
    vec2 st = cubeFaceCoordinates(direction, face);
    float size = max(probe.sizes.y / exp2(mip), 1.0);
    vec2 p = st * size - 0.5;
    vec2 base = floor(p);
    vec2 f = p - base;

    vec3 sum = vec3(0.0);

    for (int j = 0; j < 2; ++j)
    {
        for (int i = 0; i < 2; ++i)
        {
            vec2 tap = (base + vec2(float(i), float(j)) + 0.5) / size;
            float weight = (i == 0 ? 1.0 - f.x : f.x) * (j == 0 ? 1.0 - f.y : f.y);
            sum += yupDecodeRgbm(textureLod(samplerCube(u_source, linearClamp), cubeDirection(face, tap), mip), 1.0) * weight;
        }
    }

    return sum;
}

// The source radiance around a direction at a fractional lod. RGBM texels are never filtered
// encoded, which would mix their multipliers: they are decoded, then filtered trilinearly here
vec3 sourceRadiance(vec3 direction, float lod)
{
    lod = clamp(lod, 0.0, probe.sizes.z);

    if (probe.params.y < 0.5)
        return yupDecodeRgbm(textureLod(samplerCube(u_source, linearClamp), direction, lod), probe.params.y);

    float mip = floor(lod);
    return mix(sourceRgbmBilinear(direction, mip), sourceRgbmBilinear(direction, min(mip + 1.0, probe.sizes.z)), lod - mip);
}

// Split-sum prefilter with n = v = r (Karis 2013, "Real Shading in Unreal Engine 4"): GGX half
// vectors importance sampled around the texel direction, each reflected sample weighted by n.l.
// Each sample reads the source mip whose texels cover the solid angle its pdf gives it, plus one
// level, so few samples still integrate the whole lobe (Colbert and Krivanek 2007, "GPU-Based
// Importance Sampling", GPU Gems 3)
void main()
{
    float size = probe.sizes.x;
    vec3 n;
    float texelSolidAngle;

    if (probe.params.x < 0.5)
    {
        // The average texel, as for the source: a mip 0 of the source's size is a copy at lod 0
        n = cubeDirection(int(probe.params.z), uv);
        texelSolidAngle = 4.0 * yupPi / (6.0 * size * size);
    }
    else
    {
        // The texel centers span the octahedron edge to edge, as octahedralTileUv reads them
        n = octahedralDecode((uv * size - 0.5) / (size - 1.0) * 2.0 - 1.0);
        texelSolidAngle = 4.0 * yupPi / ((size - 1.0) * (size - 1.0));
    }

    float sourceTexel = 4.0 * yupPi / (6.0 * probe.sizes.y * probe.sizes.y);
    int numSamples = int(probe.sizes.w);

    if (numSamples <= 1)
    {
        fragColor = yupEncodeRadiance(sourceRadiance(n, 0.5 * log2(texelSolidAngle / sourceTexel)), probe.params.y);
        return;
    }

    float alpha = max(probe.params.w * probe.params.w, 1.0e-3);
    float alpha2 = alpha * alpha;
    mat3 frame = yupTangentFrame(n);

    vec3 sum = vec3(0.0);
    float total = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        vec3 h = frame * yupSampleGgxVndf(vec3(0.0, 0.0, 1.0), alpha, yupHammersley(i, numSamples));
        float nDotH = dot(n, h);
        vec3 l = 2.0 * nDotH * h - n;
        float nDotL = dot(n, l);

        if (nDotL > 0.0)
        {
            // With v = n the pdf of l is D (h) h.z / (4 v.h) = D (h) / 4
            float pdf = bsdfDistributionGgx(nDotH, alpha2) * 0.25;
            float sampleSolidAngle = 1.0 / (float(numSamples) * pdf + 1.0e-6);

            sum += sourceRadiance(l, 0.5 * log2(sampleSolidAngle / sourceTexel) + 1.0) * nDotL;
            total += nDotL;
        }
    }

    fragColor = yupEncodeRadiance(total > 0.0 ? sum / total : vec3(0.0), probe.params.y);
}
