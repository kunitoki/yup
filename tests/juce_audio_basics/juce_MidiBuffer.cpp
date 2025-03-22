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

#include <juce_audio_basics/juce_audio_basics.h>

using namespace juce;

TEST (MidiBufferTests, Clear)
{
    const auto message = MidiMessage::noteOn (1, 64, 0.5f);

    const auto testBuffer = [&]
    {
        MidiBuffer buffer;
        buffer.addEvent (message, 0);
        buffer.addEvent (message, 10);
        buffer.addEvent (message, 20);
        buffer.addEvent (message, 30);
        return buffer;
    }();

    {
        auto buffer = testBuffer;
        buffer.clear (10, 0);
        EXPECT_EQ (buffer.getNumEvents(), 4);
    }

    {
        auto buffer = testBuffer;
        buffer.clear (10, 1);
        EXPECT_EQ (buffer.getNumEvents(), 3);
    }

    {
        auto buffer = testBuffer;
        buffer.clear (10, 10);
        EXPECT_EQ (buffer.getNumEvents(), 3);
    }

    {
        auto buffer = testBuffer;
        buffer.clear (10, 20);
        EXPECT_EQ (buffer.getNumEvents(), 2);
    }

    {
        auto buffer = testBuffer;
        buffer.clear (10, 30);
        EXPECT_EQ (buffer.getNumEvents(), 1);
    }

    {
        auto buffer = testBuffer;
        buffer.clear (10, 300);
        EXPECT_EQ (buffer.getNumEvents(), 1);
    }
}
