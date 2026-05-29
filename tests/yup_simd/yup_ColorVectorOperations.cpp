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

#include <gtest/gtest.h>

#include <yup_simd/yup_simd.h>

using namespace yup;

TEST (ColorVectorOpsTests, PremultiplyARGBMatchesScalarReference)
{
    uint32_t pixels[] = { 0x80ff8040u, 0xff010203u, 0x00010203u };

    ColorVectorOperations::premultiplyARGB (pixels, 3);

    EXPECT_EQ (pixels[0], 0x80804020u);
    EXPECT_EQ (pixels[1], 0xff010203u);
    EXPECT_EQ (pixels[2], 0x00000000u);
}

TEST (ColorVectorOpsTests, PremultiplyRGBAMatchesScalarReference)
{
    uint8 pixels[] = { 255, 128, 64, 128, 1, 2, 3, 255, 1, 2, 3, 0 };

    ColorVectorOperations::premultiplyRGBA (pixels, 3);

    EXPECT_EQ (pixels[0], 128);
    EXPECT_EQ (pixels[1], 64);
    EXPECT_EQ (pixels[2], 32);
    EXPECT_EQ (pixels[3], 128);
    EXPECT_EQ (pixels[4], 1);
    EXPECT_EQ (pixels[5], 2);
    EXPECT_EQ (pixels[6], 3);
    EXPECT_EQ (pixels[7], 255);
    EXPECT_EQ (pixels[8], 0);
    EXPECT_EQ (pixels[9], 0);
    EXPECT_EQ (pixels[10], 0);
    EXPECT_EQ (pixels[11], 0);
}

TEST (ColorVectorOpsTests, ConvertARGBtoRGBA)
{
    const uint32_t argb[] = { 0x12345678u, 0xaabbccddu };
    uint32_t rgba[2] = {};

    ColorVectorOperations::convertARGBtoRGBA (argb, rgba, 2);

    EXPECT_EQ (rgba[0], 0x34567812u);
    EXPECT_EQ (rgba[1], 0xbbccddaau);
}

TEST (ColorVectorOpsTests, ConvertARGBtoRGBAInPlace)
{
    uint32_t pixels[] = { 0x12345678u, 0xaabbccddu };

    ColorVectorOperations::convertARGBtoRGBA (pixels, pixels, 2);

    EXPECT_EQ (pixels[0], 0x34567812u);
    EXPECT_EQ (pixels[1], 0xbbccddaau);
}

TEST (ColorVectorOpsTests, ConvertGrayscaleToRGBA)
{
    const uint8 gray[] = { 0, 127, 255 };
    uint8 rgba[12] = {};

    ColorVectorOperations::convertGrayscaleToRGBA (gray, rgba, 3);

    EXPECT_EQ (rgba[0], 0);
    EXPECT_EQ (rgba[1], 0);
    EXPECT_EQ (rgba[2], 0);
    EXPECT_EQ (rgba[3], 255);
    EXPECT_EQ (rgba[4], 127);
    EXPECT_EQ (rgba[5], 127);
    EXPECT_EQ (rgba[6], 127);
    EXPECT_EQ (rgba[7], 255);
    EXPECT_EQ (rgba[8], 255);
    EXPECT_EQ (rgba[9], 255);
    EXPECT_EQ (rgba[10], 255);
    EXPECT_EQ (rgba[11], 255);
}

TEST (ColorVectorOpsTests, ConvertRGBToRGBA)
{
    const uint8 rgb[] = { 1, 2, 3, 4, 5, 6 };
    uint8 rgba[8] = {};

    ColorVectorOperations::convertRGBToRGBA (rgb, rgba, 2);

    EXPECT_EQ (rgba[0], 1);
    EXPECT_EQ (rgba[1], 2);
    EXPECT_EQ (rgba[2], 3);
    EXPECT_EQ (rgba[3], 255);
    EXPECT_EQ (rgba[4], 4);
    EXPECT_EQ (rgba[5], 5);
    EXPECT_EQ (rgba[6], 6);
    EXPECT_EQ (rgba[7], 255);
}

TEST (ColorVectorOpsTests, LerpRowsMatchesScalarReference)
{
    const float rowA[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 0.5f, 0.0f, 1.0f };
    const float rowB[] = { 1.0f, 0.75f, 0.5f, 0.25f, 0.0f, 0.25f, 0.5f, 0.75f };
    float dst[8] = {};

    ColorVectorOperations::lerpRows (rowA, rowB, dst, 0.25f, 2);

    for (int i = 0; i < 8; ++i)
        EXPECT_NEAR (dst[i], rowA[i] + (rowB[i] - rowA[i]) * 0.25f, 1.0e-5f);
}
