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
