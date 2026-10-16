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

#include "lib/fog.glsl"

// Applies the analytic world fog (FogApply) to the scene color, from the camera to the depth of each
// pixel, or to the far plane (its limit at infinity for an infinite far plane) where nothing was drawn.
// Full tier: outputs (S, 1 - T), which the premultiplied-over blend turns into dst * T + S and
// a * T + (1 - T) in place, before temporal anti-aliasing. Raster tier (fog.options.x): reads the
// reversibly tone mapped scene color, inverts it and writes the same fogged linear color into a new
// target. Every texture is read by uv with sizes from textureSize.

layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 1) uniform texture2D u_depth;
layout(set = 1, binding = 2) uniform texture2D u_color;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    ivec2 depthSize = textureSize(sampler2D(u_depth, pointClamp), 0);
    float depth = texelFetch(sampler2D(u_depth, pointClamp), clamp(ivec2(uv * vec2(depthSize)), ivec2(0), depthSize - 1), 0).r;
    vec2 clip = vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);

    // Orthographic rays start on the near plane, perspective ones at the camera. The jittered
    // matrices, as the depth was rasterized
    vec4 nearPoint = view.inverseViewProjection * vec4(clip, 0.0, 1.0);
    vec3 origin = view.projection[3][3] > 0.5 ? nearPoint.xyz / nearPoint.w : view.cameraPosition.xyz;

    vec4 inFront;
    if (depth >= 1.0 && fog.options.y > 0.5)
    {
        vec3 direction = viewDirection(clip);
        inFront = heightFog(fogOpticalDepthToInfinity(origin, direction), direction);
    }
    else
    {
        vec4 world = view.inverseViewProjection * vec4(clip, min(depth, 1.0), 1.0);
        vec3 position = world.xyz / world.w;
        vec3 ray = position - origin;
        inFront = heightFog(fogOpticalDepth(origin, position), dot(ray, ray) > 1.0e-12 ? normalize(ray) : viewDirection(clip));
    }

    if (fog.options.x < 0.5)
    {
        fragColor = vec4(inFront.rgb, 1.0 - inFront.a);
        return;
    }

    ivec2 colorSize = textureSize(sampler2D(u_color, pointClamp), 0);
    vec4 c = yupSanitize(texelFetch(sampler2D(u_color, pointClamp), clamp(ivec2(uv * vec2(colorSize)), ivec2(0), colorSize - 1), 0));
    vec4 linear = vec4(yupUnpremultiplyInverse(c), c.a);
    fragColor = vec4(min(linear.rgb * inFront.a + inFront.rgb, vec3(yupMaxHalf)), linear.a * inFront.a + 1.0 - inFront.a);
}
