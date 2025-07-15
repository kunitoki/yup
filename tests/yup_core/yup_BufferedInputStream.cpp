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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

using namespace yup;

class BufferedInputStreamTests : public ::testing::Test
{
protected:
    template <typename Fn, size_t... Ix, typename Values>
    static void applyImpl (Fn&& fn, std::index_sequence<Ix...>, Values&& values)
    {
        fn (std::get<Ix> (values)...);
    }

    template <typename Fn, typename... Values>
    static void apply (Fn&& fn, std::tuple<Values...> values)
    {
        applyImpl (fn, std::make_index_sequence<sizeof...(Values)>(), values);
    }

    template <typename Fn, typename Values>
    static void allCombinationsImpl (Fn&& fn, Values&& values)
    {
        apply (fn, values);
    }

    template <typename Fn, typename Values, typename Range, typename... Ranges>
    static void allCombinationsImpl (Fn&& fn, Values&& values, Range&& range, Ranges&&... ranges)
    {
        for (auto& item : range)
            allCombinationsImpl (fn, std::tuple_cat (values, std::tie (item)), ranges...);
    }

    template <typename Fn, typename... Ranges>
    static void allCombinations (Fn&& fn, Ranges&&... ranges)
    {
        allCombinationsImpl (fn, std::tie(), ranges...);
    }

    void runTest (const MemoryBlock& data, const int readSize, const bool peek)
    {
        MemoryInputStream mi (data, true);

        BufferedInputStream stream (mi, jmin (200, (int) data.getSize()));

        EXPECT_EQ (stream.getPosition(), (int64) 0);
        EXPECT_EQ (stream.getTotalLength(), (int64) data.getSize());
        EXPECT_EQ (stream.getNumBytesRemaining(), stream.getTotalLength());
        EXPECT_FALSE (stream.isExhausted());

        size_t numBytesRead = 0;
        MemoryBlock readBuffer (data.getSize());

        while (numBytesRead < data.getSize())
        {
            if (peek)
                EXPECT_EQ (stream.peekByte(), *(char*) (data.begin() + numBytesRead));

            const auto startingPos = numBytesRead;
            numBytesRead += (size_t) stream.read (readBuffer.begin() + numBytesRead, readSize);

            EXPECT_TRUE (std::equal (readBuffer.begin() + startingPos,
                                     readBuffer.begin() + numBytesRead,
                                     data.begin() + startingPos,
                                     data.begin() + numBytesRead));
            EXPECT_EQ (stream.getPosition(), (int64) numBytesRead);
            EXPECT_EQ (stream.getNumBytesRemaining(), (int64) (data.getSize() - numBytesRead));
            EXPECT_EQ (stream.isExhausted(), (numBytesRead == data.getSize()));
        }

        EXPECT_EQ (stream.getPosition(), (int64) data.getSize());
        EXPECT_EQ (stream.getNumBytesRemaining(), (int64) 0);
        EXPECT_TRUE (stream.isExhausted());

        EXPECT_TRUE (readBuffer == data);

        // Skip test
        stream.setPosition (0);
        EXPECT_EQ (stream.getPosition(), (int64) 0);
        EXPECT_EQ (stream.getTotalLength(), (int64) data.getSize());
        EXPECT_EQ (stream.getNumBytesRemaining(), stream.getTotalLength());
        EXPECT_FALSE (stream.isExhausted());

        numBytesRead = 0;
        const int numBytesToSkip = 5;

        while (numBytesRead < data.getSize())
        {
            EXPECT_EQ (stream.peekByte(), *(char*) (data.begin() + numBytesRead));

            stream.skipNextBytes (numBytesToSkip);
            numBytesRead += numBytesToSkip;
            numBytesRead = std::min (numBytesRead, data.getSize());

            EXPECT_EQ (stream.getPosition(), (int64) numBytesRead);
            EXPECT_EQ (stream.getNumBytesRemaining(), (int64) (data.getSize() - numBytesRead));
            EXPECT_EQ (stream.isExhausted(), (numBytesRead == data.getSize()));
        }

        EXPECT_EQ (stream.getPosition(), (int64) data.getSize());
        EXPECT_EQ (stream.getNumBytesRemaining(), (int64) 0);
        EXPECT_TRUE (stream.isExhausted());
    }
};

TEST_F (BufferedInputStreamTests, ReadAndSkipCombinations)
{
    const MemoryBlock testBufferA ("abcdefghijklmnopqrstuvwxyz", 26);

    const auto testBufferB = []
    {
        MemoryBlock mb { 8192 };
        auto r = Random::getSystemRandom();

        std::for_each (mb.begin(), mb.end(), [&] (char& item)
        {
            item = (char) r.nextInt (std::numeric_limits<char>::max());
        });

        return mb;
    }();

    const MemoryBlock buffers[] { testBufferA, testBufferB };
    const int readSizes[] { 3, 10, 50 };
    const bool shouldPeek[] { false, true };

    allCombinations ([this] (const MemoryBlock& data, const int readSize, const bool peek)
    {
        runTest (data, readSize, peek);
    },
                     buffers,
                     readSizes,
                     shouldPeek);
}
