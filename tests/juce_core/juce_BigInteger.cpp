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

TEST (BigIntegerTests, BasicTests)
{
    auto getBigRandom = [] (Random& r)
    {
        BigInteger b;

        while (b < 2)
            r.fillBitsRandomly (b, 0, r.nextInt (150) + 1);

        return b;
    };

    Random r = getRandom();

    EXPECT_TRUE (BigInteger().isZero());
    EXPECT_TRUE (BigInteger (1).isOne());

    for (int j = 10000; --j >= 0;)
    {
        BigInteger b1 (getBigRandom (r)), b2 (getBigRandom (r));

        BigInteger b3 = b1 + b2;
        EXPECT_TRUE (b3 > b1 && b3 > b2);
        EXPECT_TRUE (b3 - b1 == b2);
        EXPECT_TRUE (b3 - b2 == b1);

        BigInteger b4 = b1 * b2;
        EXPECT_TRUE (b4 > b1 && b4 > b2);
        EXPECT_TRUE (b4 / b1 == b2);
        EXPECT_TRUE (b4 / b2 == b1);
        EXPECT_TRUE (((b4 << 1) >> 1) == b4);
        EXPECT_TRUE (((b4 << 10) >> 10) == b4);
        EXPECT_TRUE (((b4 << 100) >> 100) == b4);

        // TODO: should add tests for other ops (although they also get pretty well tested in the RSA unit test)

        BigInteger b5;
        b5.loadFromMemoryBlock (b3.toMemoryBlock());
        EXPECT_TRUE (b3 == b5);
    }
}

TEST (BigIntegerTests, BitSetting)
{
    Random r = getRandom();
    static uint8 test[2048];

    for (int j = 100000; --j >= 0;)
    {
        uint32 offset = static_cast<uint32> (r.nextInt (200) + 10);
        uint32 num = static_cast<uint32> (r.nextInt (32) + 1);
        uint32 value = static_cast<uint32> (r.nextInt());

        if (num < 32)
            value &= ((1u << num) - 1);

        auto old1 = readLittleEndianBitsInBuffer (test, offset - 6, 6);
        auto old2 = readLittleEndianBitsInBuffer (test, offset + num, 6);
        writeLittleEndianBitsInBuffer (test, offset, num, value);
        auto result = readLittleEndianBitsInBuffer (test, offset, num);

        EXPECT_TRUE (result == value);
        EXPECT_TRUE (old1 == readLittleEndianBitsInBuffer (test, offset - 6, 6));
        EXPECT_TRUE (old2 == readLittleEndianBitsInBuffer (test, offset + num, 6));
    }
}
