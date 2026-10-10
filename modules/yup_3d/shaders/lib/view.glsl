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


#ifndef YUP_3D_LIB_VIEW
#define YUP_3D_LIB_VIEW

// The per-view uniforms shared by every pass, at set 0, binding 0. Fields are append-only.
layout(set = 0, binding = 0) uniform ViewBlock
{
    mat4 view;
    mat4 projection;               // jittered on the full tier
    mat4 viewProjection;           // jittered
    mat4 inverseViewProjection;    // jittered
    mat4 unjitteredViewProjection;
    mat4 previousViewProjection;   // unjittered, previous frame
    vec4 cameraPosition;           // xyz, w = exposure
    vec4 viewport;                 // width, height, 1/width, 1/height
    vec4 jitter;                   // xy current, zw previous, in NDC units
    vec4 clipPlanes;               // near, far, frame index modulo 64, tone mapping operator (0 Reinhard, 1 ACES, 2 AgX)
} view;

#ifdef YUP_3D_CONTACT_SHADOWS
// The view depth (distance along the view direction) of a stored depth in [0, 1], from the
// projection: perspective, perspective with an infinite far plane, or orthographic. The jitter only
// offsets x and y, so the jittered projection gives the same depth. Undefined at depth 1 with an
// infinite far plane, which lies at infinity. Only the passes reading the scene depth declare it
float viewLinearDepth(float depth)
{
    mat4 p = view.projection;
    return (depth * p[3][3] - p[3][2]) / (depth * p[2][3] - p[2][2]);
}
#endif

// The screen-space motion between two clip positions, current uv minus previous uv, with uv from
// the top-left
vec2 viewVelocity(vec4 currentClip, vec4 previousClip)
{
    return (currentClip.xy / currentClip.w - previousClip.xy / previousClip.w) * vec2(0.5, -0.5);
}

// The unit direction away from the camera through a clip position, for perspective and orthographic
// cameras. It joins two finite points of the ray, at NDC depths 0 and 0.5 as LightClusters does:
// NDC depth 1 lies at infinity with an infinite far plane
vec3 viewDirection(vec2 clip)
{
    vec4 nearPoint = view.inverseViewProjection * vec4(clip, 0.0, 1.0);
    vec4 middlePoint = view.inverseViewProjection * vec4(clip, 0.5, 1.0);
    return normalize(middlePoint.xyz / middlePoint.w - nearPoint.xyz / nearPoint.w);
}

// The motion of the background through a clip position: a direction at infinity, which only the
// camera rotation moves. Orthographic cameras put it at w = 0, where it does not move at all
vec2 viewBackgroundVelocity(vec2 clip)
{
    vec3 d = viewDirection(clip);
    vec4 current = view.unjitteredViewProjection * vec4(d, 0.0);
    vec4 previous = view.previousViewProjection * vec4(d, 0.0);
    return current.w > 1.0e-6 && previous.w > 1.0e-6 ? viewVelocity(current, previous) : vec2(0.0);
}

#endif
