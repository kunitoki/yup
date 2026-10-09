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


#ifndef YUP_3D_LIB_LIGHTING
#define YUP_3D_LIB_LIGHTING

#include "lib/bsdf.glsl"
#include "lib/view.glsl"

// The per-pass lighting inputs of the passes that shade, declared once: the set 1 LightingBlock
// (mirrored by LightingInputs::Block in yup_LightingInputs.h), the environment, the sun shadow map,
// and the set 0 point sampler the shadow map is read with. Then the environment lookups.

layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform LightingBlock
{
    vec4 ambient;             // rgb linear ambient, w 1 when the environment shows behind the scene (exposure is view.cameraPosition.w)
    mat4 shadowMatrix;        // world to shadow map clip space, depth from 0 to 1
    vec4 shadowInfo;          // x unused, y texel size, z normal offset, w depth bias
    vec4 environmentInfo;     // x intensity (0 for none), y last level, z background blur level, w output mode (0 ldrInline, 1 hdrReversible, 2 hdrLinear)
    vec4 irradiance[9];       // rgb coefficients of the environment diffuse light
    vec4 clusterGrid;         // full tier: tiles x, tiles y, depth slices, number of lights
    vec4 clusterDepth;        // full tier: near, far, number of directional lights, 1 when the last slice is open (infinite far plane)
} lighting;

layout(set = 1, binding = 1) uniform texture2D u_environment;
layout(set = 1, binding = 2) uniform sampler s_environment;
layout(set = 1, binding = 3) uniform texture2D u_shadow; // packed depth, read with texelFetch

// The environment towards a direction, from a latitude-longitude image: around the vertical axis
// from +Z, and from the zenith down. atan is undefined at the poles, where both arguments are zero
vec3 environmentSample(vec3 d, float level)
{
    vec2 uv = vec2(atan(d.x, d.z + 1.0e-6) / (2.0 * yupPi), acos(clamp(d.y, -1.0, 1.0)) / yupPi);
    return textureLod(sampler2D(u_environment, s_environment), uv, level).rgb;
}

// The environment reflected towards a direction, blurred by the roughness through the mip levels
vec3 environmentRadiance(vec3 d, float roughness)
{
    return environmentSample(d, roughness * lighting.environmentInfo.y);
}

// The diffuse light of the environment around a normal, from nine spherical harmonics coefficients
vec3 environmentIrradiance(vec3 n)
{
    vec3 result = lighting.irradiance[0].rgb
                + lighting.irradiance[1].rgb * n.y
                + lighting.irradiance[2].rgb * n.z
                + lighting.irradiance[3].rgb * n.x
                + lighting.irradiance[4].rgb * (n.x * n.y)
                + lighting.irradiance[5].rgb * (n.y * n.z)
                + lighting.irradiance[6].rgb * (3.0 * n.z * n.z - 1.0)
                + lighting.irradiance[7].rgb * (n.x * n.z)
                + lighting.irradiance[8].rgb * (n.x * n.x - n.y * n.y);

    return max(result, vec3(0.0));
}

// The exposed environment behind the scene through a clip position, blurred to the background level
vec3 lightingBackground(vec2 clip)
{
    vec3 color = environmentSample(viewDirection(clip), lighting.environmentInfo.z);
    return color * (lighting.environmentInfo.x * view.cameraPosition.w);
}

#endif
