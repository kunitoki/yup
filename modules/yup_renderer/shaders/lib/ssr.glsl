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

#ifndef YUP_3D_LIB_SSR
#define YUP_3D_LIB_SSR

#include "lib/view.glsl"
#include "lib/octahedral.glsl"
#include "lib/bsdf.glsl"

// The inputs both screen-space reflection passes (ReflectionPass) read: the Hi-Z pyramid of the
// G-buffer depth, whose mip 0 is the depth itself, and the G-buffer normals and roughness (RT1), all
// read with texelFetch through the nearest pointClamp. Positions are rebuilt with the jittered
// projection the G-buffer was drawn with, in view space.

layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform SsrBlock
{
    vec4 params; // x perceptual roughness from which the confidence fades, y the roughness cut, z hit thickness (fraction of the view depth), w cells marched at most
    vec4 frame;  // x 1 / the exposure of the previous scene color, y Hi-Z mip levels, z width of the screen edge fade (uv), w 1 when the resolve's history is valid
} ssr;

layout(set = 1, binding = 1) uniform texture2D u_hiZ;
layout(set = 1, binding = 2) uniform texture2D u_normalRoughness;

float ssrDepth(ivec2 pixel)
{
    return texelFetch(sampler2D(u_hiZ, pointClamp), pixel, 0).r;
}

// The distance along the view direction of a stored depth, for perspective and orthographic projections
float ssrViewDepth(float depth)
{
    mat4 p = view.projection;
    return (depth * p[3][3] - p[3][2]) / (depth * p[2][3] - p[2][2]);
}

// The view-space position at a uv (from the top-left) and a stored depth, inverting the projection
// for perspective and orthographic cameras alike, as lib/gtao.glsl does
vec3 ssrViewPosition(vec2 uv, float depth)
{
    mat4 p = view.projection;
    vec2 ndc = vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);

    float z = -ssrViewDepth(depth);
    float w = p[2][3] * z + p[3][3];
    return vec3((ndc * w - vec2(p[2][0], p[2][1]) * z - vec2(p[3][0], p[3][1])) / vec2(p[0][0], p[1][1]), z);
}

// The unit vector from a view-space position towards the camera
vec3 ssrViewVector(vec3 position)
{
    return view.projection[2][3] < -0.5 ? normalize(-position) : vec3(0.0, 0.0, 1.0);
}

// The view-space shading normal (xyz) and the perceptual roughness (w) of a pixel
vec4 ssrNormalRoughness(ivec2 pixel)
{
    vec4 encoded = texelFetch(sampler2D(u_normalRoughness, pointClamp), pixel, 0);
    return vec4(normalize(mat3(view.view) * octahedralDecode(encoded.xy)), encoded.z);
}

#endif
