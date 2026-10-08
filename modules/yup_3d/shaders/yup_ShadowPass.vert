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

// Renders the depth of the scene seen from a directional light into a shadow map.

layout(set = 0, binding = 0) uniform ShadowData
{
    mat4 shadowMatrix;        // world to shadow map clip space, depth from 0 to 1
} shadow;

layout(set = 0, binding = 1) uniform DrawData
{
    mat4 model;
    mat4 normalMatrix;
} draw;

layout(location = 0) in vec3 a_position;

layout(location = 0) out float v_depth;

void main()
{
    vec4 clip = shadow.shadowMatrix * (draw.model * vec4(a_position, 1.0));

    // The depth is passed on rather than read from gl_FragCoord, whose range depends on the backend
    v_depth = clip.z;
    gl_Position = clip;
}
