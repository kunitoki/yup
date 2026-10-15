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


#ifndef YUP_3D_LIB_GTAO
#define YUP_3D_LIB_GTAO

#include "lib/view.glsl"
#include "lib/octahedral.glsl"

// The inputs of the ambient occlusion passes (AmbientOcclusionPass): the depth and the normals of
// the G-buffer on the full tier, of the depth and normal prepass on raster, both read with texelFetch
// through the nearest pointClamp, since depth32float is not filterable on GLES 3.0 and WebGL2. The
// passes write one texel per params.w x params.w input texels, at the input texel of their top-left
// (lib/probes.glsl gtaoUpsampleUv() accounts for it).

layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform GtaoBlock
{
    vec4 params; // x the occlusion distance in view units, y slices, z steps per side, w input texels per output texel
    vec4 noise;  // x 1 for the fixed 4x4 pattern, else interleaved gradient noise; y 1 when it turns with the frame; z the screen-space GI trace: 1 / the exposure of the previous scene color
    vec4 denoise; // xy the output texel step of the 1D denoise pass
} gtao;

layout(set = 1, binding = 1) uniform texture2D u_depth;
layout(set = 1, binding = 2) uniform texture2D u_normalRoughness;

// The input texel of an output texel
ivec2 gtaoInputPixel(ivec2 outputPixel)
{
    return outputPixel * int(gtao.params.w);
}

float gtaoDepth(ivec2 pixel)
{
    return texelFetch(sampler2D(u_depth, pointClamp), pixel, 0).r;
}

// The view-space position of the center of an input texel at a stored depth, inverting the
// projection the depth was drawn with (jittered on the full tier) for perspective and orthographic
// cameras alike: the view depth first, then x and y at that depth. Cheaper than a 4x4 inverse,
// for the many samples of the trace
vec3 gtaoViewPosition(ivec2 pixel, float depth)
{
    mat4 p = view.projection;
    vec2 uv = (vec2(pixel) + 0.5) * view.viewport.zw;
    vec2 ndc = vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);

    float z = (p[3][2] - depth * p[3][3]) / (depth * p[2][3] - p[2][2]);
    float w = p[2][3] * z + p[3][3];
    return vec3((ndc * w - vec2(p[2][0], p[2][1]) * z - vec2(p[3][0], p[3][1])) / vec2(p[0][0], p[1][1]), z);
}

// The world-space shading normal of an input texel
vec3 gtaoNormal(ivec2 pixel)
{
    return octahedralDecode(texelFetch(sampler2D(u_normalRoughness, pointClamp), pixel, 0).xy);
}

// The view-space shading normal of an input texel
vec3 gtaoViewNormal(ivec2 pixel)
{
    return normalize(mat3(view.view) * gtaoNormal(pixel));
}

#endif
