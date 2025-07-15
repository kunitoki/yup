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

#include <yup_audio_basics/yup_audio_basics.h>
#include <gtest/gtest.h>

using namespace yup;

class MPEZoneLayoutTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup common test data if needed
    }
};

TEST_F (MPEZoneLayoutTest, Initialisation)
{
    MPEZoneLayout layout;
    EXPECT_FALSE (layout.getLowerZone().isActive());
    EXPECT_FALSE (layout.getUpperZone().isActive());
}

TEST_F (MPEZoneLayoutTest, AddingZones)
{
    MPEZoneLayout layout;

    layout.setLowerZone (7);

    EXPECT_TRUE (layout.getLowerZone().isActive());
    EXPECT_FALSE (layout.getUpperZone().isActive());
    EXPECT_EQ (layout.getLowerZone().getMasterChannel(), 1);
    EXPECT_EQ (layout.getLowerZone().numMemberChannels, 7);

    layout.setUpperZone (7);

    EXPECT_TRUE (layout.getLowerZone().isActive());
    EXPECT_TRUE (layout.getUpperZone().isActive());
    EXPECT_EQ (layout.getLowerZone().getMasterChannel(), 1);
    EXPECT_EQ (layout.getLowerZone().numMemberChannels, 7);
    EXPECT_EQ (layout.getUpperZone().getMasterChannel(), 16);
    EXPECT_EQ (layout.getUpperZone().numMemberChannels, 7);

    layout.setLowerZone (3);

    EXPECT_TRUE (layout.getLowerZone().isActive());
    EXPECT_TRUE (layout.getUpperZone().isActive());
    EXPECT_EQ (layout.getLowerZone().getMasterChannel(), 1);
    EXPECT_EQ (layout.getLowerZone().numMemberChannels, 3);
    EXPECT_EQ (layout.getUpperZone().getMasterChannel(), 16);
    EXPECT_EQ (layout.getUpperZone().numMemberChannels, 7);

    layout.setUpperZone (3);

    EXPECT_TRUE (layout.getLowerZone().isActive());
    EXPECT_TRUE (layout.getUpperZone().isActive());
    EXPECT_EQ (layout.getLowerZone().getMasterChannel(), 1);
    EXPECT_EQ (layout.getLowerZone().numMemberChannels, 3);
    EXPECT_EQ (layout.getUpperZone().getMasterChannel(), 16);
    EXPECT_EQ (layout.getUpperZone().numMemberChannels, 3);

    layout.setLowerZone (15);

    EXPECT_TRUE (layout.getLowerZone().isActive());
    EXPECT_FALSE (layout.getUpperZone().isActive());
    EXPECT_EQ (layout.getLowerZone().getMasterChannel(), 1);
    EXPECT_EQ (layout.getLowerZone().numMemberChannels, 15);
}

TEST_F (MPEZoneLayoutTest, ClearAllZones)
{
    MPEZoneLayout layout;

    EXPECT_FALSE (layout.getLowerZone().isActive());
    EXPECT_FALSE (layout.getUpperZone().isActive());

    layout.setLowerZone (7);
    layout.setUpperZone (2);

    EXPECT_TRUE (layout.getLowerZone().isActive());
    EXPECT_TRUE (layout.getUpperZone().isActive());

    layout.clearAllZones();

    EXPECT_FALSE (layout.getLowerZone().isActive());
    EXPECT_FALSE (layout.getUpperZone().isActive());
}

TEST_F (MPEZoneLayoutTest, ProcessMidiBuffers)
{
    MPEZoneLayout layout;
    MidiBuffer buffer;

    buffer = MPEMessages::setLowerZone (7);
    layout.processNextMidiBuffer (buffer);

    EXPECT_TRUE (layout.getLowerZone().isActive());
    EXPECT_FALSE (layout.getUpperZone().isActive());
    EXPECT_EQ (layout.getLowerZone().getMasterChannel(), 1);
    EXPECT_EQ (layout.getLowerZone().numMemberChannels, 7);

    buffer = MPEMessages::setUpperZone (7);
    layout.processNextMidiBuffer (buffer);

    EXPECT_TRUE (layout.getLowerZone().isActive());
    EXPECT_TRUE (layout.getUpperZone().isActive());
    EXPECT_EQ (layout.getLowerZone().getMasterChannel(), 1);
    EXPECT_EQ (layout.getLowerZone().numMemberChannels, 7);
    EXPECT_EQ (layout.getUpperZone().getMasterChannel(), 16);
    EXPECT_EQ (layout.getUpperZone().numMemberChannels, 7);

    {
        buffer = MPEMessages::setLowerZone (10);
        layout.processNextMidiBuffer (buffer);

        EXPECT_TRUE (layout.getLowerZone().isActive());
        EXPECT_TRUE (layout.getUpperZone().isActive());
        EXPECT_EQ (layout.getLowerZone().getMasterChannel(), 1);
        EXPECT_EQ (layout.getLowerZone().numMemberChannels, 10);
        EXPECT_EQ (layout.getUpperZone().getMasterChannel(), 16);
        EXPECT_EQ (layout.getUpperZone().numMemberChannels, 4);

        buffer = MPEMessages::setLowerZone (10, 33, 44);
        layout.processNextMidiBuffer (buffer);

        EXPECT_EQ (layout.getLowerZone().numMemberChannels, 10);
        EXPECT_EQ (layout.getLowerZone().perNotePitchbendRange, 33);
        EXPECT_EQ (layout.getLowerZone().masterPitchbendRange, 44);
    }

    {
        buffer = MPEMessages::setUpperZone (10);
        layout.processNextMidiBuffer (buffer);

        EXPECT_TRUE (layout.getLowerZone().isActive());
        EXPECT_TRUE (layout.getUpperZone().isActive());
        EXPECT_EQ (layout.getLowerZone().getMasterChannel(), 1);
        EXPECT_EQ (layout.getLowerZone().numMemberChannels, 4);
        EXPECT_EQ (layout.getUpperZone().getMasterChannel(), 16);
        EXPECT_EQ (layout.getUpperZone().numMemberChannels, 10);

        buffer = MPEMessages::setUpperZone (10, 33, 44);

        layout.processNextMidiBuffer (buffer);

        EXPECT_EQ (layout.getUpperZone().numMemberChannels, 10);
        EXPECT_EQ (layout.getUpperZone().perNotePitchbendRange, 33);
        EXPECT_EQ (layout.getUpperZone().masterPitchbendRange, 44);
    }

    buffer = MPEMessages::clearAllZones();
    layout.processNextMidiBuffer (buffer);

    EXPECT_FALSE (layout.getLowerZone().isActive());
    EXPECT_FALSE (layout.getUpperZone().isActive());
}

TEST_F (MPEZoneLayoutTest, ProcessIndividualMidiMessages)
{
    MPEZoneLayout layout;

    layout.processNextMidiEvent ({ 0x80, 0x59, 0xd0 }); // unrelated note-off msg
    layout.processNextMidiEvent ({ 0xb0, 0x64, 0x06 }); // RPN part 1
    layout.processNextMidiEvent ({ 0xb0, 0x65, 0x00 }); // RPN part 2
    layout.processNextMidiEvent ({ 0xb8, 0x0b, 0x66 }); // unrelated CC msg
    layout.processNextMidiEvent ({ 0xb0, 0x06, 0x03 }); // RPN part 3
    layout.processNextMidiEvent ({ 0x90, 0x60, 0x00 }); // unrelated note-on msg

    EXPECT_TRUE (layout.getLowerZone().isActive());
    EXPECT_FALSE (layout.getUpperZone().isActive());
    EXPECT_EQ (layout.getLowerZone().getMasterChannel(), 1);
    EXPECT_EQ (layout.getLowerZone().numMemberChannels, 3);
    EXPECT_EQ (layout.getLowerZone().perNotePitchbendRange, 48);
    EXPECT_EQ (layout.getLowerZone().masterPitchbendRange, 2);

    const auto masterPitchBend = 0x0c;
    layout.processNextMidiEvent ({ 0xb0, 0x64, 0x00 });
    layout.processNextMidiEvent ({ 0xb0, 0x06, masterPitchBend });

    EXPECT_EQ (layout.getLowerZone().masterPitchbendRange, masterPitchBend);

    const auto newPitchBend = 0x0d;
    layout.processNextMidiEvent ({ 0xb0, 0x06, newPitchBend });

    EXPECT_EQ (layout.getLowerZone().masterPitchbendRange, newPitchBend);
}
