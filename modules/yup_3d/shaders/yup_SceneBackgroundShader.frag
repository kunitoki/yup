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

// Shows the environment behind the scene, blurred to one of its roughness levels, exposed and
// tone mapped like the surfaces.

// Must match BackgroundData in yup_SceneRenderer.cpp
layout(set = 0, binding = 0) uniform BackgroundData
{
    mat4 inverseViewProjection;
    vec4 info;                // x intensity, y level, z tone mapping (0 Reinhard, 1 ACES), w exposure
} background;

layout(set = 0, binding = 1) uniform texture2D u_environment;
layout(set = 0, binding = 2) uniform sampler s_environment;

layout(location = 0) in vec2 v_clip;

layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;

vec3 linearToSrgb(vec3 c)
{
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), c));
}

vec3 acesFilmic(vec3 x)
{
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main()
{
    // From the near to the far plane through this pixel, for perspective and orthographic cameras
    vec4 nearPoint = background.inverseViewProjection * vec4(v_clip, 0.0, 1.0);
    vec4 farPoint = background.inverseViewProjection * vec4(v_clip, 1.0, 1.0);
    vec3 d = normalize(farPoint.xyz / farPoint.w - nearPoint.xyz / nearPoint.w);

    // atan is undefined at the poles, where both arguments are zero
    vec2 uv = vec2(atan(d.x, d.z + 1.0e-6) / (2.0 * PI), acos(clamp(d.y, -1.0, 1.0)) / PI);
    vec3 color = textureLod(sampler2D(u_environment, s_environment), uv, background.info.y).rgb;

    color = color * (background.info.x * background.info.w);
    color = background.info.z > 0.5 ? acesFilmic(color) : color / (vec3(1.0) + color);

    fragColor = vec4(linearToSrgb(color), 1.0);
}
