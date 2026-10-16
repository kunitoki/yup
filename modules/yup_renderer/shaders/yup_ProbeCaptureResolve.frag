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

// Copies a reflection probe capture, drawn by the ForwardPass into a 2D target, into one face of
// mip 0 of the probe's cube. The ForwardPass draws the face with a right-handed camera looking along
// the face's axis, which shows it mirrored left to right against the orientation of the cube
// (lib/cube.glsl): the copy flips it back. Below the float floor the capture holds premultiplied,
// sRGB encoded color, tone mapped per channel with Reinhard c / (1 + c) (ForwardPass's ldrInline
// output, which no channel saturates), in rgba8unorm: the copy un-premultiplies it, decodes the
// sRGB, inverts Reinhard per channel, premultiplies again and encodes RGBM.

layout(set = 0, binding = 3) uniform sampler pointClamp;

layout(set = 1, binding = 0) uniform ProbeCaptureBlock
{
    vec4 params; // x 1 when the capture is tone mapped and sRGB encoded, and the cube holds RGBM
} capture;

layout(set = 1, binding = 1) uniform texture2D u_capture;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec4 texel = textureLod(sampler2D(u_capture, pointClamp), vec2(1.0 - uv.x, uv.y), 0.0);
    vec3 color = texel.rgb;

    if (capture.params.x > 0.5)
    {
        vec3 mapped = texel.a > 1.0e-4 ? yupSrgbToLinear(min(texel.rgb / texel.a, vec3(1.0))) : vec3(0.0);
        color = mapped / max(1.0 - mapped, vec3(1.0e-4)) * texel.a;
    }

    fragColor = yupEncodeRadiance(color, capture.params.x);
}
