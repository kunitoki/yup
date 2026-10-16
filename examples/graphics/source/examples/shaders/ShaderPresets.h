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
};
