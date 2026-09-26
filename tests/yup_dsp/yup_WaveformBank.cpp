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

class WaveformBankTests : public ::testing::Test
{
protected:
    std::array<FourierSeries<double>, 2> frames {
        FourierSeries<double>::create (Waveform::sine, 16),
        FourierSeries<double>::create (Waveform::cosine, 16)
    };
    WaveformBank<double> bank;

    void SetUp() override { bank.prepare ({ frames.data(), frames.size() }); }
};

TEST_F (WaveformBankTests, MorphsEndpointsAtTheSamePhase)
{
    for (const auto morph : { 0.0, 0.25, 0.5, 1.0 })
    {
        for (int i = 0; i < 97; ++i)
        {
            const auto phase = i / 97.0;
            const auto angle = MathConstants<double>::twoPi * phase;
            EXPECT_NEAR ((1.0 - morph) * std::sin (angle) + morph * std::cos (angle),
                         bank.getValue (phase, morph, 32.0), 2e-5);
            EXPECT_NEAR (MathConstants<double>::twoPi * ((1.0 - morph) * std::cos (angle) - morph * std::sin (angle)),
                         bank.getSlope (phase, morph, 32.0), 0.005);
        }
    }
}

TEST_F (WaveformBankTests, BandwidthSelectionExcludesUnsafeHarmonics)
{
    frames[0].clear();
    frames[0].setDC (0.25);
    frames[0].setHarmonic (8, 1.0, 0.0);
    bank.prepare ({ frames.data(), frames.size() });

    EXPECT_NEAR (1.25, bank.getValue (0.0, 0.0, 32.0), 1e-6);
    EXPECT_NEAR (0.25, bank.getValue (0.0, 0.0, 8.0), 1e-6);
    EXPECT_NEAR (0.25, bank.getValue (0.2, 0.0, 0.0), 1e-6);
}

TEST_F (WaveformBankTests, BandwidthTransitionsAreContinuous)
{
    frames[0].setWaveform (Waveform::sawtooth);
    bank.prepare ({ frames.data(), frames.size() });

    for (const auto boundary : { 1.0, 2.0, 4.0, 8.0, 16.0, 32.0 })
        EXPECT_NEAR (bank.getValue (0.173, 0.0, boundary - 1e-7),
                     bank.getValue (0.173, 0.0, boundary + 1e-7), 1e-6);
}

TEST_F (WaveformBankTests, WrapsPhaseAndClampsFramePosition)
{
    EXPECT_EQ (bank.getValue (0.25, 0.0, 32.0), bank.getValue (-0.75, -1.0, 32.0));
    EXPECT_EQ (bank.getValue (0.25, 1.0, 32.0), bank.getValue (1.25, 2.0, 32.0));
}

TEST_F (WaveformBankTests, EmptyBankIsSilent)
{
    bank.prepare ({});
    EXPECT_EQ (0, bank.getNumFrames());
    EXPECT_EQ (0.0, bank.getValue (0.3, 0.5, 32.0));
    EXPECT_EQ (0.0, bank.getSlope (0.3, 0.5, 32.0));
}

TEST_F (WaveformBankTests, RefreshedFramesReadLikeAFreshlyPreparedBank)
{
    std::array<FourierSeries<double>, 2> replacements {
        FourierSeries<double>::create (Waveform::sawtooth, 16),
        FourierSeries<double>::create (Waveform::triangle, 16)
    };
    replacements[0].setDC (0.125);

    WaveformBank<double> reference;
    reference.prepare ({ replacements.data(), replacements.size() });

    ASSERT_TRUE (bank.refreshFrames ({ replacements.data(), replacements.size() }));
    EXPECT_EQ (reference.getNumFrames(), bank.getNumFrames());
    EXPECT_EQ (reference.getNumHarmonics(), bank.getNumHarmonics());

    for (const auto position : { 0.0, 0.3, 0.5, 1.0 })
    {
        for (const auto bandwidth : { 0.0, 1.0, 4.0, 16.0, 32.0 })
        {
            for (int i = 0; i < 37; ++i)
            {
                const auto phase = i / 37.0;
                EXPECT_DOUBLE_EQ (reference.getValue (phase, position, bandwidth),
                                  bank.getValue (phase, position, bandwidth));
                EXPECT_DOUBLE_EQ (reference.getSlope (phase, position, bandwidth),
                                  bank.getSlope (phase, position, bandwidth));
            }
        }
    }
}

TEST_F (WaveformBankTests, RefreshZeroExtendsShorterFrames)
{
    std::array<FourierSeries<double>, 2> shorter {
        FourierSeries<double>::create (Waveform::sine, 4),
        FourierSeries<double>::create (Waveform::sine, 4)
    };

    ASSERT_TRUE (bank.refreshFrames ({ shorter.data(), shorter.size() }));
    EXPECT_EQ (16, bank.getNumHarmonics());

    for (int i = 0; i < 37; ++i)
    {
        const auto phase = i / 37.0;
        EXPECT_NEAR (std::sin (MathConstants<double>::twoPi * phase),
                     bank.getValue (phase, 1.0, 32.0), 2e-5);
    }
}

TEST_F (WaveformBankTests, RejectsMismatchedRefreshesAndLeavesTheBankUnchanged)
{
    const auto before = bank.getValue (0.173, 0.25, 32.0);

    std::array<FourierSeries<double>, 3> tooManyFrames {
        FourierSeries<double>::create (Waveform::sawtooth, 16),
        FourierSeries<double>::create (Waveform::sawtooth, 16),
        FourierSeries<double>::create (Waveform::sawtooth, 16)
    };
    EXPECT_FALSE (bank.refreshFrames ({ tooManyFrames.data(), tooManyFrames.size() }));

    std::array<FourierSeries<double>, 2> tooManyHarmonics {
        FourierSeries<double>::create (Waveform::sawtooth, 16),
        FourierSeries<double>::create (Waveform::sawtooth, 32)
    };
    EXPECT_FALSE (bank.refreshFrames ({ tooManyHarmonics.data(), tooManyHarmonics.size() }));

    EXPECT_EQ (2, bank.getNumFrames());
    EXPECT_EQ (16, bank.getNumHarmonics());
    EXPECT_DOUBLE_EQ (before, bank.getValue (0.173, 0.25, 32.0));
}

TEST_F (WaveformBankTests, RefreshingAnEmptyBankSucceedsWithNoFrames)
{
    bank.prepare ({});
    EXPECT_TRUE (bank.refreshFrames ({}));
    EXPECT_EQ (0, bank.getNumFrames());
}
