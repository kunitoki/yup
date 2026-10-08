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


#ifndef YUP_3D_LIB_SAMPLING
#define YUP_3D_LIB_SAMPLING

// Radical inverse of index in the given base: the low-discrepancy Halton sequence (Halton 1964).
// Index 0 gives 0, so sequences usually start from 1.
float yupHalton(int index, int base)
{
    float result = 0.0;
    float fraction = 1.0;

    for (int i = index; i > 0; i /= base)
    {
        fraction /= float(base);
        result += fraction * float(i % base);
    }

    return result;
}

// Per-pixel noise in 0..1 that dithers well over small neighborhoods (Jimenez 2014, "Next Generation
// Post Processing in Call of Duty: Advanced Warfare")
float yupInterleavedGradientNoise(vec2 pixel)
{
    return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

#endif
