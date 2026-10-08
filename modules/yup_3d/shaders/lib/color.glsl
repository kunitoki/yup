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

#ifndef YUP_3D_LIB_COLOR
#define YUP_3D_LIB_COLOR

// Color encodings and tone mapping. Tone mapping takes exposed scene-linear Rec.709 and returns
// display-linear values in 0..1, which are sRGB encoded afterwards.

vec3 yupSrgbToLinear(vec3 c)
{
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));
}

vec3 yupLinearToSrgb(vec3 c)
{
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), c));
}

// Rec.709 luminance weights
float yupLuma(vec3 c)
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec3 yupToneMapReinhard(vec3 c)
{
    return c / (vec3(1.0) + c);
}

// Rational fit of the ACES filmic curve (Narkowicz 2016, "ACES Filmic Tone Mapping Curve")
vec3 yupToneMapAces(vec3 x)
{
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

// AgX-style tone mapping (after Troy Sobotka's AgX): an inset of the primaries, a per-channel
// sigmoid over a log2 range around mid grey, then an outset. https://github.com/sobotka/AgX carries
// no license (checked 2026-10-08), so none of its values are used: the matrices, the -5..+6 stop
// range and the polynomial are our own fit, derived and documented in tools/agx_fit.py. No Blender
// or OCIO source or configuration was consulted.
vec3 yupToneMapAgx(vec3 c)
{
    // Columns: Rec.709 primaries moved away from D65 and rotated, then partly moved back
    const mat3 inset = mat3(vec3(0.83947207, 0.02293524, 0.04314896),
                            vec3(0.03456180, 0.94392547, 0.05587855),
                            vec3(0.12596613, 0.03313929, 0.90097249));
    const mat3 outset = mat3(vec3(1.12753300, -0.00651416, -0.03292859),
                             vec3(-0.00675187, 1.00599316, -0.03460768),
                             vec3(-0.12078112, 0.00052100, 1.06753627));

    vec3 shaped = inset * max(c, vec3(0.0));
    vec3 x = clamp((log2(max(shaped, vec3(1.0e-10)) / 0.18) + 5.0) / 11.0, 0.0, 1.0);

    // sRGB encoded sigmoid, contrast 1.6 in the log domain with mid grey kept at 0.18
    vec3 y = -4.6886051 * x + 19.31242;
    y = y * x - 28.204515;
    y = y * x + 16.346045;
    y = y * x - 2.4249506;
    y = y * x + 0.66635377;
    y = y * x - 0.0046820379;

    return clamp(outset * yupSrgbToLinear(clamp(y, 0.0, 1.0)), 0.0, 1.0);
}

// Operator 0 Reinhard, 1 ACES, 2 AgX
vec3 yupToneMap(vec3 color, float op)
{
    if (op < 0.5)
        return yupToneMapReinhard(color);

    if (op < 1.5)
        return yupToneMapAces(color);

    return yupToneMapAgx(color);
}

// Invertible luminance-weighted compression, so resolves and filters don't let fireflies dominate
// (Karis 2014, "High Quality Temporal Supersampling")
vec3 yupReversibleToneMap(vec3 c)
{
    return c / (1.0 + yupLuma(c));
}

// Clamped so a luma at or above 1 (filtering overshoot) stays finite
vec3 yupReversibleToneMapInverse(vec3 c)
{
    return c / max(1.0 - yupLuma(c), 1.0e-4);
}

// The inverse of a reversibly tone mapped color premultiplied by its coverage alpha, such as an
// MSAA resolve over a transparent clear: un-premultiplied first, so the inverse sees the color of
// the surface rather than of its coverage, then premultiplied again. Uncovered texels are black
vec3 yupUnpremultiplyInverse(vec4 c)
{
    return c.a > 1.0e-4 ? yupReversibleToneMapInverse(c.rgb / c.a) * c.a : vec3(0.0);
}

const float yupMaxHalf = 65504.0;

// Tests the exponent bits, which fast math cannot fold away like isnan and isinf
bvec4 yupIsNonFinite(vec4 c)
{
    return equal(floatBitsToUint(c) & uvec4(0x7f800000u), uvec4(0x7f800000u));
}

// NaN and -infinity become 0 and +infinity the largest half float, with rgb kept within the half
// float range and alpha within 0..1, so one bad texel can't spread through a filter
vec4 yupSanitize(vec4 c)
{
    vec4 replacement = mix(vec4(0.0), vec4(yupMaxHalf), equal(floatBitsToUint(c), uvec4(0x7f800000u)));
    return clamp(mix(c, replacement, yupIsNonFinite(c)), vec4(0.0), vec4(yupMaxHalf, yupMaxHalf, yupMaxHalf, 1.0));
}

#ifdef YUP_3D_RGBM
#define YUP_3D_RGBM_RANGE 8.0

// HDR color in an rgba8 target: rgb scaled by a shared multiplier stored in alpha, up to 8
vec4 yupEncodeRgbm(vec3 c)
{
    vec3 scaled = c / YUP_3D_RGBM_RANGE;
    float m = clamp(max(max(scaled.r, scaled.g), max(scaled.b, 1.0e-6)), 0.0, 1.0);
    m = ceil(m * 255.0) / 255.0;
    return vec4(clamp(scaled / m, 0.0, 1.0), m);
}

vec3 yupDecodeRgbm(vec4 rgbm)
{
    return rgbm.rgb * (rgbm.a * YUP_3D_RGBM_RANGE);
}
#endif

#endif
