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

namespace
{
void expectEqualsWithinOneCent (double frequencyInHertzActual,
                                double frequencyInHertzExpected)
{
    double ratio = frequencyInHertzActual / frequencyInHertzExpected;
    double oneCent = 1.0005946;
    EXPECT_LT (ratio, oneCent);
    EXPECT_GT (ratio, 1.0 / oneCent);
}
} // namespace

TEST (MPENoteTests, GetFrequencyInHertz)
{
    MPENote note;
    note.initialNote = 60;
    note.totalPitchbendInSemitones = -0.5;
    expectEqualsWithinOneCent (note.getFrequencyInHertz(), 254.178);
}

TEST (MPENoteTests, DefaultConstructedIsNotValid)
{
    MPENote note;
    EXPECT_FALSE (note.isValid());
}

TEST (MPENoteTests, DefaultKeyStateIsOff)
{
    MPENote note;
    EXPECT_EQ (MPENote::off, note.keyState);
}

TEST (MPENoteTests, NoteA4FrequencyWithNoPitchbend)
{
    MPENote note;
    note.initialNote = 69; // A4
    note.totalPitchbendInSemitones = 0.0;
    expectEqualsWithinOneCent (note.getFrequencyInHertz(), 440.0);
}

TEST (MPENoteTests, OctaveUpDoublesFrequency)
{
    MPENote low, high;
    low.initialNote = 60;
    low.totalPitchbendInSemitones = 0.0;
    high.initialNote = 72; // C5 = C4 + 12 semitones
    high.totalPitchbendInSemitones = 0.0;

    const double ratio = high.getFrequencyInHertz() / low.getFrequencyInHertz();
    EXPECT_NEAR (ratio, 2.0, 0.001);
}

TEST (MPENoteTests, CustomA4FrequencyIsUsed)
{
    MPENote note;
    note.initialNote = 69; // A4
    note.totalPitchbendInSemitones = 0.0;
    // With 432 Hz reference
    expectEqualsWithinOneCent (note.getFrequencyInHertz (432.0), 432.0);
}

TEST (MPENoteTests, DefaultMidiChannelIsZero)
{
    MPENote note;
    EXPECT_EQ (0, note.midiChannel);
}