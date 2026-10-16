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

// Ribbon Clouds by Abhishek Shivakumar (@godofecht).
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

float hash3(vec3 p)
{
    return fract(sin(1e3 * dot(p, vec3(1.0, 57.0, -13.7))) * 4375.5453);
}

float noise3(vec3 x)
{
    vec3 p = floor(x);
    vec3 f = fract(x);

    f = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(
            mix(hash3(p + vec3(0.0, 0.0, 0.0)), hash3(p + vec3(1.0, 0.0, 0.0)), f.x),
            mix(hash3(p + vec3(0.0, 1.0, 0.0)), hash3(p + vec3(1.0, 1.0, 0.0)), f.x),
            f.y
        ),
        mix(
            mix(hash3(p + vec3(0.0, 0.0, 1.0)), hash3(p + vec3(1.0, 0.0, 1.0)), f.x),
            mix(hash3(p + vec3(0.0, 1.0, 1.0)), hash3(p + vec3(1.0, 1.0, 1.0)), f.x),
            f.y
        ),
        f.z
    );
}

float noise(vec3 x)
{
    return 0.5 * (noise3(x) + noise3(x + 11.5));
}

float fbm(vec3 p)
{
    float s = 0.0;
    float a = 0.5;

    for (int i = 0; i < 5; ++i)
    {
        s += a * noise(p);
        p *= 2.02;
        a *= 0.5;
    }

    return s;
}

mat2 rot(float a)
{
    float c = cos(a);
    float s = sin(a);
    return mat2(c, -s, s, c);
}

vec3 palette(float t)
{
    return 0.55 + 0.45 * cos(6.28318 * (vec3(0.00, 0.12, 0.24) + t));
}

void mainImage(out vec4 O, vec2 U)
{
    vec2 R = iResolution.xy;
    vec2 uv = (2.0 * U - R) / R.y;
    float t = iTime;

    vec2 p = uv;
    p *= rot(0.15 * sin(0.35 * t));

    float zoom = exp(0.20 * t);
    float pulse = 1.0 + 0.12 * sin(1.3 * t);

    vec3 col = vec3(0.0);
    float trans = 1.0;
    float glow = 0.0;

    vec3 bg = vec3(0.01, 0.005, 0.02);
    bg += 0.04 * palette(0.1 * length(uv) - 0.05 * t);

    for (int i = 0; i < 56; ++i)
    {
        float fi = float(i) / 56.0;
        float depth = fi * fi;
        float z = 2.5 * fi + 1.25 * t;

        float scale = mix(0.6, 5.0, depth) * zoom * pulse;
        vec2 q = p * scale;

        q *= rot(0.35 * fi + 0.2 * sin(t + 9.0 * fi));
        q += 0.18 * vec2(sin(z + 2.0 * fi), cos(1.3 * z - fi));

        vec3 pp = vec3(q * 1.1, z);

        float f1 = fbm(pp);
        float f2 = fbm(pp * 1.9 + vec3(2.0, -1.0, 0.7));
        float f = mix(f1, f2, 0.35);

        float bands = sin(6.28318 * (8.0 * f - 1.6 * fi + 0.18 * t));
        float ribbon = pow(max(0.0, 1.0 - abs(bands)), 14.0);

        float cloud = smoothstep(0.42, 0.92, f);
        float shell = exp(-2.5 * abs(length(q) - (0.55 + 0.45 * sin(2.0 * f + 1.7 * fi))));
        float density = 0.95 * ribbon + 0.28 * cloud + 0.22 * shell;

        density *= mix(0.035, 0.11, 1.0 - fi);

        vec3 c = palette(f + 0.13 * fi - 0.07 * t);
        c += 0.35 * palette(0.7 * f - 0.11 * t + 0.4) * shell;
        c += 0.25 * vec3(1.0, 0.3, 0.8) * ribbon;

        col += c * density * trans;
        glow += density * (0.6 + 0.4 * cloud);

        trans *= 1.0 - clamp(density * 0.85, 0.0, 0.92);

        if (trans < 0.01)
        {
            break;
        }
    }

    col += bg * trans;
    col += 0.12 * glow * palette(0.12 * length(uv) - 0.15 * t);

    float spark = hash3(vec3(floor(U * 0.5), floor(t * 14.0)));
    col += 0.18 * smoothstep(0.995, 1.0, spark) * palette(length(uv) + 0.2 * t);

    float vignette = max(0.0, 1.0 - 0.16 * dot(uv, uv));
    col *= vignette;

    col = 1.0 - exp(-1.4 * col);
    col = pow(col, vec3(0.85));

    O = vec4(col, 1.0);
}

void main()
{
    mainImage(outputColor, vec2(gl_FragCoord.x, u.height - gl_FragCoord.y));
}
