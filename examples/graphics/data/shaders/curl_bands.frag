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

// Curl Bands by Abhishek Shivakumar (@godofecht).
// Copyright (c) 2026 Abhishek Shivakumar. Distributed under the ISC license above.
layout(set = 0, binding = 0) uniform Params
{
    float time;
    float width;
    float height;
    float padding;
} u;

#define iResolution vec3(u.width, u.height, 1.0)
#define iTime u.time

layout(location = 0) out vec4 outputColor;

#define TAU 6.28318530718

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 hash22(vec2 p)
{
    float n = hash12(p);
    return vec2(n, hash12(p + n + 19.19));
}

float vnoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p)
{
    float v = 0.0;
    float a = 0.55;
    mat2 m = mat2(1.6, -1.2, 1.2, 1.6);
    for (int i = 0; i < 6; i++)
    {
        v += a * vnoise(p);
        p = m * p;
        a *= 0.5;
    }
    return v;
}

float field(vec2 p, float t)
{
    // Time-varying scalar field for curl: a couple of drifting FBMs
    float f1 = fbm(p + vec2(0.07, -0.05) * t);
    float f2 = fbm(p * 1.8 - vec2(0.03, 0.08) * t);
    return 0.65 * f1 + 0.35 * f2;
}

vec2 curl2(vec2 p, float t)
{
    // curl of scalar field: (dF/dy, -dF/dx)
    float e = 0.0015;
    float fx1 = field(p + vec2(e, 0.0), t);
    float fx0 = field(p - vec2(e, 0.0), t);
    float fy1 = field(p + vec2(0.0, e), t);
    float fy0 = field(p - vec2(0.0, e), t);

    float dFdx = (fx1 - fx0) / (2.0 * e);
    float dFdy = (fy1 - fy0) / (2.0 * e);

    return vec2(dFdy, -dFdx);
}

vec2 advect(vec2 p, float t, float freq, float strength)
{
    vec2 q = p * freq;
    vec2 v = curl2(q, t);
    return p + v * (strength / freq);
}

float band(float x, float center, float width)
{
    // Sharp band around center (width in [0..1] domain)
    float a = smoothstep(center - width, center, x);
    float b = smoothstep(center, center + width, x);
    return a - b;
}

vec3 rampBands(float x)
{
    // Multi-band ramps with sharp transitions; tweak centers/widths/colors
    float w = 0.015;

    vec3 c0 = vec3(0.03, 0.02, 0.05);
    vec3 c1 = vec3(0.12, 0.06, 0.22);
    vec3 c2 = vec3(0.10, 0.35, 0.55);
    vec3 c3 = vec3(0.85, 0.35, 0.18);
    vec3 c4 = vec3(0.98, 0.90, 0.60);

    float b1 = smoothstep(0.20 - w, 0.20 + w, x);
    float b2 = smoothstep(0.45 - w, 0.45 + w, x);
    float b3 = smoothstep(0.70 - w, 0.70 + w, x);
    float b4 = smoothstep(0.88 - w, 0.88 + w, x);

    vec3 col = c0;
    col = mix(col, c1, b1);
    col = mix(col, c2, b2);
    col = mix(col, c3, b3);
    col = mix(col, c4, b4);

    // Extra razor bands (thin highlights/ink lines)
    col += 0.55 * band(x, 0.33, 0.006) * vec3(0.9, 0.95, 1.0);
    col += 0.35 * band(x, 0.61, 0.006) * vec3(0.2, 0.1, 0.0);
    col += 0.45 * band(x, 0.80, 0.005) * vec3(1.0, 0.7, 0.2);

    return col;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    float t = iTime;

    vec2 p0 = uv * 1.2;

    vec2 p = p0;

    p = advect(p, t * 2.1, 12.0, 0.14);

    vec2 p2 = p;
    p2 = advect(p2, t * 1.1 + 3.7, 2.2, 0.30);
    p2 = mix(p, p2, 0.55);

    float rotary = sin (t) + cos (1. - t);

    vec2 v = curl2(p2 * 2.0, t) * rotary + (p2)*(1.0 - rotary);

    float dens = fbm(p2 * 1.4 + 0.15 * t);
    float flow = clamp(length(v) * 0.55, 0.0, 1.0);

    float x = dens * 0.75 + flow * 0.25;
    x = smoothstep(0.05, 0.98, x);

    vec3 col = rampBands(x);

    float ca = fbm(p2 * 3.0 - 0.2 * t);
    float highlight = pow(clamp(1.0 - abs(2.0 * ca - 1.0), 0.0, 1.0), 10.0);
    col += 0.22 * highlight;

    float vig = (1.0 - smoothstep(0.15, 1.35, length(uv)));
    col *= mix(0.85, 1.05, vig);

    col = pow(col, vec3(0.95));

    fragColor = vec4(col, 1.0);
}

void main()
{
    mainImage(outputColor, vec2(gl_FragCoord.x, u.height - gl_FragCoord.y));
}
