/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include <yup_core/yup_core.h>

#include <gtest/gtest.h>
#include <vector>
#include <array>

using namespace yup;

class SpanTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        testData = { 1, 2, 3, 4, 5 };
        cStyleArray[0] = 10;
        cStyleArray[1] = 20;
        cStyleArray[2] = 30;
        cStyleArray[3] = 40;
        cStyleArray[4] = 50;
    }

    std::vector<int> testData;
    int cStyleArray[5];
    std::array<int, 3> stdArray { { 100, 200, 300 } };
};

TEST_F (SpanTest, DefaultConstruction)
{
    Span<int> emptySpan;

    EXPECT_EQ (emptySpan.size(), 0);
    EXPECT_TRUE (emptySpan.empty());
    EXPECT_EQ (emptySpan.data(), nullptr);
    EXPECT_EQ (emptySpan.begin(), emptySpan.end());
}

TEST_F (SpanTest, ConstructFromIteratorAndSize)
{
    Span<int> span (testData.data(), testData.size());

    EXPECT_EQ (span.size(), 5);
    EXPECT_FALSE (span.empty());
    EXPECT_EQ (span.data(), testData.data());
    EXPECT_EQ (span[0], 1);
    EXPECT_EQ (span[4], 5);
}

TEST_F (SpanTest, ConstructFromVector)
{
    Span<int> span (testData);

    EXPECT_EQ (span.size(), testData.size());
    EXPECT_EQ (span.data(), testData.data());

    for (size_t i = 0; i < span.size(); ++i)
    {
        EXPECT_EQ (span[i], testData[i]);
    }
}

TEST_F (SpanTest, ConstructFromConstVector)
{
    const std::vector<int>& constData = testData;
    Span<const int> span (constData);

    EXPECT_EQ (span.size(), constData.size());
    EXPECT_EQ (span.data(), constData.data());

    for (size_t i = 0; i < span.size(); ++i)
    {
        EXPECT_EQ (span[i], constData[i]);
    }
}

TEST_F (SpanTest, ConstructFromCStyleArray)
{
    Span<int> span (cStyleArray);

    EXPECT_EQ (span.size(), 5);
    EXPECT_EQ (span.data(), cStyleArray);
    EXPECT_EQ (span[0], 10);
    EXPECT_EQ (span[4], 50);
}

TEST_F (SpanTest, ConstructFromStdArray)
{
    Span<int> span (stdArray);

    EXPECT_EQ (span.size(), 3);
    EXPECT_EQ (span.data(), stdArray.data());
    EXPECT_EQ (span[0], 100);
    EXPECT_EQ (span[2], 300);
}

TEST_F (SpanTest, ConstructFromConstStdArray)
{
    const std::array<int, 3>& constArray = stdArray;
    Span<const int> span (constArray);

    EXPECT_EQ (span.size(), 3);
    EXPECT_EQ (span.data(), constArray.data());
    EXPECT_EQ (span[0], 100);
    EXPECT_EQ (span[2], 300);
}

TEST_F (SpanTest, CopyConstruction)
{
    Span<int> original (testData);
    Span<int> copy (original);

    EXPECT_EQ (copy.size(), original.size());
    EXPECT_EQ (copy.data(), original.data());

    for (size_t i = 0; i < copy.size(); ++i)
    {
        EXPECT_EQ (copy[i], original[i]);
    }
}

TEST_F (SpanTest, CopyAssignment)
{
    Span<int> original (testData);
    Span<int> assigned;

    assigned = original;

    EXPECT_EQ (assigned.size(), original.size());
    EXPECT_EQ (assigned.data(), original.data());

    for (size_t i = 0; i < assigned.size(); ++i)
    {
        EXPECT_EQ (assigned[i], original[i]);
    }
}

TEST_F (SpanTest, MoveConstruction)
{
    Span<int> original (testData);
    auto originalData = original.data();
    auto originalSize = original.size();

    Span<int> moved (std::move (original));

    EXPECT_EQ (moved.size(), originalSize);
    EXPECT_EQ (moved.data(), originalData);
}

TEST_F (SpanTest, MoveAssignment)
{
    Span<int> original (testData);
    auto originalData = original.data();
    auto originalSize = original.size();

    Span<int> assigned;
    assigned = std::move (original);

    EXPECT_EQ (assigned.size(), originalSize);
    EXPECT_EQ (assigned.data(), originalData);
}

TEST_F (SpanTest, BeginEndIterators)
{
    Span<int> span (testData);

    EXPECT_EQ (span.begin(), testData.data());
    EXPECT_EQ (span.end(), testData.data() + testData.size());

    // Test iterator compatibility
    std::vector<int> copy (span.begin(), span.end());
    EXPECT_EQ (copy, testData);
}

TEST_F (SpanTest, FrontBack)
{
    Span<int> span (testData);

    EXPECT_EQ (span.front(), testData.front());
    EXPECT_EQ (span.back(), testData.back());

    // Modify through span
    span.front() = 99;
    span.back() = 88;

    EXPECT_EQ (testData[0], 99);
    EXPECT_EQ (testData[4], 88);
}

TEST_F (SpanTest, IndexAccess)
{
    Span<int> span (testData);

    for (size_t i = 0; i < span.size(); ++i)
    {
        EXPECT_EQ (span[i], testData[i]);
    }

    // Test modification through span
    span[2] = 999;
    EXPECT_EQ (testData[2], 999);
}

TEST_F (SpanTest, RangeBasedFor)
{
    Span<int> span (testData);

    int index = 0;
    for (const auto& value : span)
    {
        EXPECT_EQ (value, testData[index++]);
    }

    // Test modification
    for (auto& value : span)
    {
        value *= 2;
    }

    for (size_t i = 0; i < testData.size(); ++i)
    {
        EXPECT_EQ (testData[i], (i + 1) * 2); // Original values were 1,2,3,4,5
    }
}

TEST_F (SpanTest, FixedSizeSpan)
{
    Span<int, 3> fixedSpan (stdArray);

    EXPECT_EQ (fixedSpan.size(), 3);
    EXPECT_EQ (fixedSpan.extent, 3);
    EXPECT_FALSE (fixedSpan.empty());

    for (size_t i = 0; i < 3; ++i)
    {
        EXPECT_EQ (fixedSpan[i], stdArray[i]);
    }
}

TEST_F (SpanTest, ZeroSizeFixedSpan)
{
    Span<int, 0> zeroSpan;

    EXPECT_EQ (zeroSpan.size(), 0);
    EXPECT_EQ (zeroSpan.extent, 0);
    EXPECT_TRUE (zeroSpan.empty());
    EXPECT_EQ (zeroSpan.begin(), zeroSpan.end());
}

TEST_F (SpanTest, SpanFromSubrange)
{
    Span<int> fullSpan (testData);

    // Create a span from part of the data
    Span<int> subSpan (testData.data() + 1, 3);

    EXPECT_EQ (subSpan.size(), 3);
    EXPECT_EQ (subSpan[0], 2);
    EXPECT_EQ (subSpan[1], 3);
    EXPECT_EQ (subSpan[2], 4);
}

TEST_F (SpanTest, ConstSpanFromNonConstData)
{
    Span<const int> constSpan (testData);

    EXPECT_EQ (constSpan.size(), testData.size());
    EXPECT_EQ (constSpan.data(), testData.data());

    // Verify it's read-only (this should compile)
    int value = constSpan[0];
    EXPECT_EQ (value, testData[0]);
}

TEST_F (SpanTest, EmptySpanProperties)
{
    std::vector<int> emptyVector;
    Span<int> emptySpan (emptyVector);

    EXPECT_TRUE (emptySpan.empty());
    EXPECT_EQ (emptySpan.size(), 0);
    EXPECT_EQ (emptySpan.begin(), emptySpan.end());
}

TEST_F (SpanTest, SingleElementSpan)
{
    int singleValue = 42;
    Span<int> singleSpan (&singleValue, 1);

    EXPECT_EQ (singleSpan.size(), 1);
    EXPECT_FALSE (singleSpan.empty());
    EXPECT_EQ (singleSpan[0], 42);
    EXPECT_EQ (singleSpan.front(), 42);
    EXPECT_EQ (singleSpan.back(), 42);
}

TEST_F (SpanTest, DynamicExtentConstant)
{
    // Test that the dynamic extent constant is properly defined
    constexpr auto maxValue = std::numeric_limits<size_t>::max();
    static_assert (dynamicExtent == maxValue);

    // Test default template parameter
    Span<int> defaultSpan;
    static_assert (defaultSpan.extent == dynamicExtent);
}
