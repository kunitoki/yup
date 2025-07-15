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

namespace
{

class InstrumentListener : public MPEInstrument::Listener
{
public:
    void noteAdded (MPENote newNote) override
    {
        lastNoteAdded = newNote;
        noteAddedCallCounter++;
    }

    void notePressureChanged (MPENote changedNote) override
    {
        lastNotePressureChanged = changedNote;
    }

    void notePitchbendChanged (MPENote changedNote) override
    {
        lastNotePitchbendChanged = changedNote;
    }

    void noteTimbreChanged (MPENote changedNote) override
    {
        lastNoteTimbreChanged = changedNote;
    }

    void noteKeyStateChanged (MPENote changedNote) override
    {
        lastNoteKeyStateChanged = changedNote;
    }

    void noteReleased (MPENote finishedNote) override
    {
        lastNoteReleased = finishedNote;
        noteReleasedCallCounter++;
    }

    MPENote lastNoteAdded, lastNotePressureChanged, lastNotePitchbendChanged,
        lastNoteTimbreChanged, lastNoteKeyStateChanged, lastNoteReleased;
    int noteAddedCallCounter = 0, noteReleasedCallCounter = 0;
};

void expectNote (MPENote note, int initialNote, int totalPitchbendInSemitones, int pitchbendInMPEUnits, int timbre, MPENote::KeyState keyState)
{
    EXPECT_EQ (note.initialNote, initialNote);
    EXPECT_EQ (note.totalPitchbendInSemitones, totalPitchbendInSemitones);
    EXPECT_EQ (note.pitchbend.as14BitInt(), pitchbendInMPEUnits);
    EXPECT_EQ (note.timbre.as7BitInt(), timbre);
    EXPECT_EQ (note.keyState, keyState);
}

} // namespace

class MPEInstrumentTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // using lower and upper MPE zones with the following layout for testing
        //
        // 1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16
        // * ...................|             |........................ *
        testLayout.setLowerZone (5);
        testLayout.setUpperZone (6);
    }

    MPEZoneLayout testLayout;
};

TEST_F (MPEInstrumentTest, InitialZoneLayout)
{
    MPEInstrument test;
    EXPECT_FALSE (test.getZoneLayout().getLowerZone().isActive());
    EXPECT_FALSE (test.getZoneLayout().getUpperZone().isActive());
}

TEST_F (MPEInstrumentTest, GetSetZoneLayout)
{
    MPEInstrument test;
    test.setZoneLayout (testLayout);

    auto newLayout = test.getZoneLayout();

    EXPECT_TRUE (test.getZoneLayout().getLowerZone().isActive());
    EXPECT_TRUE (test.getZoneLayout().getUpperZone().isActive());
    EXPECT_EQ (newLayout.getLowerZone().getMasterChannel(), 1);
    EXPECT_EQ (newLayout.getLowerZone().numMemberChannels, 5);
    EXPECT_EQ (newLayout.getUpperZone().getMasterChannel(), 16);
    EXPECT_EQ (newLayout.getUpperZone().numMemberChannels, 6);
}

TEST_F (MPEInstrumentTest, NoteOnNoteOff)
{
    {
        MPEInstrument test;
        test.setZoneLayout (testLayout);
        EXPECT_EQ (test.getNumPlayingNotes(), 0);
    }

    {
        MPEInstrument test;
        test.setZoneLayout (testLayout);

        InstrumentListener listener;
        test.addListener (&listener);

        // note-on on unused channel - ignore
        test.noteOn (7, 60, MPEValue::from7BitInt (100));
        EXPECT_EQ (test.getNumPlayingNotes(), 0);
        EXPECT_EQ (listener.noteAddedCallCounter, 0);

        // note-on on member channel - create new note
        test.noteOn (3, 60, MPEValue::from7BitInt (100));
        EXPECT_EQ (test.getNumPlayingNotes(), 1);
        EXPECT_EQ (listener.noteAddedCallCounter, 1);

        auto note1 = test.getNote (3, 60);
        EXPECT_EQ (note1.initialNote, 60);
        EXPECT_EQ (note1.totalPitchbendInSemitones, 0);
        EXPECT_EQ (note1.pitchbend.as14BitInt(), 8192);
        EXPECT_EQ (note1.timbre.as7BitInt(), 64);
        EXPECT_EQ (note1.keyState, MPENote::keyDown);

        // note-off
        test.noteOff (3, 60, MPEValue::from7BitInt (33));
        EXPECT_EQ (test.getNumPlayingNotes(), 0);
        EXPECT_EQ (listener.noteReleasedCallCounter, 1);

        // note-on on master channel - create new note
        test.noteOn (1, 62, MPEValue::from7BitInt (100));
        EXPECT_EQ (test.getNumPlayingNotes(), 1);
        EXPECT_EQ (listener.noteAddedCallCounter, 2);

        /*
        auto note2 = test.getNote(1, 62);
        EXPECT_EQ(note1.initialNote, 62);
        EXPECT_EQ(note1.totalPitchbendInSemitones, 0);
        EXPECT_EQ(note1.pitchbend.as14BitInt(), 8192);
        EXPECT_EQ(note1.timbre.as7BitInt(), 64);
        EXPECT_EQ(note1.keyState, MPENote::keyDown);
        */

        // note-off
        test.noteOff (1, 62, MPEValue::from7BitInt (33));
        EXPECT_EQ (test.getNumPlayingNotes(), 0);
        EXPECT_EQ (listener.noteReleasedCallCounter, 2);
    }
}

TEST_F (MPEInstrumentTest, NoteOffIgnoresNonMatchingNotes)
{
    MPEInstrument test;
    test.setZoneLayout (testLayout);
    test.noteOn (3, 60, MPEValue::from7BitInt (100));

    InstrumentListener listener;
    test.addListener (&listener);

    // note off with non-matching note number shouldn't do anything
    test.noteOff (3, 61, MPEValue::from7BitInt (33));
    EXPECT_EQ (test.getNumPlayingNotes(), 1);
    //expectNote(test.getNote(3, 60), 100, 0, 8192, 64, MPENote::keyDown);
    EXPECT_EQ (listener.noteReleasedCallCounter, 0);

    // note off with non-matching midi channel shouldn't do anything
    test.noteOff (2, 60, MPEValue::from7BitInt (33));
    EXPECT_EQ (test.getNumPlayingNotes(), 1);
    //expectNote(test.getNote(3, 60), 100, 0, 8192, 64, MPENote::keyDown);
    EXPECT_EQ (listener.noteReleasedCallCounter, 0);
}

TEST_F (MPEInstrumentTest, PitchbendChangeModifiesCorrectNote)
{
    MPEInstrument test;
    test.setZoneLayout (testLayout);

    test.noteOn (3, 60, MPEValue::from7BitInt (100));
    test.noteOn (4, 61, MPEValue::from7BitInt (100));
    EXPECT_EQ (test.getNumPlayingNotes(), 2);

    test.pitchbend (4, MPEValue::from14BitInt (9000));
    EXPECT_EQ (test.getNote (3, 60).pitchbend.as14BitInt(), 8192);
    EXPECT_EQ (test.getNote (4, 61).pitchbend.as14BitInt(), 9000);
}

TEST_F (MPEInstrumentTest, PressureChangeModifiesCorrectNote)
{
    MPEInstrument test;
    test.setZoneLayout (testLayout);

    test.noteOn (3, 60, MPEValue::from7BitInt (100));
    test.noteOn (4, 61, MPEValue::from7BitInt (100));

    test.pressure (4, MPEValue::from7BitInt (100));
    EXPECT_EQ (test.getNote (3, 60).pressure.as7BitInt(), 0);
    EXPECT_EQ (test.getNote (4, 61).pressure.as7BitInt(), 100);
}

TEST_F (MPEInstrumentTest, TimbreChangeModifiesCorrectNote)
{
    MPEInstrument test;
    test.setZoneLayout (testLayout);

    test.noteOn (3, 60, MPEValue::from7BitInt (100));
    test.noteOn (4, 61, MPEValue::from7BitInt (100));

    test.timbre (4, MPEValue::from7BitInt (100));
    EXPECT_EQ (test.getNote (3, 60).timbre.as7BitInt(), 64);
    EXPECT_EQ (test.getNote (4, 61).timbre.as7BitInt(), 100);
}
