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

#define YUP_3D_VIEW_LINEAR_DEPTH

#include "lib/color.glsl"
#include "lib/view.glsl"
#include "lib/octahedral.glsl"

// Shows an intermediate texture of the full tier in place of the final image
// (RenderSettings::debugView), each pixel reading the texel covering it without filtering, so a
// half resolution texture shows its texels as 2 x 2 blocks. The values are written as they are,
// without tone mapping nor sRGB encoding, and opaque.

layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform DebugViewBlock
{
    vec4 options; // x what to show: 0 black (no texture this frame), else the RenderSettings::DebugView
                  // of u_source: 1 the ambient occlusion visibility in a, 2 radiance in rgb weighted by a,
                  // 3 radiance in rgb; both exposed, 4 a signed circle of confusion in r, 5 the depth
                  // (u_source unused), 6 the G-buffer RT1 normal, 7 the velocity in rg, 8 the G-buffer
                  // RT0 base color, 9 the RT1 roughness, 10 the RT1 metallic; y the scale of the circle
                  // of confusion, 1 / its largest radius
} debugView;

layout(set = 1, binding = 1) uniform texture2D u_source;
layout(set = 1, binding = 2) uniform texture2D u_depth; // The depth of the opaque surfaces, read by the views from 5 on

layout(location = 0) out vec4 fragColor;

// The texel of a texture covering this pixel of the frame
ivec2 debugTexel(ivec2 size)
{
    return min(ivec2(gl_FragCoord.xy) * size / ivec2(view.viewport.xy), size - 1);
}

// White at the near plane, darkening on a logarithmic scale to the far plane, or to 10000 times the
// near distance with an infinite far plane
vec3 debugDepth(float depth)
{
    float near = max(view.clipPlanes.x, 1.0e-4);
    float far = view.clipPlanes.y > near ? view.clipPlanes.y : near * 10000.0;
    float distance = max(viewLinearDepth(depth), near);
    return vec3(1.0 - clamp(log2(distance / near) / log2(far / near), 0.0, 1.0));
}

// The direction of a motion as a hue, its speed as the brightness
vec3 debugMotion(vec2 velocity, vec2 size)
{
    vec2 pixels = velocity * size;
    float speed = length(pixels);
    if (speed < 1.0e-3)
        return vec3(0.0);

    float hue = atan(pixels.y, pixels.x) / 6.28318530718 + 0.5;
    vec3 rgb = clamp(abs(fract(hue + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0) - 1.0, 0.0, 1.0);
    return rgb * clamp(speed / 16.0, 0.0, 1.0);
}

void main()
{
    int mode = int(debugView.options.x + 0.5);
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);

    if (mode == 0)
        return;

    // The surface views show black where nothing was drawn, at the cleared depth
    if (mode >= 5)
    {
        ivec2 depthSize = textureSize(sampler2D(u_depth, pointClamp), 0);
        float depth = texelFetch(sampler2D(u_depth, pointClamp), debugTexel(depthSize), 0).r;
        bool background = ! (depth < 1.0);

        if (mode == 5)
        {
            fragColor.rgb = background ? vec3(0.0) : debugDepth(depth);
            return;
        }

        if (background && mode != 7)
            return;
    }

    ivec2 size = textureSize(sampler2D(u_source, pointClamp), 0);
    vec4 texelValue = texelFetch(sampler2D(u_source, pointClamp), debugTexel(size), 0);
    vec4 t = yupSanitize(texelValue);
    vec4 signedValue = mix(texelValue, vec4(0.0), yupIsNonFinite(texelValue)); // Finite, keeping the sign

    if (mode == 1)
        fragColor.rgb = vec3(t.a);
    else if (mode == 4) // Signed: not sanitized, which clamps it to positive values; the setup pass keeps it finite. The square root shows small blurs
        fragColor.rgb = (texelValue.r < 0.0 ? vec3(1.0, 0.5, 0.0) : vec3(0.0, 0.5, 1.0)) * sqrt(clamp(abs(texelValue.r) * debugView.options.y, 0.0, 1.0));
    else if (mode == 6)
        fragColor.rgb = octahedralDecode(signedValue.xy) * 0.5 + 0.5;
    else if (mode == 7)
        fragColor.rgb = debugMotion(signedValue.rg, vec2(size));
    else if (mode == 8)
        fragColor.rgb = clamp(t.rgb, 0.0, 1.0);
    else if (mode == 9)
        fragColor.rgb = vec3(clamp(t.z, 0.0, 1.0));
    else if (mode == 10)
        fragColor.rgb = vec3(clamp(t.w, 0.0, 1.0));
    else
        fragColor.rgb = clamp(t.rgb * (mode == 2 ? t.a : 1.0), 0.0, 1.0);
}
