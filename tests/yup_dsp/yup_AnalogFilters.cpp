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
constexpr double analogSampleRate = 44100.0;
constexpr int analogBlockSize = 128;

template <typename SampleType>
std::vector<SampleType> makeAnalogInput()
{
    std::vector<SampleType> input (analogBlockSize);

    for (int i = 0; i < analogBlockSize; ++i)
        input[static_cast<std::size_t> (i)] = static_cast<SampleType> ((i % 17) / 17.0 - 0.5);

    return input;
}

template <typename SampleType>
void expectFiniteAnalogBuffer (const std::vector<SampleType>& buffer)
{
    for (auto sample : buffer)
        EXPECT_TRUE (std::isfinite (sample));
}
} // namespace

//==============================================================================

TEST (AnalogFilterDesignerTests, DesignsFiniteTwoPoleCoefficients)
{
    const auto lowpass = AnalogFilterDesigner<double>::designTwoPole (FilterMode::lowpass, 1000.0, analogSampleRate, 0.5);
    const auto bandpass = AnalogFilterDesigner<double>::designTwoPole (FilterMode::bandpassCpg, 1000.0, analogSampleRate, 0.5);
    const auto peak = AnalogFilterDesigner<double>::designTwoPole (FilterMode::peak, 1000.0, analogSampleRate, 0.5);

    EXPECT_TRUE (std::isfinite (lowpass.g));
    EXPECT_TRUE (std::isfinite (lowpass.h));
    EXPECT_TRUE (std::isfinite (bandpass.r2));
    EXPECT_TRUE (std::isfinite (peak.gainCorrection));
}

TEST (AnalogFilterDesignerTests, DesignsFiniteLadderCoefficients)
{
    const auto korg = AnalogFilterDesigner<double>::designKorg35 (FilterMode::lowpass, 1200.0, analogSampleRate, 0.7, 0.25);
    const auto moog = AnalogFilterDesigner<double>::designMoogLadder (AnalogMoogLadderMode::lowpass24, 1200.0, analogSampleRate, 0.7, 0.25);
    const auto diode = AnalogFilterDesigner<double>::designRolandDiode (1200.0, analogSampleRate, 0.7, 0.25);

    EXPECT_TRUE (std::isfinite (korg.alpha0));
    EXPECT_TRUE (std::isfinite (korg.feedback));
    EXPECT_TRUE (std::isfinite (moog.alpha0));
    EXPECT_TRUE (std::isfinite (moog.feedback));
    EXPECT_TRUE (std::isfinite (diode.fg));
    EXPECT_TRUE (std::isfinite (diode.g0));
}

TEST (AnalogFilterDesignerTests, DesignsVowelFormants)
{
    const auto vowel = AnalogFilterDesigner<double>::designVowel (0.35, analogSampleRate, 0.8);

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
    filter.prepare (analogSampleRate, analogBlockSize);
    filter.setParameters (FilterMode::bandpassCpg, 1000.0f, 0.75f, 0.2f, analogSampleRate);

    const auto input = makeAnalogInput<float>();
    std::vector<float> output (analogBlockSize);

    filter.processBlock (input.data(), output.data(), analogBlockSize);

    expectFiniteAnalogBuffer (output);
}

TEST (AnalogFilterTests, VowelProcessesFiniteOutput)
{
    AnalogVowelFilter<float> filter;
    filter.prepare (analogSampleRate, analogBlockSize);
    filter.setParameters (0.5f, 0.75f, 0.2f, analogSampleRate);

    const auto input = makeAnalogInput<float>();
    std::vector<float> output (analogBlockSize);

    filter.processBlock (input.data(), output.data(), analogBlockSize);

    expectFiniteAnalogBuffer (output);
}

TEST (AnalogFilterTests, Korg35ProcessesAllModes)
{
    const auto input = makeAnalogInput<float>();

    for (auto mode : { FilterMode::lowpass, FilterMode::bandpassCsg, FilterMode::highpass })
    {
        AnalogKorg35Filter<float> filter;
        filter.prepare (analogSampleRate, analogBlockSize);
        filter.setParameters (mode, 1000.0f, 0.6f, 0.2f, analogSampleRate);

        std::vector<float> output (analogBlockSize);
        filter.processBlock (input.data(), output.data(), analogBlockSize);

        expectFiniteAnalogBuffer (output);
    }
}

TEST (AnalogFilterTests, MoogLadderProcessesRepresentativeModes)
{
    const auto input = makeAnalogInput<float>();

    for (auto mode : { AnalogMoogLadderMode::lowpass24, AnalogMoogLadderMode::highpass12, AnalogMoogLadderMode::bandpass6 })
    {
        AnalogMoogLadderFilter<float> filter;
        filter.prepare (analogSampleRate, analogBlockSize);
        filter.setParameters (mode, 1000.0f, 0.6f, 0.2f, analogSampleRate);

        std::vector<float> output (analogBlockSize);
        filter.processBlock (input.data(), output.data(), analogBlockSize);

        expectFiniteAnalogBuffer (output);
    }
}

TEST (AnalogFilterTests, RolandDiodeProcessesFiniteOutput)
{
    AnalogRolandDiodeFilter<float> filter;
    filter.prepare (analogSampleRate, analogBlockSize);
    filter.setParameters (1000.0f, 0.6f, 0.2f, analogSampleRate);

    const auto input = makeAnalogInput<float>();
    std::vector<float> output (analogBlockSize);

    filter.processBlock (input.data(), output.data(), analogBlockSize);

    expectFiniteAnalogBuffer (output);
}

TEST (AnalogFilterTests, ResetRestoresDeterministicState)
{
    AnalogMoogLadderFilter<float> filter;
    filter.prepare (analogSampleRate, analogBlockSize);
    filter.setParameters (AnalogMoogLadderMode::lowpass24, 1000.0f, 0.5f, 0.1f, analogSampleRate);

    const auto input = makeAnalogInput<float>();
    std::vector<float> firstOutput (analogBlockSize);
    std::vector<float> secondOutput (analogBlockSize);

    filter.processBlock (input.data(), firstOutput.data(), analogBlockSize);
    filter.reset();
    filter.processBlock (input.data(), secondOutput.data(), analogBlockSize);

    for (int i = 0; i < analogBlockSize; ++i)
        EXPECT_FLOAT_EQ (firstOutput[static_cast<std::size_t> (i)], secondOutput[static_cast<std::size_t> (i)]);
}
