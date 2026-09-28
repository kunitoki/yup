/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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
*/

#include <yup_audio_basics/yup_audio_basics.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class MPESynthesiserTests : public ::testing::Test
{
protected:
    struct Voice : public MPESynthesiserVoice
    {
        void noteStarted() override {}

        void noteStopped (bool) override { clearCurrentNote(); }

        void notePressureChanged() override {}

        void notePitchbendChanged() override {}

        void noteTimbreChanged() override {}

        void noteKeyStateChanged() override {}

        void renderNextBlock (AudioBuffer<float>&, int, int) override {}
    };

#if YUP_ENABLE_ALLOCATION_HOOKS
    struct AllocationCounter : private AllocationHooks::Listener
    {
        AllocationCounter() { AllocationHooks::getForCurrentThread().addListener (this); }

        ~AllocationCounter() override { AllocationHooks::getForCurrentThread().removeListener (this); }

        void newOrDeleteCalled() noexcept override { ++count; }

        size_t count = 0;
    };
#endif

    void SetUp() override
    {
        synth.enableLegacyMode();
        synth.setCurrentPlaybackSampleRate (44100.0);
        synth.setVoiceStealingEnabled (true);
        synth.addVoice (new Voice());
        synth.addVoice (new Voice());
    }

    MPEInstrument instrument;
    MPESynthesiser synth { instrument };
};

//==============================================================================
TEST_F (MPESynthesiserTests, StealsVoiceWhenAllVoicesAreActive)
{
    instrument.noteOn (1, 60, MPEValue::from7BitInt (100));
    instrument.noteOn (1, 72, MPEValue::from7BitInt (100));
    instrument.noteOn (1, 67, MPEValue::from7BitInt (100));

    const auto note0 = synth.getVoice (0)->getCurrentlyPlayingNote().initialNote;
    const auto note1 = synth.getVoice (1)->getCurrentlyPlayingNote().initialNote;

    EXPECT_TRUE (note0 == 67 || note1 == 67);
}

#if YUP_ENABLE_ALLOCATION_HOOKS
TEST_F (MPESynthesiserTests, VoiceStealingDoesNotAllocate)
{
    instrument.noteOn (1, 60, MPEValue::from7BitInt (100));
    instrument.noteOn (1, 72, MPEValue::from7BitInt (100));

    AllocationCounter allocations;
    instrument.noteOn (1, 67, MPEValue::from7BitInt (100));
    instrument.noteOn (1, 64, MPEValue::from7BitInt (100));

    EXPECT_EQ (0u, allocations.count);
}
#endif
