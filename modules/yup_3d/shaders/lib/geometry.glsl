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


#ifndef YUP_3D_LIB_GEOMETRY
#define YUP_3D_LIB_GEOMETRY

#include "lib/view.glsl"

// The vertex stage of the passes drawing meshes (ForwardPass, GBufferPass): the set 2 DrawBlock,
// the Mesh::Vertex attributes, and the varyings lib/material.glsl reads. Vertex stage only.

// Mirrors GeometryBindings::DrawBlock in yup_GeometryBindings.h
layout(set = 2, binding = 0) uniform DrawBlock
{
    mat4 model;
    mat4 normalMatrix;        // inverse transpose of the model matrix
    mat4 previousModel;       // the model matrix of the previous frame
    vec4 ids;                 // x material id in the frame's MaterialTable (GBufferPass), yzw reserved
} draw;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_color;
layout(location = 4) in vec4 a_tangent; // xyz unit tangent, w handedness; all zero for none

layout(location = 0) out vec3 v_worldPosition;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out vec4 v_color;
layout(location = 6) out vec4 v_tangent; // world-space xyz, w handedness; zero selects the derivative frame

#ifdef YUP_3D_TIER_FULL
layout(location = 4) out vec4 v_currentClip;
layout(location = 5) out vec4 v_previousClip;
#endif

// Transforms the vertex and writes every varying above, and gl_Position
void geometryMain()
{
    vec4 worldPosition = draw.model * vec4(a_position, 1.0);

    v_worldPosition = worldPosition.xyz;
    v_normal = (draw.normalMatrix * vec4(a_normal, 0.0)).xyz;
    v_uv = a_uv;
    v_color = a_color;

    // Tangents follow the surface, so they take the model matrix, not the normal matrix. A mirroring
    // model matrix reverses the handedness of the frame
    bool hasTangent = a_tangent.w != 0.0 && dot(a_tangent.xyz, a_tangent.xyz) > 0.0;
    float handedness = a_tangent.w * sign(determinant(mat3(draw.model)));
    v_tangent = hasTangent ? vec4(normalize((draw.model * vec4(a_tangent.xyz, 0.0)).xyz), handedness) : vec4(0.0);

#ifdef YUP_3D_TIER_FULL
    v_currentClip = view.unjitteredViewProjection * worldPosition;
    v_previousClip = view.previousViewProjection * (draw.previousModel * vec4(a_position, 1.0));
#endif

    gl_Position = view.projection * (view.view * worldPosition);
}

#endif
