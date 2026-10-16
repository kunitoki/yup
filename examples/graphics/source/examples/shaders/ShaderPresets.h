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

#pragma once

//==============================================================================
/** A Shadertoy-style fragment shader shown in the Shaders demo list. */
struct ShaderPreset
{
    const char* name;
    const char* fragment;
};

inline constexpr ShaderPreset shaderPresets[] = {
    { "Default", R"glsl(void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord / iResolution.xy;

    // Time varying pixel color
    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0.0, 2.0, 4.0));

    fragColor = vec4(col, 1.0);
}
)glsl" },

    { "Creation", R"glsl(// http://www.pouet.net/prod.php?which=57245
// If you intend to reuse this shader, please add credits to 'Danilo Guanabara'

#define t iTime
#define r iResolution.xy

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
    vec3 c;
    float l,z=t;
    for(int i=0;i<3;i++) {
        vec2 uv,p=fragCoord.xy/r;
        uv=p;
        p-=.5;
        p.x*=r.x/r.y;
        z+=.07;
        l=length(p);
        uv+=p/l*(sin(z)+1.)*abs(sin(l*9.-z-z));
        c[i]=.01/length(mod(uv,1.)-.5);
    }
    fragColor=vec4(c/l,t);
}
)glsl" },

    { "Plasma", R"glsl(void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 p = (2.0 * fragCoord - iResolution.xy) / iResolution.y;
    float t = iTime * 0.6;

    float v = sin(p.x * 3.0 + t);
    v += sin(3.0 * (p.x * sin(t * 0.5) + p.y * cos(t * 0.3)) + t);
    v += sin(length(p * 4.0 + vec2(sin(t), cos(t * 1.3))) * 2.0 - t);
    v *= 0.5;

    vec3 col = 0.5 + 0.5 * cos(3.14159 * v + vec3(0.0, 2.1, 4.2));
    fragColor = vec4(col, 1.0);
}
)glsl" },

    { "Raymarched Spheres", R"glsl(// x: distance, y: material (0 = floor, 1..3 = spheres)
vec2 map(vec3 p)
{
    vec2 res = vec2(p.y + 1.0, 0.0);

    for (int i = 0; i < 3; ++i)
    {
        float a = iTime * 0.7 + float(i) * 2.0944;
        vec3 center = vec3(1.4 * cos(a), 0.3 * sin(iTime * 1.3 + float(i)), 1.4 * sin(a));
        float d = length(p - center) - 0.55;

        if (d < res.x)
            res = vec2(d, float(i + 1));
    }

    return res;
}

vec3 calcNormal(vec3 p)
{
    const vec2 e = vec2(0.001, 0.0);
    return normalize(vec3(map(p + e.xyy).x - map(p - e.xyy).x,
                          map(p + e.yxy).x - map(p - e.yxy).x,
                          map(p + e.yyx).x - map(p - e.yyx).x));
}

float softShadow(vec3 p, vec3 l)
{
    float shadow = 1.0;
    float t = 0.02;

    for (int i = 0; i < 32; ++i)
    {
        float h = map(p + l * t).x;
        shadow = min(shadow, 8.0 * h / t);
        t += clamp(h, 0.02, 0.2);

        if (shadow < 0.001 || t > 6.0)
            break;
    }

    return clamp(shadow, 0.0, 1.0);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = (2.0 * fragCoord - iResolution.xy) / iResolution.y;

    vec3 ro = vec3(0.0, 0.6, 4.0);
    vec3 rd = normalize(vec3(uv.x, uv.y - 0.15, -1.6));

    float t = 0.0;
    vec2 hit = vec2(-1.0);

    for (int i = 0; i < 100; ++i)
    {
        vec2 h = map(ro + rd * t);

        if (h.x < 0.001)
        {
            hit = vec2(t, h.y);
            break;
        }

        t += h.x;

        if (t > 20.0)
            break;
    }

    vec3 sky = mix(vec3(0.6, 0.75, 0.95), vec3(0.15, 0.2, 0.35), uv.y * 0.5 + 0.5);
    vec3 col = sky;

    if (hit.x > 0.0)
    {
        vec3 p = ro + rd * hit.x;
        vec3 n = calcNormal(p);
        vec3 l = normalize(vec3(0.6, 0.8, 0.4));

        float diffuse = max(dot(n, l), 0.0);
        float shadow = softShadow(p, l);
        float specular = pow(max(dot(reflect(-l, n), -rd), 0.0), 32.0);

        vec3 albedo = hit.y < 0.5
                        ? vec3(0.35 + 0.15 * mod(floor(p.x) + floor(p.z), 2.0))
                        : 0.5 + 0.5 * cos(hit.y * 2.0 + vec3(0.0, 2.0, 4.0));

        col = albedo * (0.15 + 0.85 * diffuse * shadow) + specular * shadow * 0.5;
        col = mix(col, sky, 1.0 - exp(-0.02 * hit.x * hit.x));
    }

    fragColor = vec4(pow(col, vec3(0.4545)), 1.0);
}
)glsl" },

    { "Mouse Ripples", R"glsl(// Drag on the shader: ripples follow iMouse.xy, and pulse while the button is down.
void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 m = iMouse.xy == vec2(0.0) ? 0.5 * iResolution.xy : iMouse.xy;
    vec2 toMouse = fragCoord - m;
    float d = length(toMouse) / iResolution.y;

    float wave = sin(d * 60.0 - iTime * 6.0) * exp(-d * 4.0);
    vec2 uv = fragCoord / iResolution.xy + normalize(toMouse + 1e-4) * wave * 0.01;

    vec3 col = 0.5 + 0.5 * cos(6.2831 * (uv.xyx * 1.5 + vec3(0.0, 0.33, 0.67)) + iTime * 0.5);
    col *= 0.75 + 0.25 * wave;

    vec2 g = abs(fract(uv * vec2(16.0, 9.0)) - 0.5);
    col = mix(col, vec3(1.0), smoothstep(0.45, 0.5, max(g.x, g.y)) * 0.35);

    float pressed = iMouse.z > 0.0 ? 1.0 : 0.0;
    col += pressed * 0.4 * smoothstep(0.02, 0.0, abs(d - fract(iTime * 0.5) * 0.6));

    fragColor = vec4(col, 1.0);
}
)glsl" },

    { "Noise Warp", R"glsl(// Domain warping through the iChannel0 noise texture, sampled in texels.
float fbm(vec2 p)
{
    float v = 0.0;
    float a = 0.5;

    for (int i = 0; i < 5; ++i)
    {
        v += a * texture(iChannel0, p / iChannelResolution[0].xy).x;
        p = p * 2.03 + vec2(17.0, 31.0);
        a *= 0.5;
    }

    return v;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 p = fragCoord / iResolution.y * 64.0;
    float t = iTime * 4.0;

    vec2 q = vec2(fbm(p + vec2(0.0, t)), fbm(p + vec2(52.0, -t)));
    vec2 r = vec2(fbm(p + 64.0 * q + vec2(17.0, 9.2) + 0.5 * t),
                  fbm(p + 64.0 * q + vec2(83.0, 28.0) - 0.3 * t));
    float f = fbm(p + 64.0 * r);

    vec3 col = mix(vec3(0.1, 0.15, 0.3), vec3(0.95, 0.6, 0.25), clamp(f * f * 2.0, 0.0, 1.0));
    col = mix(col, vec3(0.2, 0.7, 0.8), clamp(length(q) - 0.5, 0.0, 1.0));

    fragColor = vec4(col, 1.0);
}
)glsl" },

    { "Shader Art", R"glsl(//https://iquilezles.org/articles/palettes/
vec3 palette( float t ) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263,0.416,0.557);

    return a + b*cos( 6.28318*(c*t+d) );
}

//https://www.shadertoy.com/view/mtyGWy
void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 uv = (fragCoord * 2.0 - iResolution.xy) / iResolution.y;
    vec2 uv0 = uv;
    vec3 finalColor = vec3(0.0);

    for (float i = 0.0; i < 4.0; i++) {
        uv = fract(uv * 1.5) - 0.5;

        float d = length(uv) * exp(-length(uv0));

        vec3 col = palette(length(uv0) + i*.4 + iTime*.4);

        d = sin(d*8. + iTime)/8.;
        d = abs(d);

        d = pow(0.01 / d, 1.2);

        finalColor += col * d;
    }

    fragColor = vec4(finalColor, 1.0);
}
)glsl" },

    { "Curl Bands", R"glsl(// Curl Bands by Abhishek Shivakumar (@godofecht).
// Copyright (c) 2026 Abhishek Shivakumar. Distributed under the ISC license above.

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
)glsl" },

    { "Ribbon Clouds", R"glsl(// Ribbon Clouds by Abhishek Shivakumar (@godofecht).
// Copyright (c) 2026 Abhishek Shivakumar. Distributed under the ISC license above.

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
)glsl" },
};
