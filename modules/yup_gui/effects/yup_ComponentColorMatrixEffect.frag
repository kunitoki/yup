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

layout(set = 0, binding = 0) uniform texture2D u_source;
layout(set = 0, binding = 1) uniform sampler s_source;

layout(set = 0, binding = 2) uniform ColorMatrixParams
{
    vec4 row0;
    vec4 row1;
    vec4 row2;
    vec4 row3;
    vec4 offset;
    vec4 invSize; // xy: reciprocal of texture dimensions
} colorMatrix;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec2 uv = gl_FragCoord.xy * colorMatrix.invSize.xy;
    vec4 premul = textureLod(sampler2D(u_source, s_source), uv, 0.0);

    vec3 straightRgb = premul.a > 0.0001 ? clamp(premul.rgb / premul.a, 0.0, 1.0) : vec3(0.0);
    vec4 straight = vec4(straightRgb, premul.a);

    vec4 transformed = vec4(
        dot(colorMatrix.row0, straight),
        dot(colorMatrix.row1, straight),
        dot(colorMatrix.row2, straight),
        dot(colorMatrix.row3, straight)) + colorMatrix.offset;

    transformed = clamp(transformed, 0.0, 1.0);
    fragColor = vec4(transformed.rgb * transformed.a, transformed.a);
}
