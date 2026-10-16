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

#include "lib/sampling.glsl"
#include "lib/ssr.glsl"

// The trace of the screen-space reflections (ReflectionPass), at half resolution: texel i reflects
// the G-buffer texel 2i. One ray per texel, sampled from the GGX normals visible from the camera and
// turned by interleaved gradient noise that changes every frame, for the running mean of temporal
// anti-aliasing to accumulate (the pass only runs with it). The ray marches the Hi-Z pyramid of the
// closest depth. Output: the hit uv, the stored depth of the surface hit and the pdf of the ray over
// the reflected directions, or 0 for a miss.

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

// A view-space position in pixels of mip 0 and stored depth
vec3 ssrScreenPosition(vec3 position)
{
    vec4 clip = view.projection * vec4(position, 1.0);
    vec3 ndc = clip.xyz / clip.w;
    return vec3((ndc.x * 0.5 + 0.5) * view.viewport.x, (0.5 - ndc.y * 0.5) * view.viewport.y, ndc.z);
}

// The t at which the ray origin + t delta leaves the cell of side size holding it, along x or y,
// whichever comes first; side is 1 where delta grows, 0 where it shrinks
float ssrCellExit(vec3 origin, vec2 inverseDelta, vec2 side, vec2 cell, float size)
{
    vec2 t = ((cell + side) * size - origin.xy) * inverseDelta;
    return min(t.x, t.y);
}

// Hi-Z ray marching (Uludag 2014, "Hi-Z Screen-Space Cone-Traced Reflections", GPU Pro 5), in pixels
// of mip 0 and stored depth, up to t = end: while the ray stays in front of the closest depth of its
// cell it skips to the next cell and climbs a level, otherwise it descends. At mip 0 a ray reaching
// the surface of its texel hits it, and a ray already behind it hits only within the thickness;
// otherwise it passes behind and goes on. A surface facing away from the view-space ray @p l is not
// what the ray hits, but the back of what the depth shows (or, on a grazing plane, the ray's own
// surface at the next texel center): the ray passes behind it too. Returns the t of the hit, or -1,
// with the stored depth of the surface hit in @p hitDepth
float ssrMarch(vec3 origin, vec3 delta, float end, vec3 l, out float hitDepth)
{
    vec2 inverseDelta = vec2(abs(delta.x) > 1.0e-6 ? 1.0 / delta.x : 1.0e30, abs(delta.y) > 1.0e-6 ? 1.0 / delta.y : 1.0e30);
    vec2 side = vec2(delta.x >= 0.0 ? 1.0 : 0.0, delta.y >= 0.0 ? 1.0 : 0.0);
    float nudge = 0.01 / max(abs(delta.x), abs(delta.y));
    int topLevel = int(ssr.frame.y) - 1;
    int iterations = int(ssr.params.w);

    // Out of the texel of the origin first, which holds its own surface
    float t = ssrCellExit(origin, inverseDelta, side, floor(origin.xy), 1.0) + nudge;
    int level = 0;
    hitDepth = 1.0;

    for (int i = 0; i < iterations && t < end; ++i)
    {
        vec3 p = origin + delta * t;
        float size = exp2(float(level));
        vec2 cell = floor(p.xy / size);
        // Clamped at both ends: a point a rounding error past the screen edge reads the edge texel
        ivec2 texel = clamp(ivec2(cell), ivec2(0), textureSize(sampler2D(u_hiZ, pointClamp), level) - 1);
        float closest = texelFetch(sampler2D(u_hiZ, pointClamp), texel, level).r;
        float exit = ssrCellExit(origin, inverseDelta, side, cell, size);

        if (p.z < closest)
        {
            // In front of every surface of the cell: where the ray reaches the closest of them
            float reach = delta.z > 0.0 ? (closest - origin.z) / delta.z : 1.0e30;
            if (reach >= exit)
            {
                t = exit + nudge;
                level = min(level + 1, topLevel);
            }
            else if (level > 0)
            {
                t = max(t, reach);
                --level;
            }
            else if (dot(ssrNormalRoughness(texel).xyz, l) < 0.0)
            {
                hitDepth = closest;
                return reach;
            }
            else
            {
                t = exit + nudge;
            }
        }
        else if (level > 0)
        {
            --level;
        }
        else
        {
            float surface = ssrViewDepth(closest);
            if (ssrViewDepth(p.z) - surface < ssr.params.z * surface && dot(ssrNormalRoughness(texel).xyz, l) < 0.0)
            {
                hitDepth = closest;
                return t;
            }

            t = exit + nudge;
        }
    }

    return -1.0;
}

void main()
{
    ivec2 pixel = min(ivec2(gl_FragCoord.xy) * 2, ivec2(view.viewport.xy) - 1);
    float depth = ssrDepth(pixel);
    vec4 normalRoughness = ssrNormalRoughness(pixel);
    fragColor = vec4(0.0);

    // Nothing drawn, or too rough: nothing to trace
    if (depth >= 1.0 || normalRoughness.w >= ssr.params.y)
        return;

    vec3 position = ssrViewPosition((vec2(pixel) + 0.5) * view.viewport.zw, depth);
    vec3 v = ssrViewVector(position);
    vec3 n = normalRoughness.xyz;
    float alpha = normalRoughness.w * normalRoughness.w;
    float alpha2 = alpha * alpha;

    mat3 frame = yupTangentFrame(n);
    vec3 tangentV = v * frame;
    if (tangentV.z <= 1.0e-4)
        return;

    float k = mod(view.clipPlanes.z, 64.0);
    vec2 noise = vec2(yupInterleavedGradientNoise(gl_FragCoord.xy + 5.588238 * k),
                      yupInterleavedGradientNoise(gl_FragCoord.yx + 5.588238 * k));

    // Only the core of the lobe, the half of the visible normals within tan (n.h) of about alpha
    // (Stachowiak 2015 biases its samples the same way): a ray in the long tail of GGX finds a bright
    // hit too rarely for the running mean to converge, and speckles the glossy reflections
    vec3 tangentH = yupSampleGgxVndf(tangentV, alpha, vec2(0.5 * noise.x, noise.y));
    vec3 l = reflect(-v, frame * tangentH);
    if (dot(n, l) <= 0.0)
        return;

    // Up to the far plane, or just short of the near one for rays turning towards the camera
    float far = view.clipPlanes.y > 0.0 ? view.clipPlanes.y : 1.0e4;
    float rayLength = l.z > 0.0 ? min(far, 0.99 * (-view.clipPlanes.x - position.z) / l.z) : far;

    vec3 origin = ssrScreenPosition(position);
    vec3 delta = ssrScreenPosition(position + l * rayLength) - origin;

    // Clipped to the screen and to the far plane
    float end = 1.0;
    if (delta.x != 0.0)
        end = min(end, ((delta.x > 0.0 ? view.viewport.x : 0.0) - origin.x) / delta.x);
    if (delta.y != 0.0)
        end = min(end, ((delta.y > 0.0 ? view.viewport.y : 0.0) - origin.y) / delta.y);
    if (delta.z > 0.0)
        end = min(end, (1.0 - origin.z) / delta.z);

    // A ray along the view direction stays within its pixel
    if (length(delta.xy) * end < 1.0)
        return;

    float hitDepth;
    float t = ssrMarch(origin, delta, end, l, hitDepth);
    if (t < 0.0)
        return;

    vec2 hitUv = (origin.xy + delta.xy * t) * view.viewport.zw;

    // The pdf of the visible normal sampling over the reflected directions (Heitz 2018): D (h) G1 (v) / (4 n.v)
    float nDotV = tangentV.z;
    float g1 = 2.0 * nDotV / (nDotV + sqrt(alpha2 + (1.0 - alpha2) * nDotV * nDotV));
    float pdf = bsdfDistributionGgx(tangentH.z, alpha2) * g1 / (4.0 * nDotV);

    fragColor = vec4(hitUv, hitDepth, pdf);
}
