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

#include <yup_dsp/yup_dsp.h>
#include <gtest/gtest.h>

using namespace yup;

class MorphingOscillatorTests : public ::testing::Test
{
protected:
    const FourierSeries<double> first = FourierSeries<double>::create (Waveform::sawtooth, 32);
    const FourierSeries<double> second = FourierSeries<double>::create (Waveform::square, 32);
};

TEST_F (MorphingOscillatorTests, MorphCommutesWithFixedRatioSynchronization)
{
    for (const auto mode : { SyncMode::none, SyncMode::hard, SyncMode::mirrored, SyncMode::pulsar })
    {
        MorphingOscillator<double> oscillator;
        oscillator.setSynthesis (MorphingOscillator<double>::Synthesis::additive);
        oscillator.prepare (48000.0, 32);
        oscillator.setSeries (first, second);
        oscillator.setSyncMode (mode);
        oscillator.setFollowerRatio (1.375);
        oscillator.setFrequency (1000.0);
        oscillator.update();

        FourierSeries<double> blended (32);
        for (int n = 1; n <= 32; ++n)
            blended.setHarmonic (n, 0.0, 0.75 * first.getSine (n) + 0.25 * second.getSine (n));

        SyncOscillator<double> reference;
        reference.setSynthesis (SyncOscillator<double>::Synthesis::additive);
        reference.prepare (48000.0, 32);
        reference.setFollowerSeries (blended);
        reference.setSyncMode (mode);
        reference.setFollowerRatio (1.375);
        reference.setFrequency (1000.0);
        reference.update();

        for (int i = 0; i < 512; ++i)
            EXPECT_NEAR (reference.processSample(), oscillator.processSample (0.25), 1e-10);
        EXPECT_FALSE (oscillator.needsUpdate());
    }
}

TEST_F (MorphingOscillatorTests, PerSampleMorphDoesNotRequireAnUpdate)
{
    MorphingOscillator<double> oscillator;
    oscillator.setSynthesis (MorphingOscillator<double>::Synthesis::additive);
    oscillator.prepare (48000.0, 1);
    oscillator.setSeries (FourierSeries<double>::create (Waveform::sine, 1),
                          FourierSeries<double>::create (Waveform::cosine, 1));
    oscillator.setFrequency (0.0);
    oscillator.setPhase (0.25);
    oscillator.update();

    for (const auto morph : { -1.0, 0.0, 0.25, 0.5, 1.0, 2.0 })
        EXPECT_NEAR (1.0 - jlimit (0.0, 1.0, morph), oscillator.processSample (morph), 1e-12);
    EXPECT_FALSE (oscillator.needsUpdate());
}
