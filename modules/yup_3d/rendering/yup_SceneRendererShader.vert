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

// Must match FrameData in yup_SceneRenderer.cpp and yup_SceneRendererShader.frag
layout(set = 0, binding = 0) uniform FrameData
{
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;      // xyz
    vec4 ambient;             // rgb linear ambient, w exposure
    vec4 lightInfo;           // x light count
    vec4 lightPositions[8];   // xyz world position, w type (0 directional, 1 point, 2 spot)
    vec4 lightDirections[8];  // xyz unit direction the light shines towards, w range (0 infinite)
    vec4 lightColors[8];      // rgb linear color, w intensity
    vec4 lightSpots[8];       // x cone scale, y cone offset
} frame;

layout(set = 0, binding = 1) uniform DrawData
{
    mat4 model;
    mat4 normalMatrix;        // inverse transpose of the model matrix
} draw;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_color;

layout(location = 0) out vec3 v_worldPosition;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out vec4 v_color;

void main()
{
    vec4 worldPosition = draw.model * vec4(a_position, 1.0);

    v_worldPosition = worldPosition.xyz;
    v_normal = (draw.normalMatrix * vec4(a_normal, 0.0)).xyz;
    v_uv = a_uv;
    v_color = a_color;

    gl_Position = frame.projection * (frame.view * worldPosition);
}
