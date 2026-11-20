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

// One pass of an FilterGraph primitive: the operation picks what the pass does with its two pictures.
// The numbers of the operations, of the composite operators and of the blend modes are the ones
// yup_FilterGraph.cpp passes, the last two being the values of their enumerations.

layout(set = 0, binding = 0) uniform FilterParams
{
    vec4 metrics; // x: 1 / width, y: 1 / height, z: operation, w: variant of the operation
    vec4 a;
    vec4 b;
    vec4 c;
    vec4 row0;    // the rows of a color matrix, then what it adds
    vec4 row1;
    vec4 row2;
    vec4 row3;
    vec4 bias;
} params;

layout(set = 0, binding = 1) uniform texture2D u_first;
layout(set = 0, binding = 2) uniform sampler s_first;
layout(set = 0, binding = 3) uniform texture2D u_second;
layout(set = 0, binding = 4) uniform sampler s_second;

layout(location = 0) out vec4 fragColor;

//==============================================================================
// Around a picture there is nothing: transparent black

float inside(vec2 uv)
{
    vec2 within = step(vec2(0.0), uv) * step(uv, vec2(1.0));
    return within.x * within.y;
}

vec4 first(vec2 uv)
{
    return textureLod(sampler2D(u_first, s_first), uv, 0.0) * inside(uv);
}

vec4 second(vec2 uv)
{
    return textureLod(sampler2D(u_second, s_second), uv, 0.0) * inside(uv);
}

vec3 straight(vec4 premultiplied)
{
    return premultiplied.a > 0.0001 ? clamp(premultiplied.rgb / premultiplied.a, 0.0, 1.0) : vec3(0.0);
}

//==============================================================================
// a.xy: the step between two taps, a.z: the deviation in steps, a.w: the taps on each side

vec4 blur(vec2 uv)
{
    int count = int(clamp(params.a.w, 0.0, 64.0));
    float falloff = 0.5 / max(params.a.z * params.a.z, 0.000001);

    vec4 sum = first(uv);
    float total = 1.0;

    for (int i = 1; i <= count; ++i)
    {
        float tap = float(i);
        float weight = exp(-tap * tap * falloff);
        vec2 delta = params.a.xy * tap;

        sum += (first(uv + delta) + first(uv - delta)) * weight;
        total += 2.0 * weight;
    }

    return sum / total;
}

// a.xy: the step between two taps, a.z: above 0.5 to dilate, a.w: the taps on each side

vec4 morphology(vec2 uv)
{
    int count = int(clamp(params.a.w, 0.0, 64.0));
    bool dilate = params.a.z > 0.5;

    vec4 value = first(uv);

    for (int i = 1; i <= count; ++i)
    {
        vec2 delta = params.a.xy * float(i);
        vec4 ahead = first(uv + delta);
        vec4 behind = first(uv - delta);

        value = dilate ? max(value, max(ahead, behind)) : min(value, min(ahead, behind));
    }

    return value;
}

//==============================================================================
// a: k1, k2, k3 and k4 of the arithmetic operator

vec4 composite(vec4 top, vec4 bottom, int rule)
{
    if (rule == 1)
        return top * bottom.a;

    if (rule == 2)
        return top * (1.0 - bottom.a);

    if (rule == 3)
        return top * bottom.a + bottom * (1.0 - top.a);

    if (rule == 4)
        return top * (1.0 - bottom.a) + bottom * (1.0 - top.a);

    if (rule == 5)
        return min(top + bottom, vec4(1.0));

    if (rule == 6)
    {
        vec4 mixed = clamp(params.a.x * top * bottom + params.a.y * top + params.a.z * bottom + vec4(params.a.w), 0.0, 1.0);
        return vec4(min(mixed.rgb, vec3(mixed.a)), mixed.a);
    }

    return top + bottom * (1.0 - top.a);
}

//==============================================================================

float luminance(vec3 color)
{
    return dot(color, vec3(0.3, 0.59, 0.11));
}

vec3 clipColor(vec3 color)
{
    float light = luminance(color);
    float low = min(color.r, min(color.g, color.b));
    float high = max(color.r, max(color.g, color.b));

    if (low < 0.0)
        color = light + (color - light) * light / (light - low);

    if (high > 1.0)
        color = light + (color - light) * (1.0 - light) / (high - light);

    return color;
}

vec3 withLuminance(vec3 color, float light)
{
    return clipColor(color + (light - luminance(color)));
}

float saturation(vec3 color)
{
    return max(color.r, max(color.g, color.b)) - min(color.r, min(color.g, color.b));
}

vec3 withSaturation(vec3 color, float amount)
{
    float low = min(color.r, min(color.g, color.b));
    float high = max(color.r, max(color.g, color.b));

    return high > low ? (color - low) * amount / (high - low) : vec3(0.0);
}

vec3 hardLight(vec3 backdrop, vec3 color)
{
    return mix(2.0 * backdrop * color, 1.0 - 2.0 * (1.0 - backdrop) * (1.0 - color), step(0.5, color));
}

vec3 softLight(vec3 backdrop, vec3 color)
{
    vec3 curve = mix(((16.0 * backdrop - 12.0) * backdrop + 4.0) * backdrop, sqrt(backdrop), step(0.25, backdrop));

    return mix(backdrop - (1.0 - 2.0 * color) * backdrop * (1.0 - backdrop),
               backdrop + (2.0 * color - 1.0) * (curve - backdrop),
               step(0.5, color));
}

vec3 blendColors(vec3 backdrop, vec3 color, int mode)
{
    if (mode == 1)
        return backdrop + color - backdrop * color;

    if (mode == 2)
        return hardLight(color, backdrop);

    if (mode == 3)
        return min(backdrop, color);

    if (mode == 4)
        return max(backdrop, color);

    if (mode == 5)
        return min(vec3(1.0), backdrop / max(1.0 - color, vec3(0.0001)));

    if (mode == 6)
        return 1.0 - min(vec3(1.0), (1.0 - backdrop) / max(color, vec3(0.0001)));

    if (mode == 7)
        return hardLight(backdrop, color);

    if (mode == 8)
        return softLight(backdrop, color);

    if (mode == 9)
        return abs(backdrop - color);

    if (mode == 10)
        return backdrop + color - 2.0 * backdrop * color;

    if (mode == 11)
        return backdrop * color;

    if (mode == 12)
        return withLuminance(withSaturation(color, saturation(backdrop)), luminance(backdrop));

    if (mode == 13)
        return withLuminance(withSaturation(backdrop, saturation(color)), luminance(backdrop));

    if (mode == 14)
        return withLuminance(color, luminance(backdrop));

    if (mode == 15)
        return withLuminance(backdrop, luminance(color));

    return color;
}

vec4 blend(vec4 top, vec4 bottom, int mode)
{
    if (mode == 16)
        return min(top + bottom, vec4(1.0));

    vec3 mixed = clamp(blendColors(straight(bottom), straight(top), mode), 0.0, 1.0);
    vec3 color = (1.0 - bottom.a) * top.rgb + (1.0 - top.a) * bottom.rgb + top.a * bottom.a * mixed;

    return vec4(color, top.a + bottom.a - top.a * bottom.a);
}

//==============================================================================
// a.x: the height of the surface, a.y: the light it gives back, a.z: the specular exponent,
// b.xyz: where a distant light comes from, or where a point light is in texels, c.rgb: the light color.
// Variants: 0 diffuse and 1 specular with a distant light, 2 diffuse and 3 specular with a point light

float height(vec2 uv)
{
    return first(uv).a;
}

vec4 lighting(vec2 uv, int variant)
{
    vec2 texel = params.metrics.xy;

    float topLeft = height(uv + texel * vec2(-1.0, -1.0));
    float top = height(uv + texel * vec2(0.0, -1.0));
    float topRight = height(uv + texel * vec2(1.0, -1.0));
    float left = height(uv + texel * vec2(-1.0, 0.0));
    float right = height(uv + texel * vec2(1.0, 0.0));
    float bottomLeft = height(uv + texel * vec2(-1.0, 1.0));
    float bottom = height(uv + texel * vec2(0.0, 1.0));
    float bottomRight = height(uv + texel * vec2(1.0, 1.0));

    float slopeX = (topRight + 2.0 * right + bottomRight) - (topLeft + 2.0 * left + bottomLeft);
    float slopeY = (bottomLeft + 2.0 * bottom + bottomRight) - (topLeft + 2.0 * top + topRight);

    vec3 normal = normalize(vec3(-params.a.x * 0.25 * slopeX, -params.a.x * 0.25 * slopeY, 1.0));
    vec3 light = params.b.xyz;

    if (variant >= 2)
        light = normalize(params.b.xyz - vec3(uv / texel, params.a.x * height(uv)));

    if (variant == 0 || variant == 2)
        return vec4(clamp(params.a.y * max(dot(normal, light), 0.0) * params.c.rgb, 0.0, 1.0), 1.0);

    vec3 halfway = normalize(light + vec3(0.0, 0.0, 1.0));
    vec3 color = clamp(params.a.y * pow(max(dot(normal, halfway), 0.0), max(params.a.z, 1.0)) * params.c.rgb, 0.0, 1.0);

    return vec4(color, max(color.r, max(color.g, color.b)));
}

//==============================================================================
// a.xy: the largest move, b and c: the channels of the second picture moving along x and along y

vec4 displacement(vec2 uv)
{
    vec4 moves = second(uv);
    vec4 channels = vec4(straight(moves), moves.a);
    vec2 amount = vec2(dot(channels, params.b), dot(channels, params.c)) - vec2(0.5);

    return first(uv + params.a.xy * amount);
}

//==============================================================================

vec3 toLinear(vec3 color)
{
    return mix(color / 12.92, pow((color + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), color));
}

vec3 toSrgb(vec3 color)
{
    return mix(color * 12.92, 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), color));
}

//==============================================================================

void main()
{
    vec2 uv = gl_FragCoord.xy * params.metrics.xy;
    int operation = int(params.metrics.z + 0.5);
    int variant = int(params.metrics.w + 0.5);

    if (operation == 1)
    {
        fragColor = blur(uv);
    }
    else if (operation == 2)
    {
        fragColor = morphology(uv);
    }
    else if (operation == 3)
    {
        fragColor = first(uv - params.a.xy);
    }
    else if (operation == 4)
    {
        fragColor = params.a;
    }
    else if (operation == 5)
    {
        vec4 texel = first(uv);
        vec4 color = vec4(straight(texel), texel.a);
        vec4 changed = clamp(vec4(dot(params.row0, color), dot(params.row1, color), dot(params.row2, color), dot(params.row3, color)) + params.bias, 0.0, 1.0);

        fragColor = vec4(changed.rgb * changed.a, changed.a);
    }
    else if (operation == 6)
    {
        fragColor = composite(first(uv), second(uv), variant);
    }
    else if (operation == 7)
    {
        fragColor = blend(first(uv), second(uv), variant);
    }
    else if (operation == 8)
    {
        fragColor = lighting(uv, variant);
    }
    else if (operation == 9)
    {
        fragColor = displacement(uv);
    }
    else if (operation == 10)
    {
        // The second picture over the shadow the first one casts: a: its color, b.xy: how far it is moved
        vec4 top = second(uv);
        fragColor = top + params.a * first(uv - params.b.xy).a * (1.0 - top.a);
    }
    else if (operation == 11)
    {
        vec4 texel = first(uv);
        fragColor = vec4(toLinear(straight(texel)) * texel.a, texel.a);
    }
    else if (operation == 12)
    {
        vec4 texel = first(uv);
        fragColor = vec4(toSrgb(straight(texel)) * texel.a, texel.a);
    }
    else
    {
        fragColor = first(uv);
    }
}
