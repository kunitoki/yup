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

#include <juce_core/juce_core.h>

using namespace juce;

TEST (GZIPDecompressorInputStreamTests, Read)
{
    const MemoryBlock data ("abcdefghijklmnopqrstuvwxyz", 26);

    MemoryOutputStream mo;
    GZIPCompressorOutputStream gzipOutputStream (mo);
    gzipOutputStream.write (data.getData(), data.getSize());
    gzipOutputStream.flush();

    MemoryInputStream mi (mo.getData(), mo.getDataSize(), false);
    GZIPDecompressorInputStream stream (&mi, false, GZIPDecompressorInputStream::zlibFormat, (int64) data.getSize());

    EXPECT_EQ (stream.getPosition(), (int64) 0);
    EXPECT_EQ (stream.getTotalLength(), (int64) data.getSize());
    EXPECT_EQ (stream.getNumBytesRemaining(), stream.getTotalLength());
    EXPECT_FALSE (stream.isExhausted());

    size_t numBytesRead = 0;
    MemoryBlock readBuffer (data.getSize());

    while (numBytesRead < data.getSize())
    {
        numBytesRead += (size_t) stream.read (&readBuffer[numBytesRead], 3);

        EXPECT_EQ (stream.getPosition(), (int64) numBytesRead);
        EXPECT_EQ (stream.getNumBytesRemaining(), (int64) (data.getSize() - numBytesRead));
        EXPECT_EQ (stream.isExhausted(), (numBytesRead == data.getSize()));
    }

    EXPECT_EQ (stream.getPosition(), (int64) data.getSize());
    EXPECT_EQ (stream.getNumBytesRemaining(), (int64) 0);
    EXPECT_TRUE (stream.isExhausted());

    EXPECT_TRUE (readBuffer == data);
}

TEST (GZIPDecompressorInputStreamTests, Skip)
{
    const MemoryBlock data ("abcdefghijklmnopqrstuvwxyz", 26);

    MemoryOutputStream mo;
    GZIPCompressorOutputStream gzipOutputStream (mo);
    gzipOutputStream.write (data.getData(), data.getSize());
    gzipOutputStream.flush();

    MemoryInputStream mi (mo.getData(), mo.getDataSize(), false);
    GZIPDecompressorInputStream stream (&mi, false, GZIPDecompressorInputStream::zlibFormat, (int64) data.getSize());

    stream.setPosition (0);
    EXPECT_EQ (stream.getPosition(), (int64) 0);
    EXPECT_EQ (stream.getTotalLength(), (int64) data.getSize());
    EXPECT_EQ (stream.getNumBytesRemaining(), stream.getTotalLength());
    EXPECT_FALSE (stream.isExhausted());

    size_t numBytesRead = 0;
    const int numBytesToSkip = 5;

    while (numBytesRead < data.getSize())
    {
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
