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
#include "lib/gtao.glsl"

// Ground-truth ambient occlusion (Jimenez et al. 2016, "Practical Real-Time Strategies for Accurate
// Indirect Occlusion"). Each slice is the plane through the view vector and a screen direction: the
// highest horizon on each side is marched over the screen footprint of the occlusion distance,
// samples fading out over its last 40%, then clamped to the hemisphere of the normal projected on
// the slice, and the cosine-weighted visible arc between them is integrated analytically. So is the
// bent normal, the cosine-weighted mean direction of the visible arcs: a mean of each slice's
// mid-horizon direction would lean towards the view vector, since only the slices along the
// normal see its part across the view.
// The slices turn per pixel: on the full tier by interleaved gradient noise (Jimenez 2014), which
// also turns with the frame under temporal anti-aliasing, on raster by a fixed 4x4 pattern that the
// separable denoise averages out. The screen footprint is capped at a tenth of the view height.
// Output: the view-space bent normal * 0.5 + 0.5 in rgb, the visibility in a.

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

// The 4x4 ordered dither (Bayer) matrix at a pixel, in [0, 1): every value once per 4x4 block
float gtaoPattern(ivec2 pixel)
{
    ivec2 low = pixel & 1;
    ivec2 high = (pixel >> 1) & 1;
    int value = 4 * (2 * (low.x ^ low.y) + low.y) + 2 * (high.x ^ high.y) + high.y;
    return float(value) / 16.0;
}

// The integrals over a slice angle t of cos (t) cos (t - n) sin (t) and of sin^2 (t) cos (t - n), the
// view and tangent parts of the bent normal before the |sin t| of the side is applied
vec2 gtaoBentIntegrals(float t, float cosNormal, float sinNormal)
{
    float c = cos(t);
    float s = sin(t);
    return vec2(sinNormal * s * s * s - cosNormal * c * c * c,
                cosNormal * s * s * s + sinNormal * (c * c * c - 3.0 * c)) / 3.0;
}

// The highest horizon cosine towards side (+1 or -1) of a screen direction, -1 without any
float gtaoHorizon(ivec2 pixel, vec3 position, vec3 v, vec2 direction, float side, float radius, float jitter)
{
    float reach = gtao.params.x;
    int steps = int(gtao.params.z);
    float horizon = -1.0;

    for (int i = 0; i < steps; ++i)
    {
        // Denser near the pixel, at least a texel apart
        float t = (float(i) + jitter) / float(steps);
        float stepOffset = max(radius * t * t, float(i + 1));
        ivec2 samplePixel = ivec2(floor(vec2(pixel) + direction * (side * stepOffset) + 0.5));

        if (any(lessThan(samplePixel, ivec2(0))) || any(greaterThanEqual(samplePixel, ivec2(view.viewport.xy))))
            break;

        float sampleDepth = gtaoDepth(samplePixel);
        if (sampleDepth >= 1.0)
            continue;

        vec3 delta = gtaoViewPosition(samplePixel, sampleDepth) - position;
        float deltaLength = length(delta);
        if (deltaLength < 1.0e-6)
            continue;

        float falloff = clamp((reach - deltaLength) / (0.4 * reach), 0.0, 1.0);
        horizon = max(horizon, mix(-1.0, dot(delta, v) / deltaLength, falloff));
    }

    return clamp(horizon, -1.0, 1.0);
}

void main()
{
    ivec2 outputPixel = ivec2(gl_FragCoord.xy);
    ivec2 pixel = gtaoInputPixel(outputPixel);
    float depth = gtaoDepth(pixel);

    // Nothing drawn: unoccluded, facing the camera
    if (depth >= 1.0)
    {
        fragColor = vec4(0.5, 0.5, 1.0, 1.0);
        return;
    }

    bool perspective = view.projection[2][3] < -0.5;
    vec3 position = gtaoViewPosition(pixel, depth);
    vec3 v = perspective ? normalize(-position) : vec3(0.0, 0.0, 1.0);
    vec3 n = gtaoViewNormal(pixel);

    // The occlusion distance on screen, in input texels
    float pixelsPerUnit = 0.5 * view.viewport.y * abs(view.projection[1][1]) / (perspective ? max(-position.z, 1.0e-6) : 1.0);
    // At most a tenth of the view height: near the camera the distance would span the screen, and
    // its scattered depth reads would dominate the cost
    float radius = min(gtao.params.x * pixelsPerUnit, 0.1 * view.viewport.y);

    if (radius < 1.0)
    {
        fragColor = vec4(n * 0.5 + 0.5, 1.0);
        return;
    }

    vec2 noise;
    if (gtao.noise.x > 0.5)
    {
        noise = vec2(gtaoPattern(outputPixel), gtaoPattern(outputPixel.yx));
    }
    else
    {
        float k = gtao.noise.y > 0.5 ? mod(view.clipPlanes.z, 64.0) : 0.0;
        noise = vec2(yupInterleavedGradientNoise(gl_FragCoord.xy + 5.588238 * k),
                     yupInterleavedGradientNoise(gl_FragCoord.yx + 5.588238 * k));
    }

    int slices = int(gtao.params.y);
    float visibility = 0.0;
    vec3 bentNormal = vec3(0.0);

    for (int slice = 0; slice < slices; ++slice)
    {
        float phi = (float(slice) + noise.x) * yupPi / float(slices);
        vec2 direction = vec2(cos(phi), sin(phi)); // in texels, y down
        vec3 directionView = vec3(direction.x, -direction.y, 0.0);

        // The slice plane: v and the direction, with tangent towards the direction
        vec3 tangent = normalize(directionView - v * dot(directionView, v));
        vec3 axis = cross(tangent, v);
        vec3 projectedNormal = n - axis * dot(n, axis);
        float projectedLength = length(projectedNormal);
        if (projectedLength < 1.0e-4)
            continue;

        float cosNormal = clamp(dot(projectedNormal, v) / projectedLength, -1.0, 1.0);
        float normalAngle = (dot(projectedNormal, tangent) < 0.0 ? -1.0 : 1.0) * acos(cosNormal);
        float sinNormal = sin(normalAngle);

        float h0 = normalAngle + max(-acos(gtaoHorizon(pixel, position, v, direction, -1.0, radius, noise.y)) - normalAngle, -0.5 * yupPi);
        float h1 = normalAngle + min(acos(gtaoHorizon(pixel, position, v, direction, 1.0, radius, noise.y)) - normalAngle, 0.5 * yupPi);

        float arc = 0.25 * (-cos(2.0 * h0 - normalAngle) + cosNormal + 2.0 * h0 * sinNormal)
                  + 0.25 * (-cos(2.0 * h1 - normalAngle) + cosNormal + 2.0 * h1 * sinNormal);
        float weight = projectedLength * max(arc, 0.0);

        // Over [h0, 0] the |sin t| of the measure is -sin t, over [0, h1] it is sin t
        vec2 bent = gtaoBentIntegrals(h1, cosNormal, sinNormal) + gtaoBentIntegrals(h0, cosNormal, sinNormal)
                  - 2.0 * gtaoBentIntegrals(0.0, cosNormal, sinNormal);

        visibility += weight;
        bentNormal += (v * bent.x + tangent * bent.y) * projectedLength;
    }

    visibility = clamp(visibility / float(max(slices, 1)), 0.0, 1.0);
    float bentLength = length(bentNormal);
    bentNormal = bentLength > 1.0e-4 ? bentNormal / bentLength : n;

    fragColor = vec4(bentNormal * 0.5 + 0.5, visibility);
}
