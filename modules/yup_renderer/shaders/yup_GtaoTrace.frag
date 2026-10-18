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
// The screen-space diffuse GI variant (YUP_3D_SCREEN_SPACE_GI, full tier only) also gathers the
// light of the occluded arcs along the same horizon search, see gtaoHorizon().

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

#ifdef YUP_3D_SCREEN_SPACE_GI
// The previous frame's radiance where each sample was: the TemporalAA history (exposed and
// premultiplied by its coverage), reprojected by the G-buffer velocity
layout(set = 0, binding = 1) uniform sampler linearClamp;
layout(set = 1, binding = 4) uniform texture2D u_velocity;
layout(set = 1, binding = 5) uniform texture2D u_previousSceneColor;

// The gathered irradiance in rgb, the share of the cosine-weighted hemisphere it came from in a
layout(location = 1) out vec4 fragIndirect;
#endif

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

#ifdef YUP_3D_SCREEN_SPACE_GI
// The cosine-weighted integral of a slice from the view vector to the slice angle h: the term of
// Jimenez et al. 2016 the visibility adds up on each side
float gtaoArc(float h, float normalAngle, float cosNormal, float sinNormal)
{
    return 0.25 * (-cos(2.0 * h - normalAngle) + cosNormal + 2.0 * h * sinNormal);
}
#endif

// The highest horizon cosine towards side (+1 or -1) of a screen direction, -1 without any.
// The screen-space diffuse GI variant also gathers the light of the arcs the search occludes, as
// Horizon-Based Indirect Lighting does (Mayaux 2018, "Horizon-Based Indirect Lighting"): each step
// that raises the horizon occludes the arc between the old and the new horizon, clamped to the
// hemisphere of the projected normal, and lights it with the radiance at the sample, weighted by
// that arc's share of the visibility integral, so the weights of all the steps add up to the
// occluded share. A sample whose surface faces away from the pixel, or that the previous frame did
// not see, still occludes but lights nothing (HBIL fades such a sample to the radiance of the
// previous one instead). Unlike HBIL, the radiance is the previous frame's TemporalAA history, all of
// its exposed light, rather than a reprojected diffuse-only buffer. Adds the radiance times the
// weight to rgb, and the weight that found radiance to a, of @p gathered
#ifdef YUP_3D_SCREEN_SPACE_GI
float gtaoHorizon(ivec2 pixel, vec3 position, vec3 v, vec2 direction, float side, float radius, float jitter,
                  float normalAngle, float cosNormal, float sinNormal, inout vec4 gathered)
#else
float gtaoHorizon(ivec2 pixel, vec3 position, vec3 v, vec2 direction, float side, float radius, float jitter)
#endif
{
    float reach = gtao.params.x;
    int steps = int(gtao.params.z);
    float horizon = -1.0;
#ifdef YUP_3D_SCREEN_SPACE_GI
    float limit = normalAngle + side * 0.5 * yupPi;
    float arc = gtaoArc(limit, normalAngle, cosNormal, sinNormal);
#endif

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
#ifdef YUP_3D_SCREEN_SPACE_GI
        float candidate = mix(-1.0, dot(delta, v) / deltaLength, falloff);
        if (candidate <= horizon)
            continue;

        horizon = candidate;
        float horizonAngle = acos(clamp(horizon, -1.0, 1.0));
        float newArc = gtaoArc(side > 0.0 ? min(horizonAngle, limit) : max(-horizonAngle, limit), normalAngle, cosNormal, sinNormal);
        float weight = arc - newArc;
        arc = newArc;

        // delta points from the pixel to the sample, whose surface lights the pixel only if it faces back
        if (weight <= 0.0 || dot(gtaoViewNormal(samplePixel), delta) >= 0.0)
            continue;

        vec2 previousUv = (vec2(samplePixel) + 0.5) * view.viewport.zw - texelFetch(sampler2D(u_velocity, pointClamp), samplePixel, 0).xy;
        if (any(lessThan(previousUv, vec2(0.0))) || any(greaterThan(previousUv, vec2(1.0))))
            continue;

        vec4 previous = textureLod(sampler2D(u_previousSceneColor, linearClamp), previousUv, 0.0);
        if (previous.a <= 1.0e-3)
            continue;

        // gtao.noise.z: 1 / the exposure the history was rendered with
        gathered += vec4(min(previous.rgb / previous.a * gtao.noise.z, vec3(65504.0)), 1.0) * weight;
#else
        horizon = max(horizon, mix(-1.0, dot(delta, v) / deltaLength, falloff));
#endif
    }

    return clamp(horizon, -1.0, 1.0);
}

void main()
{
    ivec2 outputPixel = ivec2(gl_FragCoord.xy);
    ivec2 pixel = gtaoInputPixel(outputPixel);
    float depth = gtaoDepth(pixel);
#ifdef YUP_3D_SCREEN_SPACE_GI
    fragIndirect = vec4(0.0);
#endif

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
#ifdef YUP_3D_SCREEN_SPACE_GI
    vec4 indirect = vec4(0.0);
#endif

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

#ifdef YUP_3D_SCREEN_SPACE_GI
        vec4 gathered = vec4(0.0);
        float h0 = normalAngle + max(-acos(gtaoHorizon(pixel, position, v, direction, -1.0, radius, noise.y, normalAngle, cosNormal, sinNormal, gathered)) - normalAngle, -0.5 * yupPi);
        float h1 = normalAngle + min(acos(gtaoHorizon(pixel, position, v, direction, 1.0, radius, noise.y, normalAngle, cosNormal, sinNormal, gathered)) - normalAngle, 0.5 * yupPi);
        indirect += gathered * projectedLength;
#else
        float h0 = normalAngle + max(-acos(gtaoHorizon(pixel, position, v, direction, -1.0, radius, noise.y)) - normalAngle, -0.5 * yupPi);
        float h1 = normalAngle + min(acos(gtaoHorizon(pixel, position, v, direction, 1.0, radius, noise.y)) - normalAngle, 0.5 * yupPi);
#endif

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
#ifdef YUP_3D_SCREEN_SPACE_GI
    indirect /= float(max(slices, 1));
    fragIndirect = vec4(min(indirect.rgb, vec3(65504.0)), indirect.a);
#endif
}
