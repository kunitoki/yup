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

namespace
{
constexpr double sampleRate = 44100.0;
constexpr int blockSize = 128;

template <typename SampleType>
std::vector<SampleType> makeInput()
{
    std::vector<SampleType> input (blockSize);

    for (int i = 0; i < blockSize; ++i)
        input[static_cast<std::size_t> (i)] = static_cast<SampleType> ((i % 17) / 17.0 - 0.5);

    return input;
}

template <typename SampleType>
void expectFiniteBuffer (const std::vector<SampleType>& buffer)
{
    for (auto sample : buffer)
        EXPECT_TRUE (std::isfinite (sample));
}
} // namespace

//==============================================================================

TEST (AnalogFilterDesignerTests, DesignsFiniteTwoPoleCoefficients)
{
    const auto lowpass = AnalogFilterDesigner<double>::designTwoPole (FilterMode::lowpass, 1000.0, sampleRate, 0.5);
    const auto bandpass = AnalogFilterDesigner<double>::designTwoPole (FilterMode::bandpassCpg, 1000.0, sampleRate, 0.5);
    const auto peak = AnalogFilterDesigner<double>::designTwoPole (FilterMode::peak, 1000.0, sampleRate, 0.5);

    EXPECT_TRUE (std::isfinite (lowpass.g));
    EXPECT_TRUE (std::isfinite (lowpass.h));
    EXPECT_TRUE (std::isfinite (bandpass.r2));
    EXPECT_TRUE (std::isfinite (peak.gainCorrection));
}

TEST (AnalogFilterDesignerTests, DesignsFiniteLadderCoefficients)
{
    const auto korg = AnalogFilterDesigner<double>::designKorg35 (FilterMode::lowpass, 1200.0, sampleRate, 0.7, 0.25);
    const auto moog = AnalogFilterDesigner<double>::designMoogLadder (AnalogMoogLadderMode::lowpass24, 1200.0, sampleRate, 0.7, 0.25);
    const auto diode = AnalogFilterDesigner<double>::designRolandDiode (1200.0, sampleRate, 0.7, 0.25);

    EXPECT_TRUE (std::isfinite (korg.alpha0));
    EXPECT_TRUE (std::isfinite (korg.feedback));
    EXPECT_TRUE (std::isfinite (moog.alpha0));
    EXPECT_TRUE (std::isfinite (moog.feedback));
    EXPECT_TRUE (std::isfinite (diode.fg));
    EXPECT_TRUE (std::isfinite (diode.g0));
}

TEST (AnalogFilterDesignerTests, DesignsVowelFormants)
{
    const auto vowel = AnalogFilterDesigner<double>::designVowel (0.35, sampleRate, 0.8);

    EXPECT_TRUE (std::isfinite (vowel.gainCompensation));

    for (const auto& formant : vowel.formants)
    {
        EXPECT_TRUE (std::isfinite (formant.g));
        EXPECT_TRUE (std::isfinite (formant.h));
        EXPECT_GT (formant.gainCorrection, 0.0);
    }
}

//==============================================================================

TEST (AnalogFilterTests, TwoPoleProcessesFiniteOutput)
{
    AnalogTwoPoleFilter<float> filter;
    filter.prepare (sampleRate, blockSize);
    filter.setParameters (FilterMode::bandpassCpg, 1000.0f, 0.75f, 0.2f, sampleRate);

    const auto input = makeInput<float>();
    std::vector<float> output (blockSize);

    filter.processBlock (input.data(), output.data(), blockSize);

    expectFiniteBuffer (output);
}

TEST (AnalogFilterTests, VowelProcessesFiniteOutput)
{
    AnalogVowelFilter<float> filter;
    filter.prepare (sampleRate, blockSize);
    filter.setParameters (0.5f, 0.75f, 0.2f, sampleRate);

    const auto input = makeInput<float>();
    std::vector<float> output (blockSize);

    filter.processBlock (input.data(), output.data(), blockSize);

    expectFiniteBuffer (output);
}

TEST (AnalogFilterTests, Korg35ProcessesAllModes)
{
    const auto input = makeInput<float>();

    for (auto mode : { FilterMode::lowpass, FilterMode::bandpassCsg, FilterMode::highpass })
    {
        AnalogKorg35Filter<float> filter;
        filter.prepare (sampleRate, blockSize);
        filter.setParameters (mode, 1000.0f, 0.6f, 0.2f, sampleRate);

        std::vector<float> output (blockSize);
        filter.processBlock (input.data(), output.data(), blockSize);

        expectFiniteBuffer (output);
    }
}

TEST (AnalogFilterTests, MoogLadderProcessesRepresentativeModes)
{
    const auto input = makeInput<float>();

    for (auto mode : { AnalogMoogLadderMode::lowpass24, AnalogMoogLadderMode::highpass12, AnalogMoogLadderMode::bandpass6 })
    {
        AnalogMoogLadderFilter<float> filter;
        filter.prepare (sampleRate, blockSize);
        filter.setParameters (mode, 1000.0f, 0.6f, 0.2f, sampleRate);

        std::vector<float> output (blockSize);
        filter.processBlock (input.data(), output.data(), blockSize);

        expectFiniteBuffer (output);
    }
}

TEST (AnalogFilterTests, RolandDiodeProcessesFiniteOutput)
{
    AnalogRolandDiodeFilter<float> filter;
    filter.prepare (sampleRate, blockSize);
    filter.setParameters (1000.0f, 0.6f, 0.2f, sampleRate);

    const auto input = makeInput<float>();
    std::vector<float> output (blockSize);

    filter.processBlock (input.data(), output.data(), blockSize);

    expectFiniteBuffer (output);
}

TEST (AnalogFilterTests, ResetRestoresDeterministicState)
{
    AnalogMoogLadderFilter<float> filter;
    filter.prepare (sampleRate, blockSize);
    filter.setParameters (AnalogMoogLadderMode::lowpass24, 1000.0f, 0.5f, 0.1f, sampleRate);

    const auto input = makeInput<float>();
    std::vector<float> firstOutput (blockSize);
    std::vector<float> secondOutput (blockSize);

    filter.processBlock (input.data(), firstOutput.data(), blockSize);
    filter.reset();
    filter.processBlock (input.data(), secondOutput.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
        EXPECT_FLOAT_EQ (firstOutput[static_cast<std::size_t> (i)], secondOutput[static_cast<std::size_t> (i)]);
}
