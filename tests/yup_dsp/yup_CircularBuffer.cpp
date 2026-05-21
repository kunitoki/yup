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

#include <yup_dsp/yup_dsp.h>

#include <gtest/gtest.h>

namespace yup::test
{

//==============================================================================
class CircularBufferTest : public ::testing::Test
{
};

//==============================================================================
TEST_F (CircularBufferTest, DefaultConstructionZeroInitialises)
{
    CircularBuffer<float, 4> buf;
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ (buf[i], 0.0f);
}

TEST_F (CircularBufferTest, CustomInitValueFillsBuffer)
{
    CircularBuffer<int, 3> buf (42);
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ (buf[i], 42);
}

TEST_F (CircularBufferTest, PushAdvancesWritePointerAndOldestIsAtZero)
{
    CircularBuffer<int, 4> buf;
    buf.push (1);
    buf.push (2);
    buf.push (3);
    buf.push (4);

    // After 4 pushes into a size-4 buffer, index 0 is the oldest (1) and 3 is newest (4)
    EXPECT_EQ (buf[0], 1);
    EXPECT_EQ (buf[1], 2);
    EXPECT_EQ (buf[2], 3);
    EXPECT_EQ (buf[3], 4);
}

TEST_F (CircularBufferTest, PushWrapsAroundCorrectly)
{
    CircularBuffer<int, 3> buf;
    buf.push (10);
    buf.push (20);
    buf.push (30);
    // Buffer full: [10, 20, 30], oldest=10 at index 0

    buf.push (40);
    // Oldest entry (10) overwritten: [20, 30, 40]
    EXPECT_EQ (buf[0], 20);
    EXPECT_EQ (buf[1], 30);
    EXPECT_EQ (buf[2], 40);
}

TEST_F (CircularBufferTest, PushMaintainsNewestAtHighestIndex)
{
    CircularBuffer<int, 5> buf;
    for (int i = 1; i <= 10; ++i)
        buf.push (i);

    // After 10 pushes into size-5: newest=10, oldest=6
    EXPECT_EQ (buf[4], 10);
    EXPECT_EQ (buf[3], 9);
    EXPECT_EQ (buf[2], 8);
    EXPECT_EQ (buf[1], 7);
    EXPECT_EQ (buf[0], 6);
}

TEST_F (CircularBufferTest, ClearResetsToZeroAndRewindsPointer)
{
    CircularBuffer<float, 4> buf;
    buf.push (1.0f);
    buf.push (2.0f);
    buf.push (3.0f);

    buf.clear();

    for (int i = 0; i < 4; ++i)
        EXPECT_EQ (buf[i], 0.0f);

    // After clear, push should start from logical index 0 again
    buf.push (7.0f);
    EXPECT_EQ (buf[0], 7.0f);
    for (int i = 1; i < 4; ++i)
        EXPECT_EQ (buf[i], 0.0f);
}

TEST_F (CircularBufferTest, WriteViaSubscriptOperator)
{
    CircularBuffer<int, 4> buf;
    buf.push (1);
    buf.push (2);
    buf.push (3);
    buf.push (4);

    buf[2] = 99;
    EXPECT_EQ (buf[2], 99);
    // Surrounding entries unchanged
    EXPECT_EQ (buf[1], 2);
    EXPECT_EQ (buf[3], 4);
}

TEST_F (CircularBufferTest, ConstAccessMatchesNonConst)
{
    CircularBuffer<float, 3> buf;
    buf.push (1.5f);
    buf.push (2.5f);
    buf.push (3.5f);

    const auto& constBuf = buf;
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ (constBuf[i], buf[i]);
}

TEST_F (CircularBufferTest, SizeOneFunctionsCorrectly)
{
    CircularBuffer<int, 1> buf;
    buf.push (5);
    EXPECT_EQ (buf[0], 5);

    buf.push (9);
    EXPECT_EQ (buf[0], 9);

    buf.clear();
    EXPECT_EQ (buf[0], 0);
}

TEST_F (CircularBufferTest, DoublePrecisionType)
{
    CircularBuffer<double, 4> buf;
    buf.push (1.1);
    buf.push (2.2);
    buf.push (3.3);
    buf.push (4.4);

    EXPECT_DOUBLE_EQ (buf[0], 1.1);
    EXPECT_DOUBLE_EQ (buf[3], 4.4);
}

} // namespace yup::test
