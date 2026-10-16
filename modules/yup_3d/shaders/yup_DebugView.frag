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

// Shows an intermediate texture of the full tier in place of the final image
// (RenderSettings::debugView), each pixel reading the texel covering it without filtering, so a
// half resolution texture shows its texels as 2 x 2 blocks. The values are written as they are,
// without tone mapping nor sRGB encoding, and opaque.

layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform DebugViewBlock
{
    vec4 options; // x what to show: 0 black (no texture this frame), 1 the ambient occlusion visibility in a,
                  // 2 radiance in rgb weighted by a, 3 radiance in rgb; both unexposed, 4 a signed circle of
                  // confusion in r; y the scale of the circle of confusion, 1 / its largest radius
} debugView;

layout(set = 1, binding = 1) uniform texture2D u_source;

layout(location = 0) out vec4 fragColor;

void main()
{
    int mode = int(debugView.options.x + 0.5);
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);

    if (mode == 0)
        return;

    ivec2 size = textureSize(sampler2D(u_source, pointClamp), 0);
    ivec2 texel = min(ivec2(gl_FragCoord.xy) * size / ivec2(view.viewport.xy), size - 1);
    vec4 texelValue = texelFetch(sampler2D(u_source, pointClamp), texel, 0);
    vec4 t = yupSanitize(texelValue);

    if (mode == 1)
        fragColor.rgb = vec3(t.a);
    else if (mode == 4) // Signed: not sanitized, which clamps it to positive values; the setup pass keeps it finite
        fragColor.rgb = (texelValue.r < 0.0 ? vec3(1.0, 0.5, 0.0) : vec3(0.0, 0.5, 1.0)) * clamp(abs(texelValue.r) * debugView.options.y, 0.0, 1.0);
    else
        fragColor.rgb = clamp(t.rgb * (mode == 2 ? t.a : 1.0) * view.cameraPosition.w, 0.0, 1.0);
}
