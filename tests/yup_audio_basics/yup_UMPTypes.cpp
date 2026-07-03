/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <yup_audio_basics/yup_audio_basics.h>

#include <gtest/gtest.h>

using namespace yup;
using namespace yup::ump;

TEST (UMPTypesTests, DownsampleMinCenterMax)
{
    EXPECT_EQ (downsample16To7Bit (0), 0u);
    EXPECT_EQ (downsample32To7Bit (0), 0u);
    EXPECT_EQ (downsample32To14Bit (0), 0u);

    EXPECT_EQ (downsample16To7Bit (0x8000), 0x40u);
    EXPECT_EQ (downsample32To7Bit (0x80000000u), 0x40u);
    EXPECT_EQ (downsample32To14Bit (0x80000000u), 0x2000u);

    EXPECT_EQ (downsample16To7Bit (0xffff), 0x7fu);
    EXPECT_EQ (downsample32To7Bit (0xffffffffu), 0x7fu);
    EXPECT_EQ (downsample32To14Bit (0xffffffffu), 0x3fffu);
}

TEST (UMPTypesTests, UpsampleMinCenterMax)
{
    EXPECT_EQ (upsample7To16Bit (0), 0u);
    EXPECT_EQ (upsample7To32Bit (0), 0u);
    EXPECT_EQ (upsample14To32Bit (0), 0u);

    EXPECT_EQ (upsample7To16Bit (0x40), 0x8000u);
    EXPECT_EQ (upsample7To32Bit (0x40), 0x80000000u);
    EXPECT_EQ (upsample14To32Bit (0x2000), 0x80000000u);

    EXPECT_EQ (upsample7To16Bit (0x7f), 0xffffu);
    EXPECT_EQ (upsample7To32Bit (0x7f), 0xffffffffu);
    EXPECT_EQ (upsample14To32Bit (0x3fff), 0xffffffffu);
}

TEST (UMPTypesTests, PreserveConversions)
{
    for (uint7_t v = 0u; v < 0x80; ++v)
        EXPECT_EQ (v, downsample16To7Bit (upsample7To16Bit (v)));

    for (uint7_t v = 0u; v < 0x80; ++v)
        EXPECT_EQ (v, downsample32To7Bit (upsample7To32Bit (v)));

    for (uint14_t v = 0u; v < 0x4000; ++v)
        EXPECT_EQ (v, downsample32To14Bit (upsample14To32Bit (v)));
}

TEST (UMPTypesTests, UpsampleXToYBit)
{
    for (uint7_t v = 0u; v < 0x80; ++v)
        EXPECT_EQ (upsample7To16Bit (v), upsampleXToYBit (v, 7, 16));

    for (uint7_t v = 0u; v < 0x80; ++v)
        EXPECT_EQ (upsample7To32Bit (v), upsampleXToYBit (v, 7, 32));

    for (uint14_t v = 0u; v < 0x4000; ++v)
        EXPECT_EQ (upsample14To32Bit (v), upsampleXToYBit (v, 14, 32));
}

TEST (UMPTypesTests, DownsampleMonotonicallyIncreasing)
{
    auto prev = downsample16To7Bit (static_cast<uint16_t> (0));
    for (uint16_t v = 1; v < 0xffffu; ++v)
    {
        const auto downsampled = downsample16To7Bit (v);
        EXPECT_GE (downsampled, prev);
        prev = downsampled;
    }
}

TEST (UMPTypesTests, UpsampleMonotonicallyIncreasing)
{
    auto prev = upsample7To16Bit (static_cast<uint7_t> (0));
    for (uint7_t v = 1u; v < 0x80; ++v)
    {
        const auto upsampled = upsample7To16Bit (v);
        EXPECT_GE (upsampled, prev);
        prev = upsampled;
    }
}

TEST (UMPTypesTests, SpotCheckIntermediateDownsampleValues)
{
    // Quarter and three-quarter values should be monotonically between min and center
    const auto quarter7bit = downsample16To7Bit (0x4000);
    const auto threeQuarter7bit = downsample16To7Bit (0xC000);

    EXPECT_GT (quarter7bit, 0u);
    EXPECT_LT (quarter7bit, 0x40u);
    EXPECT_GT (threeQuarter7bit, 0x40u);
    EXPECT_LT (threeQuarter7bit, 0x7fu);
}
