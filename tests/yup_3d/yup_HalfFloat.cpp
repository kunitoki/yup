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

#include <yup_3d/yup_3d.h>

#include <gtest/gtest.h>

using namespace yup;

class HalfFloatTests : public ::testing::Test
{
};

TEST_F (HalfFloatTests, ConvertsExactValues)
{
    EXPECT_EQ (floatToHalf (0.0f), 0x0000);
    EXPECT_EQ (floatToHalf (-0.0f), 0x8000);
    EXPECT_EQ (floatToHalf (1.0f), 0x3c00);
    EXPECT_EQ (floatToHalf (0.5f), 0x3800);
    EXPECT_EQ (floatToHalf (-2.0f), 0xc000);
    EXPECT_EQ (floatToHalf (65504.0f), 0x7bff);
    EXPECT_EQ (floatToHalf (6.103515625e-5f), 0x0400); // The smallest normal half
}

TEST_F (HalfFloatTests, RoundsToNearest)
{
    EXPECT_EQ (floatToHalf (1.0f + 1.0f / 4096.0f), 0x3c00); // A quarter of a half step down
    EXPECT_EQ (floatToHalf (1.0f + 3.0f / 4096.0f), 0x3c01); // Three quarters up
}

TEST_F (HalfFloatTests, FlushesSubnormalsToZero)
{
    EXPECT_EQ (floatToHalf (1.0e-5f), 0x0000);
    EXPECT_EQ (floatToHalf (-1.0e-5f), 0x8000);
}

TEST_F (HalfFloatTests, ClampsOverflowToTheLargestFiniteHalf)
{
    EXPECT_EQ (floatToHalf (70000.0f), 0x7bff);
    EXPECT_EQ (floatToHalf (std::numeric_limits<float>::infinity()), 0x7bff);
    EXPECT_EQ (floatToHalf (-std::numeric_limits<float>::infinity()), 0xfbff);
    EXPECT_EQ (floatToHalf (std::numeric_limits<float>::quiet_NaN()), 0x7bff);
}

TEST_F (HalfFloatTests, EveryFiniteHalfConvertsBack)
{
    EXPECT_EQ (halfToFloat (0x3c00), 1.0f);
    EXPECT_EQ (halfToFloat (0xc000), -2.0f);
    EXPECT_EQ (halfToFloat (0x7bff), 65504.0f);
    EXPECT_EQ (halfToFloat (0x0001), std::ldexp (1.0f, -24)); // The smallest subnormal half

    // floatToHalf flushes subnormals, so every normal half and both zeros round-trip
    for (uint32 bits = 0; bits < 0x10000u; ++bits)
    {
        const auto exponent = (bits >> 10) & 0x1fu;
        if (exponent == 0x1fu || (exponent == 0 && (bits & 0x3ffu) != 0))
            continue;

        EXPECT_EQ (floatToHalf (halfToFloat (static_cast<uint16> (bits))), bits) << bits;
    }
}

TEST_F (HalfFloatTests, RgbmKeepsTheColorWithinItsPrecision)
{
    const auto decode = [] (const uint8* rgbm, int channel)
    {
        return static_cast<float> (rgbm[channel]) / 255.0f * (static_cast<float> (rgbm[3]) / 255.0f * rgbmRange);
    };

    for (const auto& rgb : { std::array<float, 3> { 0.5f, 0.25f, 1.0f }, std::array<float, 3> { 7.0f, 0.1f, 3.0f }, std::array<float, 3> { 0.01f, 0.02f, 0.005f } })
    {
        uint8 rgbm[4] {};
        encodeRgbm (rgb.data(), rgbm);

        const auto largest = jmax (rgb[0], rgb[1], rgb[2]);
        for (int c = 0; c < 3; ++c)
            EXPECT_NEAR (decode (rgbm, c), rgb[static_cast<size_t> (c)], largest / 255.0f) << c;
    }

    // Black keeps the smallest multiplier, and the range clamps
    uint8 black[4] {};
    const float zero[] = { 0.0f, 0.0f, 0.0f };
    encodeRgbm (zero, black);
    EXPECT_EQ (black[0], 0);
    EXPECT_EQ (black[3], 1);

    uint8 bright[4] {};
    const float tooBright[] = { 100.0f, 0.0f, 0.0f };
    encodeRgbm (tooBright, bright);
    EXPECT_EQ (bright[0], 255);
    EXPECT_EQ (bright[3], 255);
}
