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

#include "lib/motion.glsl"
#include "lib/sampling.glsl"

// The last motion blur pass, at the size of the scene color: YUP_3D_MOTION_SAMPLES taps along the
// longest motion of the tile's neighborhood, each weighted by which of the two surfaces is in
// front and whether its motion reaches the other. All four channels are averaged, so a moving
// silhouette smears its coverage over the background. The share of the coverage that came from
// the taps, or that the depth of field already spread at this pixel, goes to the second target:
// tone mapping composites that share in radiance and the coverage the pixel kept sharp, such as a
// still antialiased edge, in display space (preflight R21, R22). Pixels without coverage count as
// spread: both composites agree there.

layout(set = 1, binding = 1) uniform texture2D u_color;
layout(set = 1, binding = 2) uniform texture2D u_depth;
layout(set = 1, binding = 3) uniform texture2D u_velocity;
layout(set = 1, binding = 4) uniform texture2D u_neighborMax;
layout(set = 1, binding = 5) uniform texture2D u_blurAmount;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out float blurAmount;

// The view distance at @p position, through its inverse as dofCircleOfConfusion does, so the
// background at depth 1 with an infinite far plane is far but finite
float sceneDistance(vec2 position, ivec2 depthSize)
{
    float depth = texelFetch(sampler2D(u_depth, pointClamp), motionTexel(position, depthSize), 0).r;
    mat4 p = view.projection;
    return 1.0 / max((depth * p[2][3] - p[2][2]) / (depth * p[3][3] - p[3][2]), 1.0e-6);
}

vec2 motionAt(vec2 position, ivec2 velocitySize, vec2 colorSize)
{
    vec2 velocity = texelFetch(sampler2D(u_velocity, pointClamp), motionTexel(position, velocitySize), 0).xy;
    return motionOf(velocity, colorSize);
}

// 1 when a surface at the distance @p a lies in front of one at @p b, fading to 0 within 2% of
// @p b behind it: the soft depth comparison, relative so it holds at any scale
float inFront(float a, float b)
{
    return clamp(1.0 - (a - b) / (0.02 * b), 0.0, 1.0);
}

// How much a pixel moving by a motion of length @p radius covers one @p distance away
float cone(float distance, float radius)
{
    return clamp(1.0 - distance / radius, 0.0, 1.0);
}

float cylinder(float distance, float radius)
{
    return 1.0 - smoothstep(0.95 * radius, 1.05 * radius, distance);
}

// The reconstruction filter of McGuire, Hennessy, Bukowski and Osman 2012, "A Reconstruction
// Filter for Plausible Motion Blur" (I3D): taps along the longest neighboring motion, jittered by
// interleaved gradient noise of the pixel position only, so frames stay deterministic
// The share of the coverage the smear moved, with the depth of field's own share of the center
// (preflight R22, R24), measured as the depth of field does: the change of coverage relative to the
// larger coverage, in full from a tenth. A still pixel, or a uniform partial coverage the smear
// leaves as it was (fog over the background color), keeps the depth of field's share. Pixels without
// coverage count as blurred
float motionBlurAmount(float sharp, float smeared, float spread)
{
    float largest = max(smeared, sharp);
    float moved = largest > 1.0e-4 ? min(abs(smeared - sharp) / (0.1 * largest), 1.0) : 1.0;
    return 1.0 - (1.0 - moved) * (1.0 - spread);
}

void main()
{
    ivec2 colorSize = textureSize(sampler2D(u_color, pointClamp), 0);
    vec2 size = vec2(colorSize);
    ivec2 x = motionTexel(uv, colorSize);
    vec4 center = yupSanitize(texelFetch(sampler2D(u_color, pointClamp), x, 0));
    float centerSpread = 0.0;
    if (motion.options.z > 0.5)
        centerSpread = texelFetch(sampler2D(u_blurAmount, pointClamp), motionTexel(uv, textureSize(sampler2D(u_blurAmount, pointClamp), 0)), 0).r;

    // Tiles where nothing moves by half a pixel keep the color and the depth of field's share
    ivec2 tiles = textureSize(sampler2D(u_neighborMax, pointClamp), 0);
    vec4 neighborhood = texelFetch(sampler2D(u_neighborMax, pointClamp), min(x / YUP_3D_MOTION_TILE_SIZE, tiles - 1), 0);
    vec2 longest = neighborhood.xy;
    if (dot(longest, longest) < 0.25)
    {
        fragColor = center;
        blurAmount = centerSpread;
        return;
    }

    float jitter = yupInterleavedGradientNoise(gl_FragCoord.xy) - 0.5;

    // Where every pixel around moves, and alike within a pixel, nothing can cover anything else:
    // the taps all count the same, without their depth and motion (Jimenez 2014, "Next Generation
    // Post Processing in Call of Duty: Advanced Warfare"). A still pixel around, slower than half a
    // pixel, takes the weighted taps below, which keep it as it is
    if (neighborhood.z >= 0.5 && length(longest) - neighborhood.z < 1.0)
    {
        vec4 uniformSum = center;
        float taps = 1.0;

        for (int i = 0; i < YUP_3D_MOTION_SAMPLES; ++i)
        {
            float t = mix(-1.0, 1.0, (float(i) + jitter + 1.0) / float(YUP_3D_MOTION_SAMPLES + 1));
            ivec2 y = clamp(ivec2(floor(vec2(x) + 0.5 + longest * t)), ivec2(0), colorSize - 1);
            if (all(equal(y, x)))
                continue;

            uniformSum += yupSanitize(texelFetch(sampler2D(u_color, pointClamp), y, 0));
            taps += 1.0;
        }

        fragColor = uniformSum / taps;
        blurAmount = motionBlurAmount(center.a, fragColor.a, centerSpread);
        return;
    }

    ivec2 depthSize = textureSize(sampler2D(u_depth, pointClamp), 0);
    ivec2 velocitySize = textureSize(sampler2D(u_velocity, pointClamp), 0);
    float distanceX = sceneDistance(uv, depthSize);
    float radiusX = max(length(motionAt(uv, velocitySize, size)), 0.5);

    float weight = 1.0 / radiusX;
    vec4 sum = center * weight;

    for (int i = 0; i < YUP_3D_MOTION_SAMPLES; ++i)
    {
        float t = mix(-1.0, 1.0, (float(i) + jitter + 1.0) / float(YUP_3D_MOTION_SAMPLES + 1));
        ivec2 y = clamp(ivec2(floor(vec2(x) + 0.5 + longest * t)), ivec2(0), colorSize - 1);

        // The pixel itself counts once, as the center: a tap landing on it would count its own
        // coverage as smeared
        if (all(equal(y, x)))
            continue;

        vec2 position = (vec2(y) + 0.5) / size;
        float distanceY = sceneDistance(position, depthSize);
        float radiusY = max(length(motionAt(position, velocitySize, size)), 0.5);
        float d = length(vec2(y - x));

        // Y in front smearing over X, X moving off Y behind it, or both moving
        float w = inFront(distanceY, distanceX) * cone(d, radiusY)
                + inFront(distanceX, distanceY) * cone(d, radiusX)
                + cylinder(d, radiusY) * cylinder(d, radiusX) * 2.0;

        weight += w;
        sum += w * yupSanitize(texelFetch(sampler2D(u_color, pointClamp), y, 0));
    }

    fragColor = sum / weight;

    blurAmount = motionBlurAmount(center.a, fragColor.a, centerSpread);
}
