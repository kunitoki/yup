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

#include "lib/view.glsl"

// The lines of the debug overlays (RenderSettings::wireframe, SceneRenderer::setBoundsOverlay),
// drawn over the final image through the unjittered camera, so they hold still while the temporal
// anti-aliasing jitters the scene. YUP_3D_BOXES draws the 12 edges of the box the model matrix maps
// the unit cube onto, from the vertex index alone, two vertices per edge; otherwise the edges of a
// mesh, from its positions and a line list of its edges.

// Mirrors LinesBlock in yup_DebugOverlayPass.cpp
layout(set = 1, binding = 0) uniform LinesBlock
{
    mat4 model;
    vec4 color; // Straight alpha
} lines;

#ifndef YUP_3D_BOXES
layout(location = 0) in vec3 a_position;
#endif

layout(location = 0) out vec4 v_color;

void main()
{
#ifdef YUP_3D_BOXES
    // Edge e of the 12 runs along the axis e / 4, at the corner e % 4 of the two other axes
    int edge = gl_VertexIndex / 2;
    int axis = edge / 4;
    int corner = edge - axis * 4;
    float along = float(gl_VertexIndex - edge * 2);
    float u = float(corner % 2);
    float v = float(corner / 2);
    vec3 position = axis == 0 ? vec3(along, u, v) : (axis == 1 ? vec3(v, along, u) : vec3(u, v, along));
#else
    vec3 position = a_position;
#endif

    v_color = lines.color;
    gl_Position = view.unjitteredViewProjection * (lines.model * vec4(position, 1.0));
}
