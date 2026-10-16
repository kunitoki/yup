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

#include "lib/color.glsl"
#include "lib/view.glsl"

// Maps the HDR scene color to display sRGB with the operator in view.clipPlanes.w. The shading
// passes already applied the exposure, so it is not applied again here.
// The scene color is premultiplied by its coverage alpha over a transparent clear, and may be
// reversibly tone mapped. Each pixel tone maps the color of its covered part, un-premultiplied
// (and inverted), and composites it in display space over the display-referred background by the
// coverage, like the MSAA resolve of the raster tier blends tone mapped samples. The glow of bloom
// is added to the covered part before tone mapping, and onto the background in display space.

layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform ToneMappingBlock
{
    vec4 options;             // x 1 when the scene color is reversibly tone mapped, y 1 when u_glow holds a glow
    vec4 background;          // the display-encoded background color, composited under the scene
} toneMapping;

layout(set = 1, binding = 1) uniform texture2D u_source;
layout(set = 1, binding = 2) uniform texture2D u_glow;
#ifdef YUP_3D_RADIANCE_COMPOSITE
layout(set = 1, binding = 3) uniform texture2D u_blurAmount;
#endif

layout(location = 0) out vec4 fragColor;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec4 scene = yupSanitize(texelFetch(sampler2D(u_source, pointClamp), pixel, 0));

    vec3 glow = vec3(0.0);
    if (toneMapping.options.y > 0.5)
        glow = yupSanitize(vec4(texelFetch(sampler2D(u_glow, pointClamp), pixel, 0).rgb, 0.0)).rgb;

    vec3 color = vec3(0.0);
    if (scene.a > 1.0e-4)
    {
        color = scene.rgb / scene.a;
        if (toneMapping.options.x > 0.5)
            color = yupReversibleToneMapInverse(color);
    }

#ifdef YUP_3D_RADIANCE_COMPOSITE
    // Frames blurred by the depth of field: their partial coverage is spread light, not the edge of a
    // surface, so the premultiplied radiance is tone mapped and the background added over the rest
    // (preflight R20). Equal to the blend below at full and at zero coverage
    // The premultiplied radiance itself, so texels covered by less than the threshold above keep their light
    // Only by the share the blur wrote: pixels it left sharp keep the blend below (preflight R21), and
    // each pixel evaluates only the composites its share needs
    float blurred = texelFetch(sampler2D(u_blurAmount, pointClamp), pixel, 0).r;
    vec4 spread = vec4(0.0);
    if (blurred > 0.0)
    {
        vec3 radiance = toneMapping.options.x > 0.5 ? color * scene.a : scene.rgb;
        float spreadAlpha = scene.a + toneMapping.background.a * (1.0 - scene.a);
        spread = clamp(vec4(yupLinearToSrgb(yupToneMap(radiance + glow, view.clipPlanes.w)) + (1.0 - scene.a) * toneMapping.background.rgb, spreadAlpha), 0.0, 1.0);

        if (blurred >= 1.0)
        {
            fragColor = spread;
            return;
        }
    }
#endif

    vec3 covered = yupLinearToSrgb(yupToneMap(color + glow, view.clipPlanes.w));
    vec3 uncovered = min(toneMapping.background.rgb + yupLinearToSrgb(yupToneMap(glow, view.clipPlanes.w)), vec3(1.0));
    float alpha = scene.a + toneMapping.background.a * (1.0 - scene.a);

    fragColor = clamp(vec4(mix(uncovered, covered, scene.a), alpha), 0.0, 1.0);

#ifdef YUP_3D_RADIANCE_COMPOSITE
    fragColor = mix(fragColor, spread, blurred);
#endif
}
