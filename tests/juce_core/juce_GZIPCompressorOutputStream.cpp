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

TEST (GZIPCompressorOutputStreamTests, Zipping)
{
    Random rng = Random::getSystemRandom();

    for (int i = 100; --i >= 0;)
    {
        MemoryOutputStream original, compressed, uncompressed;

        {
            GZIPCompressorOutputStream zipper (compressed, rng.nextInt (10));

            for (int j = rng.nextInt (100); --j >= 0;)
            {
                MemoryBlock data ((unsigned int) (rng.nextInt (2000) + 1));

                for (int k = (int) data.getSize(); --k >= 0;)
                    data[k] = (char) rng.nextInt (255);

                original << data;
                zipper << data;
            }
        }

        {
            MemoryInputStream compressedInput (compressed.getData(), compressed.getDataSize(), false);
            GZIPDecompressorInputStream unzipper (compressedInput);

            uncompressed << unzipper;
        }

        EXPECT_EQ ((int) uncompressed.getDataSize(), (int) original.getDataSize());

        if (original.getDataSize() == uncompressed.getDataSize())
            EXPECT_TRUE (memcmp (uncompressed.getData(), original.getData(), original.getDataSize()) == 0);
    }
}
