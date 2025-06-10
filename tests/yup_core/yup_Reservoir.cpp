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

using namespace yup;

class ReservoirTest : public ::testing::Test
{
protected:
    struct MockBuffer
    {
        std::vector<int> data;
        Range<int> bufferedRange;

        MockBuffer()
            : bufferedRange (0, 0)
        {
        }

        void fillBuffer (int startPosition, const std::vector<int>& sourceData)
        {
            int bufferSize = 10; // Fixed buffer size for testing
            int actualStart = std::max (0, startPosition);
            int actualEnd = std::min (static_cast<int> (sourceData.size()), startPosition + bufferSize);

            data.clear();
            data.resize (bufferSize, 0);

            if (actualStart < actualEnd)
            {
                for (int i = actualStart; i < actualEnd; ++i)
                {
                    if (i - actualStart < bufferSize)
                        data[i - actualStart] = sourceData[i];
                }
                bufferedRange = Range<int> (actualStart, actualEnd);
            }
            else
            {
                bufferedRange = Range<int> (0, 0);
            }
        }

        Range<int> getBufferedRange() const
        {
            return bufferedRange;
        }

        void readFromBuffer (Range<int> range, std::vector<int>& output)
        {
            if (! bufferedRange.contains (range.getStart()) || ! bufferedRange.contains (range.getEnd() - 1))
                return;

            int startOffset = range.getStart() - bufferedRange.getStart();
            int length = range.getLength();

            for (int i = 0; i < length; ++i)
            {
                if (startOffset + i < static_cast<int> (data.size()))
                    output.push_back (data[startOffset + i]);
            }
        }
    };
};

TEST_F (ReservoirTest, BasicBufferedRead)
{
    std::vector<int> sourceData = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    MockBuffer buffer;
    std::vector<int> output;

    auto getBufferedRange = [&buffer]()
    {
        return buffer.getBufferedRange();
    };

    auto readFromReservoir = [&buffer, &output] (Range<int> range)
    {
        buffer.readFromBuffer (range, output);
    };

    auto fillReservoir = [&buffer, &sourceData] (int startPosition)
    {
        buffer.fillBuffer (startPosition, sourceData);
    };

    // Test reading a range that fits in one buffer
    Range<int> requestedRange (2, 7);
    auto remainingRange = Reservoir::doBufferedRead (requestedRange, getBufferedRange, readFromReservoir, fillReservoir);

    EXPECT_TRUE (remainingRange.isEmpty());
    EXPECT_EQ (output.size(), 5);
    EXPECT_EQ (output[0], 2);
    EXPECT_EQ (output[1], 3);
    EXPECT_EQ (output[2], 4);
    EXPECT_EQ (output[3], 5);
    EXPECT_EQ (output[4], 6);
}

TEST_F (ReservoirTest, MultipleBufferReads)
{
    std::vector<int> sourceData = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    MockBuffer buffer;
    std::vector<int> output;

    auto getBufferedRange = [&buffer]()
    {
        return buffer.getBufferedRange();
    };

    auto readFromReservoir = [&buffer, &output] (Range<int> range)
    {
        buffer.readFromBuffer (range, output);
    };

    auto fillReservoir = [&buffer, &sourceData] (int startPosition)
    {
        buffer.fillBuffer (startPosition, sourceData);
    };

    // Test reading a range that spans multiple buffers
    Range<int> requestedRange (5, 15);
    auto remainingRange = Reservoir::doBufferedRead (requestedRange, getBufferedRange, readFromReservoir, fillReservoir);

    EXPECT_TRUE (remainingRange.isEmpty());
    EXPECT_EQ (output.size(), 10);

    // Verify the data is correct
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ (output[i], i + 5);
    }
}

TEST_F (ReservoirTest, ReadBeyondDataEnd)
{
    std::vector<int> sourceData = { 0, 1, 2, 3, 4 };
    MockBuffer buffer;
    std::vector<int> output;

    auto getBufferedRange = [&buffer]()
    {
        return buffer.getBufferedRange();
    };

    auto readFromReservoir = [&buffer, &output] (Range<int> range)
    {
        buffer.readFromBuffer (range, output);
    };

    auto fillReservoir = [&buffer, &sourceData] (int startPosition)
    {
        buffer.fillBuffer (startPosition, sourceData);
    };

    // Test reading beyond the end of available data
    Range<int> requestedRange (3, 10);
    auto remainingRange = Reservoir::doBufferedRead (requestedRange, getBufferedRange, readFromReservoir, fillReservoir);

    // Should have some remaining range since we can't fulfill the entire request
    EXPECT_FALSE (remainingRange.isEmpty());
    EXPECT_EQ (remainingRange.getStart(), 5); // Should start from where the data ends

    // Should have read the available data
    EXPECT_EQ (output.size(), 2);
    EXPECT_EQ (output[0], 3);
    EXPECT_EQ (output[1], 4);
}

TEST_F (ReservoirTest, EmptyRange)
{
    std::vector<int> sourceData = { 0, 1, 2, 3, 4 };
    MockBuffer buffer;
    std::vector<int> output;

    auto getBufferedRange = [&buffer]()
    {
        return buffer.getBufferedRange();
    };

    auto readFromReservoir = [&buffer, &output] (Range<int> range)
    {
        buffer.readFromBuffer (range, output);
    };

    auto fillReservoir = [&buffer, &sourceData] (int startPosition)
    {
        buffer.fillBuffer (startPosition, sourceData);
    };

    // Test with empty range
    Range<int> requestedRange (3, 3);
    auto remainingRange = Reservoir::doBufferedRead (requestedRange, getBufferedRange, readFromReservoir, fillReservoir);

    EXPECT_TRUE (remainingRange.isEmpty());
    EXPECT_EQ (output.size(), 0);
}

TEST_F (ReservoirTest, FailedBufferFill)
{
    std::vector<int> sourceData = { 0, 1, 2 };
    MockBuffer buffer;
    std::vector<int> output;

    auto getBufferedRange = [&buffer]()
    {
        return buffer.getBufferedRange();
    };

    auto readFromReservoir = [&buffer, &output] (Range<int> range)
    {
        buffer.readFromBuffer (range, output);
    };

    // Simulate a fill failure by not updating the buffer
    auto fillReservoir = [] (int)
    {
        // Do nothing - simulate failure to fill
    };

    // Test reading when buffer fill fails
    Range<int> requestedRange (5, 10);
    auto remainingRange = Reservoir::doBufferedRead (requestedRange, getBufferedRange, readFromReservoir, fillReservoir);

    // Should return the entire requested range as remaining since fill failed
    EXPECT_EQ (remainingRange, requestedRange);
    EXPECT_EQ (output.size(), 0);
}

TEST_F (ReservoirTest, PartiallyAvailableBuffer)
{
    std::vector<int> sourceData = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    MockBuffer buffer;
    std::vector<int> output;

    // Pre-fill buffer with part of the data
    buffer.fillBuffer (2, sourceData);

    auto getBufferedRange = [&buffer]()
    {
        return buffer.getBufferedRange();
    };

    auto readFromReservoir = [&buffer, &output] (Range<int> range)
    {
        buffer.readFromBuffer (range, output);
    };

    auto fillReservoir = [&buffer, &sourceData] (int startPosition)
    {
        buffer.fillBuffer (startPosition, sourceData);
    };

    // Test reading from pre-filled buffer
    Range<int> requestedRange (4, 8);
    auto remainingRange = Reservoir::doBufferedRead (requestedRange, getBufferedRange, readFromReservoir, fillReservoir);

    EXPECT_TRUE (remainingRange.isEmpty());
    EXPECT_EQ (output.size(), 4);
    EXPECT_EQ (output[0], 4);
    EXPECT_EQ (output[1], 5);
    EXPECT_EQ (output[2], 6);
    EXPECT_EQ (output[3], 7);
}
