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

#include "lib/bsdf.glsl"
#include "lib/color.glsl"
#include "lib/cube.glsl"

// Renders a latitude-longitude texture into one face of mip 0 of a cube. The equirect maps a unit
// direction as EnvironmentMap::getTextureCoordinates() does: u = atan (x, z) / 2pi around the
// vertical axis from +Z, v = acos (y) / pi from the zenith (0) to the nadir (1).

layout(set = 0, binding = 1) uniform sampler equirectSampler;

layout(set = 1, binding = 0) uniform ProbeEquirectBlock
{
    vec4 params; // x cube face, y 1 when the textures hold RGBM, z face size in texels, w highest equirect mip
    vec4 source; // xy equirect size at mip 0, in texels
} probe;

layout(set = 1, binding = 1) uniform texture2D u_equirect;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

// Each cube texel reads the equirect mip whose texels match its angular size, as filtered importance
// sampling picks a lod (Colbert and Krivanek 2007, "GPU-Based Importance Sampling", GPU Gems 3), so
// a large equirect does not alias into a small cube. The size is matched against the equirect's rows,
// pi / height radians apart everywhere: its columns narrow towards the poles, and a solid angle
// match would blur the rows there for that alone
// The equirect at mip 0 around some coordinates, filtered bilinearly after decoding RGBM: an RGBM
// equirect has no other mip, and filtering encoded texels would mix their multipliers. Wraps around
// the vertical axis and clamps at the poles, as the sampler does
vec3 equirectRgbmBilinear(vec2 coordinates)
{
    ivec2 size = ivec2(probe.source.xy);
    vec2 p = coordinates * probe.source.xy - 0.5;
    vec2 base = floor(p);
    vec2 f = p - base;

    vec3 sum = vec3(0.0);

    for (int j = 0; j < 2; ++j)
    {
        for (int i = 0; i < 2; ++i)
        {
            ivec2 texel = ivec2(base) + ivec2(i, j);
            texel.x = (texel.x % size.x + size.x) % size.x;
            texel.y = clamp(texel.y, 0, size.y - 1);

            float weight = (i == 0 ? 1.0 - f.x : f.x) * (j == 0 ? 1.0 - f.y : f.y);
            sum += yupDecodeRgbm(texelFetch(sampler2D(u_equirect, equirectSampler), texel, 0), 1.0) * weight;
        }
    }

    return sum;
}

void main()
{
    vec3 d = cubeDirection(int(probe.params.x), uv);
    vec2 coordinates = vec2(fract(atan(d.x, d.z) / (2.0 * yupPi)), acos(clamp(d.y, -1.0, 1.0)) / yupPi);

    float cubeTexelAngle = sqrt(cubeTexelSolidAngle(uv, probe.params.z));
    float lod = clamp(log2(cubeTexelAngle / (yupPi / probe.source.y)), 0.0, probe.params.w);

    vec3 radiance = probe.params.y < 0.5 ? yupDecodeRgbm(textureLod(sampler2D(u_equirect, equirectSampler), coordinates, lod), probe.params.y)
                                         : equirectRgbmBilinear(coordinates);

    fragColor = yupEncodeRadiance(radiance, probe.params.y);
}
