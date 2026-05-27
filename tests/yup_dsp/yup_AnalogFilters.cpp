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

#include <array>

#include <gtest/gtest.h>

using namespace yup;

namespace
{
constexpr double analogSampleRate = 44100.0;
constexpr int analogBlockSize = 128;
constexpr std::array<double, 4> analogResponseFrequencies { 0.0, 100.0, 1000.0, 10000.0 };

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

template <typename CoeffType>
void expectFiniteAnalogResponse (const Complex<CoeffType>& response)
{
    EXPECT_TRUE (std::isfinite (response.real()));
    EXPECT_TRUE (std::isfinite (response.imag()));
    EXPECT_TRUE (std::isfinite (std::abs (response)));
}

template <typename FilterType>
void expectFiniteAnalogResponses (const FilterType& filter)
{
    for (auto frequency : analogResponseFrequencies)
        expectFiniteAnalogResponse (filter.getComplexResponse (frequency));
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

TEST (AnalogFilterTests, TwoPoleComplexResponseCoversSupportedModes)
{
    for (auto mode : { FilterMode::lowpass, FilterMode::highpass, FilterMode::bandpassCsg, FilterMode::bandpassCpg, FilterMode::bandstop, FilterMode::peak })
    {
        AnalogTwoPoleFilter<double> filter;
        filter.prepare (analogSampleRate, analogBlockSize);
        filter.setParameters (mode, 1000.0, 0.65, 0.0, analogSampleRate);

        expectFiniteAnalogResponses (filter);
    }
}

TEST (AnalogFilterTests, TwoPoleComplexResponseMatchesModeShape)
{
    AnalogTwoPoleFilter<double> lowpass;
    lowpass.prepare (analogSampleRate, analogBlockSize);
    lowpass.setParameters (FilterMode::lowpass, 1000.0, 0.35, 0.0, analogSampleRate);
    EXPECT_GT (std::abs (lowpass.getComplexResponse (100.0)), std::abs (lowpass.getComplexResponse (10000.0)));

    AnalogTwoPoleFilter<double> highpass;
    highpass.prepare (analogSampleRate, analogBlockSize);
    highpass.setParameters (FilterMode::highpass, 1000.0, 0.35, 0.0, analogSampleRate);
    EXPECT_LT (std::abs (highpass.getComplexResponse (100.0)), std::abs (highpass.getComplexResponse (10000.0)));

    AnalogTwoPoleFilter<double> bandpass;
    bandpass.prepare (analogSampleRate, analogBlockSize);
    bandpass.setParameters (FilterMode::bandpassCpg, 1000.0, 0.75, 0.0, analogSampleRate);

    const auto centerResponse = std::abs (bandpass.getComplexResponse (1000.0));
    EXPECT_GT (centerResponse, std::abs (bandpass.getComplexResponse (100.0)));
    EXPECT_GT (centerResponse, std::abs (bandpass.getComplexResponse (10000.0)));
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

TEST (AnalogFilterTests, VowelComplexResponseIsFinite)
{
    AnalogVowelFilter<double> filter;
    filter.prepare (analogSampleRate, analogBlockSize);
    filter.setParameters (0.5, 0.75, 0.0, analogSampleRate);

    expectFiniteAnalogResponses (filter);
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

TEST (AnalogFilterTests, Korg35ComplexResponseCoversAllModes)
{
    for (auto mode : { FilterMode::lowpass, FilterMode::bandpassCsg, FilterMode::highpass })
    {
        AnalogKorg35Filter<double> filter;
        filter.prepare (analogSampleRate, analogBlockSize);
        filter.setParameters (mode, 1000.0, 0.55, 0.0, analogSampleRate);

        expectFiniteAnalogResponses (filter);
    }
}

TEST (AnalogFilterTests, Korg35ComplexResponseMatchesModeShape)
{
    AnalogKorg35Filter<double> lowpass;
    lowpass.prepare (analogSampleRate, analogBlockSize);
    lowpass.setParameters (FilterMode::lowpass, 1000.0, 0.4, 0.0, analogSampleRate);
    EXPECT_GT (std::abs (lowpass.getComplexResponse (100.0)), std::abs (lowpass.getComplexResponse (10000.0)));

    AnalogKorg35Filter<double> highpass;
    highpass.prepare (analogSampleRate, analogBlockSize);
    highpass.setParameters (FilterMode::highpass, 1000.0, 0.4, 0.0, analogSampleRate);
    EXPECT_LT (std::abs (highpass.getComplexResponse (100.0)), std::abs (highpass.getComplexResponse (10000.0)));

    AnalogKorg35Filter<double> bandpass;
    bandpass.prepare (analogSampleRate, analogBlockSize);
    bandpass.setParameters (FilterMode::bandpassCsg, 1000.0, 0.55, 0.0, analogSampleRate);

    const auto centerResponse = std::abs (bandpass.getComplexResponse (1000.0));
    EXPECT_GT (centerResponse, std::abs (bandpass.getComplexResponse (100.0)));
    EXPECT_GT (centerResponse, std::abs (bandpass.getComplexResponse (10000.0)));
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

TEST (AnalogFilterTests, MoogLadderComplexResponseCoversAllModes)
{
    for (auto mode : { AnalogMoogLadderMode::lowpass24,
                       AnalogMoogLadderMode::highpass24,
                       AnalogMoogLadderMode::lowpass18,
                       AnalogMoogLadderMode::highpass18,
                       AnalogMoogLadderMode::lowpass12,
                       AnalogMoogLadderMode::highpass12,
                       AnalogMoogLadderMode::lowpass6,
                       AnalogMoogLadderMode::highpass6,
                       AnalogMoogLadderMode::bandpass12,
                       AnalogMoogLadderMode::bandpass6 })
    {
        AnalogMoogLadderFilter<double> filter;
        filter.prepare (analogSampleRate, analogBlockSize);
        filter.setParameters (mode, 1000.0, 0.55, 0.0, analogSampleRate);

        expectFiniteAnalogResponses (filter);
    }
}

TEST (AnalogFilterTests, MoogLadderComplexResponseMatchesModeShape)
{
    AnalogMoogLadderFilter<double> lowpass;
    lowpass.prepare (analogSampleRate, analogBlockSize);
    lowpass.setParameters (AnalogMoogLadderMode::lowpass24, 1000.0, 0.4, 0.0, analogSampleRate);
    EXPECT_GT (std::abs (lowpass.getComplexResponse (100.0)), std::abs (lowpass.getComplexResponse (10000.0)));

    AnalogMoogLadderFilter<double> highpass;
    highpass.prepare (analogSampleRate, analogBlockSize);
    highpass.setParameters (AnalogMoogLadderMode::highpass24, 1000.0, 0.4, 0.0, analogSampleRate);
    EXPECT_LT (std::abs (highpass.getComplexResponse (100.0)), std::abs (highpass.getComplexResponse (10000.0)));

    AnalogMoogLadderFilter<double> bandpass;
    bandpass.prepare (analogSampleRate, analogBlockSize);
    bandpass.setParameters (AnalogMoogLadderMode::bandpass12, 1000.0, 0.55, 0.0, analogSampleRate);

    const auto centerResponse = std::abs (bandpass.getComplexResponse (1000.0));
    EXPECT_GT (centerResponse, std::abs (bandpass.getComplexResponse (100.0)));
    EXPECT_GT (centerResponse, std::abs (bandpass.getComplexResponse (10000.0)));
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

TEST (AnalogFilterTests, RolandDiodeComplexResponseIsFiniteAndLowpass)
{
    AnalogRolandDiodeFilter<double> filter;
    filter.prepare (analogSampleRate, analogBlockSize);
    filter.setParameters (1000.0, 0.45, 0.0, analogSampleRate);

    expectFiniteAnalogResponses (filter);
    EXPECT_GT (std::abs (filter.getComplexResponse (100.0)), std::abs (filter.getComplexResponse (10000.0)));
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
