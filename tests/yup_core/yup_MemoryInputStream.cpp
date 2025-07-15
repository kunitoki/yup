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

namespace
{
String createRandomWideCharString (Random& r)
{
    yup_wchar buffer[50] = { 0 };

    for (int i = 0; i < numElementsInArray (buffer) - 1; ++i)
    {
        if (r.nextBool())
        {
            do
            {
                buffer[i] = (yup_wchar) (1 + r.nextInt (0x10ffff - 1));
            } while (! CharPointer_UTF16::canRepresent (buffer[i]));
        }
        else
            buffer[i] = (yup_wchar) (1 + r.nextInt (0xff));
    }

    return CharPointer_UTF32 (buffer);
}
} // namespace

TEST (MemoryInputStreamTests, Basics)
{
    Random r = Random::getSystemRandom();

    int randomInt = r.nextInt();
    int64 randomInt64 = r.nextInt64();
    double randomDouble = r.nextDouble();
    String randomString (createRandomWideCharString (r));

    MemoryOutputStream mo;
    mo.writeInt (randomInt);
    mo.writeIntBigEndian (randomInt);
    mo.writeCompressedInt (randomInt);
    mo.writeString (randomString);
    mo.writeInt64 (randomInt64);
    mo.writeInt64BigEndian (randomInt64);
    mo.writeDouble (randomDouble);
    mo.writeDoubleBigEndian (randomDouble);

    MemoryInputStream mi (mo.getData(), mo.getDataSize(), false);
    EXPECT_EQ (mi.readInt(), randomInt);
    EXPECT_EQ (mi.readIntBigEndian(), randomInt);
    EXPECT_EQ (mi.readCompressedInt(), randomInt);
    EXPECT_EQ (mi.readString(), randomString);
    EXPECT_EQ (mi.readInt64(), randomInt64);
    EXPECT_EQ (mi.readInt64BigEndian(), randomInt64);
    EXPECT_EQ (mi.readDouble(), randomDouble);
    EXPECT_EQ (mi.readDoubleBigEndian(), randomDouble);
}

TEST (MemoryInputStreamTests, Read)
{
    const MemoryBlock data ("abcdefghijklmnopqrstuvwxyz", 26);
    MemoryInputStream stream (data, true);

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

TEST (MemoryInputStreamTests, Skip)
{
    const MemoryBlock data ("abcdefghijklmnopqrstuvwxyz", 26);
    MemoryInputStream stream (data, true);

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
