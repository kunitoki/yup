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

#include <algorithm>
#include <cmath>
#include <vector>

using namespace yup;

//==============================================================================
class VAStateVariableFilterTests : public ::testing::Test
{
protected:
    static constexpr double sampleRate = 48000.0;
    static constexpr int blockSize = 256;

    /** A frequency low enough to stand in for DC in the analytic response. */
    static constexpr double nearDc = 0.1;

    /** Sampled sines miss the true peak by up to 1 - cos (pi / 48) at 1 kHz. */
    static constexpr double sinePeakTolerance = 5e-3;

    void SetUp() override
    {
        filter.prepare (sampleRate, blockSize);
    }

    /** Feeds a sine for a while and returns the steady-state peak amplitude. */
    static double measureSineGain (VAStateVariableFilter<double>& f, double frequency)
    {
        f.reset();

        const int settle = 48000;
        const int measure = 4800;
        auto peak = 0.0;

        for (int i = 0; i < settle + measure; ++i)
        {
            const auto x = std::sin (MathConstants<double>::twoPi * frequency * i / sampleRate);
            const auto y = f.processSample (x);

            if (i >= settle)
                peak = std::max (peak, std::abs (y));
        }

        return peak;
    }

    VAStateVariableFilter<double> filter;
};

//==============================================================================
TEST_F (VAStateVariableFilterTests, DefaultsToLowpassAtOneKilohertz)
{
    EXPECT_TRUE (filter.getMode().test (FilterMode::lowpass));
    EXPECT_DOUBLE_EQ (1000.0, filter.getCutoffFrequency());
}

TEST_F (VAStateVariableFilterTests, ResonanceMapsToQ)
{
    EXPECT_NEAR (0.5, VAStateVariableFilter<double>::resonanceToQ (0.0), 1e-12);
    EXPECT_NEAR (1.0, VAStateVariableFilter<double>::resonanceToQ (0.5), 1e-12);
    EXPECT_NEAR (5.0, VAStateVariableFilter<double>::resonanceToQ (0.9), 1e-12);
    EXPECT_LT (VAStateVariableFilter<double>::resonanceToQ (1.0), 1000.0);

    filter.setResonance (0.5);
    EXPECT_NEAR (1.0, filter.getQ(), 1e-12);
}

TEST_F (VAStateVariableFilterTests, CutoffPitchFollowsMidi)
{
    filter.setCutoffPitch (69.0);
    EXPECT_NEAR (440.0, filter.getCutoffFrequency(), 1e-9);

    filter.setCutoffPitch (81.0);
    EXPECT_NEAR (880.0, filter.getCutoffFrequency(), 1e-9);
}

TEST_F (VAStateVariableFilterTests, SupportsSevenModes)
{
    const auto modes = filter.getSupportedModes();

    EXPECT_TRUE (modes.test (FilterMode::lowpass));
    EXPECT_TRUE (modes.test (FilterMode::highpass));
    EXPECT_TRUE (modes.test (FilterMode::bandpassCsg));
    EXPECT_TRUE (modes.test (FilterMode::bandpassCpg));
    EXPECT_TRUE (modes.test (FilterMode::bandstop));
    EXPECT_TRUE (modes.test (FilterMode::allpass));
    EXPECT_TRUE (modes.test (FilterMode::peak));
    EXPECT_FALSE (modes.test (FilterMode::lowshelf));
    EXPECT_FALSE (modes.test (FilterMode::highshelf));
}

TEST_F (VAStateVariableFilterTests, CompositeBandpassResolvesToConstantSkirtGain)
{
    filter.setMode (FilterMode::bandpass);
    EXPECT_TRUE (filter.getMode().test (FilterMode::bandpassCsg));
}

TEST_F (VAStateVariableFilterTests, LowpassPassesDcAndRejectsHighFrequencies)
{
    filter.setParameters (FilterMode::lowpass, 1000.0, 0.707, 0.0, sampleRate);

    EXPECT_NEAR (1.0, filter.getMagnitudeResponse (nearDc), 1e-6);
    EXPECT_LT (filter.getMagnitudeResponse (20000.0), 0.01);
    EXPECT_NEAR (1.0, measureSineGain (filter, 20.0), sinePeakTolerance);
    EXPECT_LT (measureSineGain (filter, 16000.0), 0.01);
}

TEST_F (VAStateVariableFilterTests, HighpassRejectsDc)
{
    filter.setParameters (FilterMode::highpass, 1000.0, 0.707, 0.0, sampleRate);

    EXPECT_LT (filter.getMagnitudeResponse (nearDc), 1e-5);
    EXPECT_NEAR (1.0, filter.getMagnitudeResponse (20000.0), 1e-2);
    EXPECT_LT (measureSineGain (filter, 10.0), 1e-3);
}

TEST_F (VAStateVariableFilterTests, UnityGainBandpassIsUnityAtCutoff)
{
    filter.setParameters (FilterMode::bandpassCpg, 1000.0, 4.0, 0.0, sampleRate);

    EXPECT_NEAR (1.0, filter.getMagnitudeResponse (1000.0), 1e-6);
    EXPECT_NEAR (1.0, measureSineGain (filter, 1000.0), sinePeakTolerance);
}

TEST_F (VAStateVariableFilterTests, ConstantSkirtBandpassPeaksAtQ)
{
    filter.setParameters (FilterMode::bandpassCsg, 1000.0, 4.0, 0.0, sampleRate);

    EXPECT_NEAR (4.0, filter.getMagnitudeResponse (1000.0), 1e-6);
    EXPECT_NEAR (4.0, measureSineGain (filter, 1000.0), 4.0 * sinePeakTolerance);
}

TEST_F (VAStateVariableFilterTests, NotchRejectsCutoff)
{
    filter.setParameters (FilterMode::bandstop, 1000.0, 2.0, 0.0, sampleRate);

    EXPECT_LT (filter.getMagnitudeResponse (1000.0), 1e-9);
    EXPECT_NEAR (1.0, filter.getMagnitudeResponse (nearDc), 1e-6);
    EXPECT_LT (measureSineGain (filter, 1000.0), sinePeakTolerance);
}

TEST_F (VAStateVariableFilterTests, AllpassHasUnityMagnitudeEverywhere)
{
    filter.setParameters (FilterMode::allpass, 1000.0, 0.707, 0.0, sampleRate);

    for (auto frequency : { 10.0, 500.0, 1000.0, 4000.0, 20000.0 })
        EXPECT_NEAR (1.0, filter.getMagnitudeResponse (frequency), 1e-9);

    EXPECT_NEAR (1.0, measureSineGain (filter, 1000.0), sinePeakTolerance);
}

TEST_F (VAStateVariableFilterTests, PeakShelfBoostsByGainAtCutoff)
{
    filter.setParameters (FilterMode::peak, 1000.0, 2.0, 12.0, sampleRate);

    const auto expected = Decibels::decibelsToGain (12.0);

    EXPECT_NEAR (expected, filter.getMagnitudeResponse (1000.0), 1e-6);
    EXPECT_NEAR (1.0, filter.getMagnitudeResponse (nearDc), 1e-3);
    EXPECT_NEAR (expected, measureSineGain (filter, 1000.0), expected * sinePeakTolerance);
}

TEST_F (VAStateVariableFilterTests, ZeroShelfGainIsBypass)
{
    filter.setParameters (FilterMode::peak, 1000.0, 2.0, 0.0, sampleRate);

    for (auto frequency : { 10.0, 1000.0, 10000.0 })
        EXPECT_NEAR (1.0, filter.getMagnitudeResponse (frequency), 1e-9);
}

TEST_F (VAStateVariableFilterTests, AllOutputsAgreeWithModeSelection)
{
    using Filter = VAStateVariableFilter<double>;

    Filter selected;
    Filter all;

    selected.prepare (sampleRate, blockSize);
    all.prepare (sampleRate, blockSize);

    const struct
    {
        FilterModeType mode;
        double Filter::Outputs::*output;
    } pairs[] = {
        { FilterMode::lowpass, &Filter::Outputs::lowpass },
        { FilterMode::highpass, &Filter::Outputs::highpass },
        { FilterMode::bandpassCsg, &Filter::Outputs::bandpass },
        { FilterMode::bandpassCpg, &Filter::Outputs::unityGainBandpass },
        { FilterMode::bandstop, &Filter::Outputs::notch },
        { FilterMode::allpass, &Filter::Outputs::allpass },
        { FilterMode::peak, &Filter::Outputs::bandShelf },
    };

    for (const auto& pair : pairs)
    {
        selected.setParameters (pair.mode, 2000.0, 1.5, 6.0, sampleRate);
        all.setParameters (pair.mode, 2000.0, 1.5, 6.0, sampleRate);
        selected.reset();
        all.reset();

        for (int i = 0; i < 64; ++i)
        {
            const auto x = (i == 0) ? 1.0 : 0.0;

            EXPECT_DOUBLE_EQ (selected.processSample (x), all.processAllOutputs (x).*(pair.output));
        }
    }
}

TEST_F (VAStateVariableFilterTests, PeakOutputIsLowpassMinusHighpass)
{
    filter.setParameters (FilterMode::lowpass, 2000.0, 1.0, 0.0, sampleRate);

    for (int i = 0; i < 64; ++i)
    {
        const auto outputs = filter.processAllOutputs (i == 0 ? 1.0 : 0.0);

        EXPECT_NEAR (outputs.lowpass - outputs.highpass, outputs.peak, 1e-12);
    }
}

TEST_F (VAStateVariableFilterTests, ProcessBlockMatchesProcessSample)
{
    VAStateVariableFilter<float> blockFilter;
    VAStateVariableFilter<float> sampleFilter;

    blockFilter.prepare (sampleRate, blockSize);
    sampleFilter.prepare (sampleRate, blockSize);
    blockFilter.setParameters (FilterMode::bandstop, 3000.0f, 3.0f, 0.0f, sampleRate);
    sampleFilter.setParameters (FilterMode::bandstop, 3000.0f, 3.0f, 0.0f, sampleRate);

    std::vector<float> input (blockSize), output (blockSize);

    for (int i = 0; i < blockSize; ++i)
        input[static_cast<std::size_t> (i)] = std::sin (0.37f * static_cast<float> (i)) * 0.5f;

    blockFilter.processBlock (input.data(), output.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
        EXPECT_NEAR (sampleFilter.processSample (input[static_cast<std::size_t> (i)]), output[static_cast<std::size_t> (i)], 1e-6f);
}

TEST_F (VAStateVariableFilterTests, ResetClearsState)
{
    filter.setParameters (FilterMode::lowpass, 500.0, 5.0, 0.0, sampleRate);

    for (int i = 0; i < 100; ++i)
        filter.processSample (1.0);

    filter.reset();

    EXPECT_DOUBLE_EQ (0.0, filter.processAllOutputs (0.0).lowpass);
}

TEST_F (VAStateVariableFilterTests, AliasesCompile)
{
    VAStateVariableFilterFloat asFloat;
    VAStateVariableFilterDouble asDouble;

    asFloat.prepare (sampleRate, blockSize);
    asDouble.prepare (sampleRate, blockSize);

    EXPECT_FLOAT_EQ (0.0f, asFloat.processSample (0.0f));
    EXPECT_DOUBLE_EQ (0.0, asDouble.processSample (0.0));
}
