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


#ifndef YUP_3D_LIB_SAMPLING
#define YUP_3D_LIB_SAMPLING

#include "lib/bsdf.glsl"

// Radical inverse of index in the given base: the low-discrepancy Halton sequence (Halton 1964).
// Index 0 gives 0, so sequences usually start from 1.
float yupHalton(int index, int base)
{
    float result = 0.0;
    float fraction = 1.0;

    for (int i = index; i > 0; i /= base)
    {
        fraction /= float(base);
        result += fraction * float(i % base);
    }

    return result;
}

// Per-pixel noise in 0..1 that dithers well over small neighborhoods (Jimenez 2014, "Next Generation
// Post Processing in Call of Duty: Advanced Warfare")
float yupInterleavedGradientNoise(vec2 pixel)
{
    return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

// Point i of an n point Hammersley set in [0, 1)^2: i / n against the base 2 radical inverse of i,
// with the bit reversal written out so it needs no bitfieldReverse (Colbert and Krivanek 2007,
// "GPU-Based Importance Sampling", GPU Gems 3)
vec2 yupHammersley(int i, int n)
{
    uint bits = uint(i);
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return vec2(float(i) / float(n), float(bits) * 2.3283064365386963e-10);
}

// An orthonormal frame around a unit normal: columns tangent, bitangent and the normal, mapping
// tangent space (z along the normal) to world space
mat3 yupTangentFrame(vec3 n)
{
    vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 t = normalize(cross(up, n));
    return mat3(t, cross(n, t), n);
}

// A GGX microfacet normal, in tangent space, sampled from the distribution of the normals visible
// from the unit view direction @p v (tangent space, z up) for a roughness alpha (Heitz 2018,
// "Sampling the GGX Distribution of Visible Normals", JCGT 7(4)). Its pdf is
// D (h) G1 (v) max (0, v.h) / v.z, which is D (h) h.z when v is the normal.
vec3 yupSampleGgxVndf(vec3 v, float alpha, vec2 u)
{
    vec3 vh = normalize(vec3(alpha * v.x, alpha * v.y, v.z));
    float lengthSquared = vh.x * vh.x + vh.y * vh.y;
    vec3 t1 = lengthSquared > 0.0 ? vec3(-vh.y, vh.x, 0.0) * inversesqrt(lengthSquared) : vec3(1.0, 0.0, 0.0);
    vec3 t2 = cross(vh, t1);

    float r = sqrt(u.x);
    float phi = 2.0 * yupPi * u.y;
    float p1 = r * cos(phi);
    float s = 0.5 * (1.0 + vh.z);
    float p2 = mix(sqrt(max(0.0, 1.0 - p1 * p1)), r * sin(phi), s);

    vec3 nh = p1 * t1 + p2 * t2 + sqrt(max(0.0, 1.0 - p1 * p1 - p2 * p2)) * vh;
    return normalize(vec3(alpha * nh.x, alpha * nh.y, max(1.0e-6, nh.z)));
}

#endif
