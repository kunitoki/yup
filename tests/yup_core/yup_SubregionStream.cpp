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

TEST (SubregionInputStreamTests, Read)
{
    const MemoryBlock data ("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz", 52);
    MemoryInputStream mi (data, true);

    const int offset = Random::getSystemRandom().nextInt ((int) data.getSize());
    const size_t subregionSize = data.getSize() - (size_t) offset;

    SubregionStream stream (&mi, offset, (int) subregionSize, false);

    EXPECT_EQ (stream.getPosition(), (int64) 0);
    EXPECT_EQ (stream.getTotalLength(), (int64) subregionSize);
    EXPECT_EQ (stream.getNumBytesRemaining(), stream.getTotalLength());
    EXPECT_FALSE (stream.isExhausted());

    size_t numBytesRead = 0;
    MemoryBlock readBuffer (subregionSize);

    while (numBytesRead < subregionSize)
    {
        numBytesRead += (size_t) stream.read (&readBuffer[numBytesRead], 3);

        EXPECT_EQ (stream.getPosition(), (int64) numBytesRead);
        EXPECT_EQ (stream.getNumBytesRemaining(), (int64) (subregionSize - numBytesRead));
        EXPECT_EQ (stream.isExhausted(), (numBytesRead == subregionSize));
    }

    EXPECT_EQ (stream.getPosition(), (int64) subregionSize);
    EXPECT_EQ (stream.getNumBytesRemaining(), (int64) 0);
    EXPECT_TRUE (stream.isExhausted());

    const MemoryBlock memoryBlockToCheck (data.begin() + (size_t) offset, data.getSize() - (size_t) offset);
    EXPECT_TRUE (readBuffer == memoryBlockToCheck);
}

TEST (SubregionInputStreamTests, Skip)
{
    const MemoryBlock data ("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz", 52);
    MemoryInputStream mi (data, true);

    const int offset = Random::getSystemRandom().nextInt ((int) data.getSize());
    const size_t subregionSize = data.getSize() - (size_t) offset;

    SubregionStream stream (&mi, offset, (int) subregionSize, false);

    stream.setPosition (0);
    EXPECT_EQ (stream.getPosition(), (int64) 0);
    EXPECT_EQ (stream.getTotalLength(), (int64) subregionSize);
    EXPECT_EQ (stream.getNumBytesRemaining(), stream.getTotalLength());
    EXPECT_FALSE (stream.isExhausted());

    size_t numBytesRead = 0;
    const int64 numBytesToSkip = 5;

    while (numBytesRead < subregionSize)
    {
        stream.skipNextBytes (numBytesToSkip);
        numBytesRead += numBytesToSkip;
        numBytesRead = std::min (numBytesRead, subregionSize);

        EXPECT_EQ (stream.getPosition(), (int64) numBytesRead);
        EXPECT_EQ (stream.getNumBytesRemaining(), (int64) (subregionSize - numBytesRead));
        EXPECT_EQ (stream.isExhausted(), (numBytesRead == subregionSize));
    }

    EXPECT_EQ (stream.getPosition(), (int64) subregionSize);
    EXPECT_EQ (stream.getNumBytesRemaining(), (int64) 0);
    EXPECT_TRUE (stream.isExhausted());
}
