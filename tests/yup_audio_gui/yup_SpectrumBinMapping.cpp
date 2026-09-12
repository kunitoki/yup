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

#include <yup_audio_gui/yup_audio_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{

constexpr int spectrumTestFftSize = 2048;
constexpr double spectrumTestSampleRate = 44100.0;
constexpr float spectrumTestMinFrequency = 20.0f;
constexpr float spectrumTestMaxFrequency = 20000.0f;
constexpr int spectrumTestNumDisplayPoints = 512;

constexpr int spectrumTestNumBins()
{
    return spectrumTestFftSize / 2 + 1;
}

constexpr float spectrumTestBinsPerHertz()
{
    return static_cast<float> (spectrumTestFftSize) / static_cast<float> (spectrumTestSampleRate);
}

/** Levels expressed as a linear ramp on the amplitude decibel scale, from 0 dB at bin 0 to
    rangeDecibels at the last bin. Such a spectrum is reproduced exactly by the fractional level
    interpolation, which makes it a precise reference for it.
*/
std::vector<float> makeDecibelRampSpectrum (float rangeDecibels = -40.0f)
{
    std::vector<float> levels (static_cast<size_t> (spectrumTestNumBins()), 1.0f);
    const float lastBin = static_cast<float> (spectrumTestNumBins() - 1);

    for (int bin = 0; bin < spectrumTestNumBins(); ++bin)
    {
        const float decibels = rangeDecibels * static_cast<float> (bin) / lastBin;
        levels[static_cast<size_t> (bin)] = std::pow (10.0f, decibels / 20.0f);
    }

    return levels;
}

/** A single loud bin surrounded by a quiet floor, used to probe the interpolation across a peak. */
std::vector<float> makeImpulseSpectrum (int peakBin, float peakLevel = 1.0f, float floorLevel = 0.001f)
{
    std::vector<float> levels (static_cast<size_t> (spectrumTestNumBins()), floorLevel);
    levels[static_cast<size_t> (peakBin)] = peakLevel;

    return levels;
}

class SpectrumBinMappingTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mapping.setFftParameters (spectrumTestFftSize, spectrumTestSampleRate);
        mapping.setFrequencyRange (spectrumTestMinFrequency, spectrumTestMaxFrequency);
        mapping.setNumDisplayPoints (spectrumTestNumDisplayPoints);
    }

    SpectrumBinMapping mapping;
};

} // namespace

//==============================================================================
// Configuration
//==============================================================================

TEST_F (SpectrumBinMappingTests, DefaultConstructedMappingIsInvalid)
{
    SpectrumBinMapping emptyMapping;

    EXPECT_FALSE (emptyMapping.isValid());
    EXPECT_EQ (0, emptyMapping.getNumDisplayPoints());

    // Reading from an unconfigured mapping must be safe.
    EXPECT_FLOAT_EQ (0.0f, emptyMapping.getFrequencyForDisplayPoint (10));
    EXPECT_FLOAT_EQ (0.0f, emptyMapping.getRange (10).exactBin);
    EXPECT_FLOAT_EQ (0.0f, emptyMapping.getBandLevel ({}, 10, SpectrumBinMapping::BandAggregation::peak));
}

TEST_F (SpectrumBinMappingTests, ConfiguredMappingReportsItsParameters)
{
    EXPECT_TRUE (mapping.isValid());
    EXPECT_EQ (spectrumTestFftSize, mapping.getFftSize());
    EXPECT_DOUBLE_EQ (spectrumTestSampleRate, mapping.getSampleRate());
    EXPECT_FLOAT_EQ (spectrumTestMinFrequency, mapping.getMinFrequency());
    EXPECT_FLOAT_EQ (spectrumTestMaxFrequency, mapping.getMaxFrequency());
    EXPECT_EQ (spectrumTestNumDisplayPoints, mapping.getNumDisplayPoints());
}

TEST_F (SpectrumBinMappingTests, InvalidSettingsKeepTheMappingInvalid)
{
    SpectrumBinMapping invalidMapping;

    invalidMapping.setFftParameters (0, spectrumTestSampleRate);
    invalidMapping.setFrequencyRange (spectrumTestMinFrequency, spectrumTestMaxFrequency);
    invalidMapping.setNumDisplayPoints (spectrumTestNumDisplayPoints);
    EXPECT_FALSE (invalidMapping.isValid());

    invalidMapping.setFftParameters (spectrumTestFftSize, 0.0);
    EXPECT_FALSE (invalidMapping.isValid());

    invalidMapping.setFftParameters (spectrumTestFftSize, spectrumTestSampleRate);
    invalidMapping.setFrequencyRange (spectrumTestMaxFrequency, spectrumTestMinFrequency);
    EXPECT_FALSE (invalidMapping.isValid());

    invalidMapping.setFrequencyRange (spectrumTestMinFrequency, spectrumTestMaxFrequency);
    invalidMapping.setNumDisplayPoints (1);
    EXPECT_FALSE (invalidMapping.isValid());
}

TEST_F (SpectrumBinMappingTests, ChangingTheDisplayPointCountRebuildsEveryRange)
{
    mapping.setNumDisplayPoints (64);

    EXPECT_EQ (64, mapping.getNumDisplayPoints());

    const float binsPerHertz = spectrumTestBinsPerHertz();
    EXPECT_FLOAT_EQ (spectrumTestMinFrequency * binsPerHertz, mapping.getRange (0).startBin);
    EXPECT_NEAR (spectrumTestMaxFrequency * binsPerHertz, mapping.getRange (63).endBin, 0.01f);
    EXPECT_NEAR (spectrumTestMaxFrequency, mapping.getFrequencyForDisplayPoint (63), 1.0f);
}

TEST_F (SpectrumBinMappingTests, ChangingTheFftSizeScalesTheBinPositions)
{
    const float referenceBin = mapping.getRange (200).exactBin;

    mapping.setFftParameters (spectrumTestFftSize * 2, spectrumTestSampleRate);

    const float scaledBin = mapping.getRange (200).exactBin;
    EXPECT_NEAR (referenceBin * 2.0f, scaledBin, scaledBin * 1.0e-3f + 1.0e-4f);
}

TEST_F (SpectrumBinMappingTests, ChangingTheFrequencyRangeRescalesTheBands)
{
    mapping.setFrequencyRange (500.0f, 4000.0f);

    EXPECT_FLOAT_EQ (500.0f, mapping.getFrequencyForDisplayPoint (0));
    EXPECT_NEAR (4000.0f, mapping.getFrequencyForDisplayPoint (spectrumTestNumDisplayPoints - 1), 0.5f);
    EXPECT_NEAR (500.0f * spectrumTestBinsPerHertz(), mapping.getRange (0).startBin, 0.01f);
}

//==============================================================================
// Frequency axis > bin ranges
//==============================================================================

TEST_F (SpectrumBinMappingTests, FrequencyAxisSpansTheConfiguredRange)
{
    EXPECT_FLOAT_EQ (spectrumTestMinFrequency, mapping.getRange (0).centerFrequency);
    EXPECT_NEAR (spectrumTestMaxFrequency, mapping.getRange (spectrumTestNumDisplayPoints - 1).centerFrequency, 1.0f);

    EXPECT_FLOAT_EQ (spectrumTestMinFrequency * spectrumTestBinsPerHertz(), mapping.getRange (0).startBin);
    EXPECT_NEAR (spectrumTestMaxFrequency * spectrumTestBinsPerHertz(), mapping.getRange (spectrumTestNumDisplayPoints - 1).endBin, 0.01f);
}

TEST_F (SpectrumBinMappingTests, BandsTileTheFrequencyRangeWithoutGaps)
{
    for (int point = 1; point < mapping.getNumDisplayPoints(); ++point)
        EXPECT_NEAR (mapping.getRange (point - 1).endBin, mapping.getRange (point).startBin, 1.0e-4f);
}

TEST_F (SpectrumBinMappingTests, BandsGrowWithFrequencyOnALogarithmicAxis)
{
    float previousWidth = 0.0f;

    for (int point = 0; point < mapping.getNumDisplayPoints(); ++point)
    {
        const auto& range = mapping.getRange (point);
        const float width = range.endBin - range.startBin;

        EXPECT_GE (width, previousWidth);
        previousWidth = width;
    }
}

TEST_F (SpectrumBinMappingTests, ExactBinMatchesTheCentreFrequency)
{
    const float binsPerHertz = spectrumTestBinsPerHertz();

    for (int point = 0; point < mapping.getNumDisplayPoints(); ++point)
    {
        const auto& range = mapping.getRange (point);

        EXPECT_NEAR (range.centerFrequency * binsPerHertz, range.exactBin, range.exactBin * 1.0e-4f + 1.0e-6f);
        EXPECT_LE (range.startBin, range.exactBin);
        EXPECT_LE (range.exactBin, range.endBin);
    }
}

TEST_F (SpectrumBinMappingTests, RangeAccessClampsOutOfBoundsIndices)
{
    EXPECT_FLOAT_EQ (mapping.getRange (0).exactBin, mapping.getRange (-5).exactBin);
    EXPECT_FLOAT_EQ (mapping.getRange (spectrumTestNumDisplayPoints - 1).exactBin,
                     mapping.getRange (spectrumTestNumDisplayPoints + 5).exactBin);
}

//==============================================================================
// Fractional level interpolation
//==============================================================================

TEST_F (SpectrumBinMappingTests, InterpolatedLevelIsExactAtEveryBinCentre)
{
    const auto levels = makeDecibelRampSpectrum();

    for (int bin = 0; bin < spectrumTestNumBins(); ++bin)
        EXPECT_NEAR (levels[static_cast<size_t> (bin)],
                     SpectrumBinMapping::getInterpolatedLevel (levels, static_cast<float> (bin)),
                     1.0e-6f);
}

TEST_F (SpectrumBinMappingTests, InterpolatedLevelReproducesADecibelRamp)
{
    // A spectrum that is linear in decibels is reproduced exactly by the fractional interpolation.
    const auto levels = makeDecibelRampSpectrum (-60.0f);
    const float lastBin = static_cast<float> (spectrumTestNumBins() - 1);

    for (float position = 0.0f; position <= lastBin; position += 0.01f)
    {
        const float expected = -60.0f * position / lastBin;
        const float interpolated = 20.0f * std::log10 (SpectrumBinMapping::getInterpolatedLevel (levels, position));

        EXPECT_NEAR (expected, interpolated, 0.02f);
    }
}

TEST_F (SpectrumBinMappingTests, InterpolatedLevelChangesGraduallyAroundASteepPeak)
{
    // The neighbouring bins of this peak differ by 60 dB, which used to render as a single step.
    const auto levels = makeImpulseSpectrum (100);
    const float step = 0.001f;

    float previous = SpectrumBinMapping::getInterpolatedLevel (levels, 99.0f);
    float largestChange = 0.0f;

    for (float position = 99.0f + step; position <= 101.0f; position += step)
    {
        const float current = SpectrumBinMapping::getInterpolatedLevel (levels, position);

        largestChange = jmax (largestChange, std::abs (20.0f * std::log10 (current / previous)));
        previous = current;
    }

    EXPECT_LT (largestChange, 0.5f);
}

TEST_F (SpectrumBinMappingTests, InterpolatedLevelNeverSkipsAValueBetweenNeighbouringBins)
{
    // Consecutive fractions of a bin must never report the very same level: that is the flat
    // staircase the interpolation is meant to remove.
    const auto levels = makeDecibelRampSpectrum (-40.0f);
    const float lastBin = static_cast<float> (spectrumTestNumBins() - 1);
    const float step = 0.01f;

    int identicalNeighbours = 0;
    float previous = SpectrumBinMapping::getInterpolatedLevel (levels, 1.0f);

    for (float position = 1.0f + step; position <= lastBin - 1.0f; position += step)
    {
        const float current = SpectrumBinMapping::getInterpolatedLevel (levels, position);

        if (current == previous)
            ++identicalNeighbours;

        EXPECT_LE (current, previous);
        previous = current;
    }

    EXPECT_EQ (0, identicalNeighbours);
}

TEST_F (SpectrumBinMappingTests, InterpolatedLevelIsMonotoneOnAMonotoneSpectrum)
{
    std::vector<float> levels (static_cast<size_t> (spectrumTestNumBins()), 0.0f);

    for (int bin = 0; bin < spectrumTestNumBins(); ++bin)
        levels[static_cast<size_t> (bin)] = 0.05f + 0.9f * static_cast<float> (bin) / static_cast<float> (spectrumTestNumBins() - 1);

    float previous = 0.0f;

    for (float position = 0.0f; position <= static_cast<float> (spectrumTestNumBins() - 1); position += 0.01f)
    {
        const float current = SpectrumBinMapping::getInterpolatedLevel (levels, position);

        EXPECT_GE (current, previous - 1.0e-7f);
        previous = current;
    }
}

TEST_F (SpectrumBinMappingTests, InterpolatedLevelStaysWithinTheSurroundingBins)
{
    constexpr int peakBin = 512;
    const auto levels = makeImpulseSpectrum (peakBin);

    EXPECT_NEAR (1.0f, SpectrumBinMapping::getInterpolatedLevel (levels, static_cast<float> (peakBin)), 1.0e-6f);

    for (float position = peakBin - 2.0f; position <= peakBin + 2.0f; position += 0.005f)
    {
        const float level = SpectrumBinMapping::getInterpolatedLevel (levels, position);

        EXPECT_GE (level, 0.0f);
        EXPECT_LE (level, 1.0f + 1.0e-6f);
    }
}

TEST_F (SpectrumBinMappingTests, InterpolatedLevelClampsOutsideTheBinRange)
{
    const auto levels = makeDecibelRampSpectrum();

    EXPECT_FLOAT_EQ (levels.front(), SpectrumBinMapping::getInterpolatedLevel (levels, -12.0f));
    EXPECT_FLOAT_EQ (levels.back(), SpectrumBinMapping::getInterpolatedLevel (levels, static_cast<float> (spectrumTestNumBins()) + 30.0f));
}

TEST_F (SpectrumBinMappingTests, InterpolatedLevelHandlesEmptyAndSingleBinInputs)
{
    EXPECT_FLOAT_EQ (0.0f, SpectrumBinMapping::getInterpolatedLevel ({}, 4.0f));

    const std::vector<float> singleBin { 0.25f };
    EXPECT_FLOAT_EQ (0.25f, SpectrumBinMapping::getInterpolatedLevel (singleBin, 0.0f));
    EXPECT_FLOAT_EQ (0.25f, SpectrumBinMapping::getInterpolatedLevel (singleBin, 17.0f));
}

//==============================================================================
// Band aggregation
//==============================================================================

TEST_F (SpectrumBinMappingTests, ConstantSpectrumIntegratesToTheBandWidth)
{
    const std::vector<float> levels (static_cast<size_t> (spectrumTestNumBins()), 1.0f);
    const int points[] = { 0, 1, 100, 400, spectrumTestNumDisplayPoints - 1 };

    for (auto point : points)
    {
        const auto& range = mapping.getRange (point);
        const float bandWidth = range.endBin - range.startBin;

        EXPECT_NEAR (bandWidth, mapping.getBandLevel (levels, point, SpectrumBinMapping::BandAggregation::sum), bandWidth * 1.0e-3f + 1.0e-6f);
        EXPECT_NEAR (1.0f, mapping.getBandLevel (levels, point, SpectrumBinMapping::BandAggregation::mean), 1.0e-4f);
        EXPECT_NEAR (1.0f, mapping.getBandLevel (levels, point, SpectrumBinMapping::BandAggregation::peak), 1.0e-4f);
    }
}

TEST_F (SpectrumBinMappingTests, PeakAggregationFindsTheLoudestBinInsideTheBand)
{
    constexpr float peakBin = 100.0f;
    const auto levels = makeImpulseSpectrum (static_cast<int> (peakBin));

    // Find the display point whose band covers the peak bin.
    int coveringPoint = -1;

    for (int point = 0; point < mapping.getNumDisplayPoints(); ++point)
        if (mapping.getRange (point).startBin <= peakBin && peakBin <= mapping.getRange (point).endBin)
            coveringPoint = point;

    ASSERT_GE (coveringPoint, 0);
    EXPECT_NEAR (1.0f, mapping.getBandLevel (levels, coveringPoint, SpectrumBinMapping::BandAggregation::peak), 1.0e-4f);
}

TEST_F (SpectrumBinMappingTests, PeakAggregationIsBoundedByTheBandAndItsEdgeLevels)
{
    const auto levels = makeDecibelRampSpectrum (-30.0f);

    for (int point = 0; point < mapping.getNumDisplayPoints(); ++point)
    {
        const auto& range = mapping.getRange (point);

        const float edgePeak = jmax (SpectrumBinMapping::getInterpolatedLevel (levels, range.startBin),
                                     SpectrumBinMapping::getInterpolatedLevel (levels, range.endBin));

        float highestBinInside = 0.0f;

        for (int bin = (int) std::ceil (range.startBin); bin <= (int) std::floor (range.endBin); ++bin)
            highestBinInside = jmax (highestBinInside, levels[static_cast<size_t> (jlimit (0, spectrumTestNumBins() - 1, bin))]);

        const float bandPeak = mapping.getBandLevel (levels, point, SpectrumBinMapping::BandAggregation::peak);

        EXPECT_GE (bandPeak, edgePeak * (1.0f - 1.0e-5f));
        EXPECT_LE (bandPeak, jmax (edgePeak, highestBinInside) * (1.0f + 1.0e-5f));
    }
}

TEST_F (SpectrumBinMappingTests, MeanAggregationIsTheSumDividedByTheBandWidth)
{
    const auto levels = makeDecibelRampSpectrum();

    for (int point = 0; point < mapping.getNumDisplayPoints(); point += 32)
    {
        const auto& range = mapping.getRange (point);
        const float bandWidth = range.endBin - range.startBin;

        const float sum = mapping.getBandLevel (levels, point, SpectrumBinMapping::BandAggregation::sum);
        const float mean = mapping.getBandLevel (levels, point, SpectrumBinMapping::BandAggregation::mean);

        EXPECT_NEAR (sum / bandWidth, mean, sum * 1.0e-3f + 1.0e-6f);
    }
}

TEST_F (SpectrumBinMappingTests, BandLevelsAreSafeForOutOfRangeIndicesAndEmptyInputs)
{
    const std::vector<float> noLevels;

    EXPECT_FLOAT_EQ (0.0f, mapping.getBandLevel (noLevels, 0, SpectrumBinMapping::BandAggregation::peak));
    EXPECT_FLOAT_EQ (0.0f, mapping.getBandLevel ({}, -10, SpectrumBinMapping::BandAggregation::sum));

    const auto levels = makeDecibelRampSpectrum();
    EXPECT_FLOAT_EQ (0.0f, mapping.getBandLevel (levels, -10, SpectrumBinMapping::BandAggregation::peak));
    EXPECT_GT (mapping.getBandLevel (levels, 0, SpectrumBinMapping::BandAggregation::peak), 0.0f);
}

//==============================================================================
// Analyzer display scenario
//==============================================================================

TEST_F (SpectrumBinMappingTests, NeighbouringDisplayPointsNeverReportTheSameLevel)
{
    // A realistic configuration: 512 display points over 20 Hz > 20 kHz at 44.1 kHz, where the
    // lowest display bands are a fraction of an FFT bin wide. This is the region that used to
    // collapse into one flat value per bin.
    const auto levels = makeDecibelRampSpectrum (-45.0f);

    int identicalNeighbours = 0;
    float largestChange = 0.0f;

    for (int point = 1; point < mapping.getNumDisplayPoints(); ++point)
    {
        const float previous = mapping.getBandLevel (levels, point - 1, SpectrumBinMapping::BandAggregation::peak);
        const float current = mapping.getBandLevel (levels, point, SpectrumBinMapping::BandAggregation::peak);

        if (current == previous)
            ++identicalNeighbours;

        largestChange = jmax (largestChange, std::abs (20.0f * std::log10 (current / previous)));
    }

    EXPECT_EQ (0, identicalNeighbours);
    EXPECT_LT (largestChange, 1.0f);
}

TEST_F (SpectrumBinMappingTests, LowAndHighFrequencyBandsBothReportThePeakLevel)
{
    const auto levels = makeDecibelRampSpectrum (-45.0f);

    for (int point = 0; point < mapping.getNumDisplayPoints(); point += 64)
    {
        const auto& range = mapping.getRange (point);

        // The peak of this monotone spectrum always sits at the lower edge of the band.
        const float expected = SpectrumBinMapping::getInterpolatedLevel (levels, range.startBin);
        const float actual = mapping.getBandLevel (levels, point, SpectrumBinMapping::BandAggregation::peak);

        EXPECT_NEAR (expected, actual, expected * 1.0e-3f + 1.0e-9f);
    }
}

TEST_F (SpectrumBinMappingTests, SubBinBandsIntegrateLessPowerThanWideBands)
{
    // Two display points on a constant spectrum: the power integrated over a sub-bin band must stay
    // below the power integrated over band that spans several bins.
    const std::vector<float> levels (static_cast<size_t> (spectrumTestNumBins()), 1.0f);

    const auto& narrowRange = mapping.getRange (0);
    const auto& wideRange = mapping.getRange (spectrumTestNumDisplayPoints - 1);

    ASSERT_LT (narrowRange.endBin - narrowRange.startBin, 1.0f);
    ASSERT_GT (wideRange.endBin - wideRange.startBin, 1.0f);

    EXPECT_LT (mapping.getBandLevel (levels, 0, SpectrumBinMapping::BandAggregation::sum),
               mapping.getBandLevel (levels, spectrumTestNumDisplayPoints - 1, SpectrumBinMapping::BandAggregation::sum));

    // The average level of a constant spectrum is the same everywhere, whatever the band width.
    EXPECT_NEAR (mapping.getBandLevel (levels, 0, SpectrumBinMapping::BandAggregation::mean),
                 mapping.getBandLevel (levels, spectrumTestNumDisplayPoints - 1, SpectrumBinMapping::BandAggregation::mean),
                 1.0e-4f);
}
