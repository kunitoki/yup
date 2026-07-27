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

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

TEST (MPEUtilsTests, MPEChannelAssignerLowerZone)
{
    MPEZoneLayout layout;
    layout.setLowerZone (15);

    // lower zone
    MPEChannelAssigner channelAssigner (layout.getLowerZone());

    // check that channels are assigned in correct order
    int noteNum = 60;
    for (int ch = 2; ch <= 16; ++ch)
    {
        EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (noteNum), ch);
        EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (noteNum), ch);

        ++noteNum;
    }

    // check that note-offs are processed
    channelAssigner.noteOff (60);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (60), 2);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (60), 2);

    channelAssigner.noteOff (61);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (61), 3);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (61), 3);

    // check that assigned channel was last to play note
    channelAssigner.noteOff (65);
    channelAssigner.noteOff (66);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (66), 8);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (65), 7);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (66), 8);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (65), 7);

    // find closest channel playing nonequal note
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (80), 16);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (55), 2);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (80), 16);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (55), 2);

    // all notes off
    channelAssigner.allNotesOff();

    // last note played
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (66), 8);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (65), 7);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (80), 16);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (55), 2);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (66), 8);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (65), 7);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (80), 16);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (55), 2);

    // normal assignment
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (101), 3);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (20), 4);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (101), 3);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (20), 4);
}

TEST (MPEUtilsTests, MPEChannelAssignerUpperZone)
{
    MPEZoneLayout layout;
    layout.setUpperZone (15);

    // upper zone
    MPEChannelAssigner channelAssigner (layout.getUpperZone());

    // check that channels are assigned in correct order
    int noteNum = 60;
    for (int ch = 15; ch >= 1; --ch)
    {
        EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (noteNum), ch);
        EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (noteNum), ch);

        ++noteNum;
    }

    // check that note-offs are processed
    channelAssigner.noteOff (60);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (60), 15);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (60), 15);

    channelAssigner.noteOff (61);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (61), 14);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (61), 14);

    // check that assigned channel was last to play note
    channelAssigner.noteOff (65);
    channelAssigner.noteOff (66);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (66), 9);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (65), 10);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (66), 9);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (65), 10);

    // find closest channel playing nonequal note
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (80), 1);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (55), 15);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (80), 1);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (55), 15);

    // all notes off
    channelAssigner.allNotesOff();

    // last note played
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (66), 9);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (65), 10);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (80), 1);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (55), 15);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (66), 9);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (65), 10);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (80), 1);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (55), 15);

    // normal assignment
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (101), 14);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (20), 13);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (101), 14);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (20), 13);
}

TEST (MPEUtilsTests, MPEChannelAssignerLegacy)
{
    MPEChannelAssigner channelAssigner;

    // check that channels are assigned in correct order
    int noteNum = 60;
    for (int ch = 1; ch <= 16; ++ch)
    {
        EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (noteNum), ch);
        EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (noteNum), ch);

        ++noteNum;
    }

    // check that note-offs are processed
    channelAssigner.noteOff (60);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (60), 1);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (60), 1);

    channelAssigner.noteOff (61);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (61), 2);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (61), 2);

    // check that assigned channel was last to play note
    channelAssigner.noteOff (65);
    channelAssigner.noteOff (66);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (66), 7);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (65), 6);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (66), 7);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (65), 6);

    // find closest channel playing nonequal note
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (80), 16);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (55), 1);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (80), 16);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (55), 1);

    // all notes off
    channelAssigner.allNotesOff();

    // last note played
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (66), 7);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (65), 6);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (80), 16);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (55), 1);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (66), 7);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (65), 6);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (80), 16);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (55), 1);

    // normal assignment
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (101), 2);
    EXPECT_EQ (channelAssigner.findMidiChannelForNewNote (20), 3);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (101), 2);
    EXPECT_EQ (channelAssigner.findMidiChannelForExistingNote (20), 3);
}

TEST (MPEUtilsTests, MPEChannelRemapperLowerZone)
{
    // 3 different MPE 'sources', constant IDs
    const int sourceID1 = 0;
    const int sourceID2 = 1;
    const int sourceID3 = 2;

    MPEZoneLayout layout;
    layout.setLowerZone (15);

    // lower zone
    MPEChannelRemapper channelRemapper (layout.getLowerZone());

    // first source, shouldn't remap
    for (int ch = 2; ch <= 16; ++ch)
    {
        auto noteOn = MidiMessage::noteOn (ch, 60, 1.0f);

        channelRemapper.remapMidiChannelIfNeeded (noteOn, sourceID1);
        EXPECT_EQ (noteOn.getChannel(), ch);
    }

    auto noteOn = MidiMessage::noteOn (2, 60, 1.0f);

    // remap onto oldest last-used channel
    channelRemapper.remapMidiChannelIfNeeded (noteOn, sourceID2);
    EXPECT_EQ (noteOn.getChannel(), 2);

    // remap onto oldest last-used channel
    channelRemapper.remapMidiChannelIfNeeded (noteOn, sourceID3);
    EXPECT_EQ (noteOn.getChannel(), 3);

    // remap to correct channel for source ID
    auto noteOff = MidiMessage::noteOff (2, 60, 1.0f);
    channelRemapper.remapMidiChannelIfNeeded (noteOff, sourceID3);
    EXPECT_EQ (noteOff.getChannel(), 3);
}

TEST (MPEUtilsTests, MPEChannelRemapperUpperZone)
{
    // 3 different MPE 'sources', constant IDs
    const int sourceID1 = 0;
    const int sourceID2 = 1;
    const int sourceID3 = 2;

    MPEZoneLayout layout;
    layout.setUpperZone (15);

    // upper zone
    MPEChannelRemapper channelRemapper (layout.getUpperZone());

    // first source, shouldn't remap
    for (int ch = 15; ch >= 1; --ch)
    {
        auto noteOn = MidiMessage::noteOn (ch, 60, 1.0f);

        channelRemapper.remapMidiChannelIfNeeded (noteOn, sourceID1);
        EXPECT_EQ (noteOn.getChannel(), ch);
    }

    auto noteOn = MidiMessage::noteOn (15, 60, 1.0f);

    // remap onto oldest last-used channel
    channelRemapper.remapMidiChannelIfNeeded (noteOn, sourceID2);
    EXPECT_EQ (noteOn.getChannel(), 15);

    // remap onto oldest last-used channel
    channelRemapper.remapMidiChannelIfNeeded (noteOn, sourceID3);
    EXPECT_EQ (noteOn.getChannel(), 14);

    // remap to correct channel for source ID
    auto noteOff = MidiMessage::noteOff (15, 60, 1.0f);
    channelRemapper.remapMidiChannelIfNeeded (noteOff, sourceID3);
    EXPECT_EQ (noteOff.getChannel(), 14);
}