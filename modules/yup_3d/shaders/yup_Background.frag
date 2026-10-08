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

#include "lib/color.glsl"
#include "lib/view.glsl"

// Shows the environment behind the scene, blurred to one of its roughness levels, exposed and
// written per output mode like the surfaces. The full variant also writes the velocity of the
// environment, which only moves with the camera.

// Must match ForwardLightingBlock in yup_ForwardPass.cpp and LightingBlock in yup_ForwardPass.frag
layout(set = 1, binding = 0) uniform LightingBlock
{
    vec4 ambient;
    vec4 lightInfo;
    vec4 lightPositions[8];
    vec4 lightDirections[8];
    vec4 lightColors[8];
    vec4 lightSpots[8];
    mat4 shadowMatrix;
    vec4 shadowInfo;
    vec4 environmentInfo;     // x intensity, y last level, z background blur level, w output mode (0 ldrInline, 1 hdrReversible, 2 hdrLinear)
    vec4 irradiance[9];
} lighting;

layout(set = 1, binding = 1) uniform texture2D u_environment;
layout(set = 1, binding = 2) uniform sampler s_environment;

layout(location = 0) in vec2 v_clip;

layout(location = 0) out vec4 fragColor;

#ifdef YUP_3D_TIER_FULL
layout(location = 1) out vec2 outVelocity;
#endif

const float PI = 3.14159265359;

void main()
{
    // From the near to the far plane through this pixel, for perspective and orthographic cameras
    vec4 nearPoint = view.inverseViewProjection * vec4(v_clip, 0.0, 1.0);
    vec4 farPoint = view.inverseViewProjection * vec4(v_clip, 1.0, 1.0);
    vec3 d = normalize(farPoint.xyz / farPoint.w - nearPoint.xyz / nearPoint.w);

    // atan is undefined at the poles, where both arguments are zero
    vec2 uv = vec2(atan(d.x, d.z + 1.0e-6) / (2.0 * PI), acos(clamp(d.y, -1.0, 1.0)) / PI);
    vec3 color = textureLod(sampler2D(u_environment, s_environment), uv, lighting.environmentInfo.z).rgb;

    vec3 exposed = color * (lighting.environmentInfo.x * view.cameraPosition.w);

    if (lighting.environmentInfo.w < 0.5)
        fragColor = vec4(yupLinearToSrgb(yupToneMap(exposed, view.clipPlanes.w)), 1.0);
    else if (lighting.environmentInfo.w < 1.5)
        fragColor = vec4(yupReversibleToneMap(min(exposed, vec3(yupMaxHalf))), 1.0);
    else
        fragColor = vec4(min(exposed, vec3(yupMaxHalf)), 1.0);

#ifdef YUP_3D_TIER_FULL
    // A direction at infinity: only the camera rotation moves it. Orthographic cameras put it at
    // w = 0, where it does not move at all
    vec4 current = view.unjitteredViewProjection * vec4(d, 0.0);
    vec4 previous = view.previousViewProjection * vec4(d, 0.0);
    outVelocity = current.w > 1.0e-6 && previous.w > 1.0e-6
                    ? (current.xy / current.w - previous.xy / previous.w) * vec2(0.5, -0.5)
                    : vec2(0.0);
#endif
}
