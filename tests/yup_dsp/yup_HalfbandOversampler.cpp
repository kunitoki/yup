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

#include <cmath>
#include <type_traits>
#include <vector>

namespace yup::test
{

//==============================================================================
class HalfbandOversamplerTest : public ::testing::Test
{
protected:
    static constexpr double sampleRate = 48000.0;

    using Design = HalfbandOversampler<float, 2>::Design;

    static Design firDesign()
    {
        return {};
    }

    static Design iirDesign()
    {
        Design design;
        design.filterType = HalfbandFilterType::polyphaseIIR;
        return design;
    }

    template <typename SampleType>
    struct Streams
    {
        std::vector<SampleType> upsampled;
        std::vector<SampleType> roundTrip;
    };

    template <typename SampleType>
    static std::vector<SampleType> makeNoise (int numSamples, int64 seed)
    {
        Random random (seed);
        std::vector<SampleType> result (static_cast<std::size_t> (numSamples));

        for (auto& value : result)
            value = static_cast<SampleType> (random.nextDouble() * 2.0 - 1.0);

        return result;
    }

    template <typename SampleType>
    static std::vector<SampleType> makeSine (int numSamples, double normalizedFrequency, int offset = 0)
    {
        std::vector<SampleType> result (static_cast<std::size_t> (numSamples));

        for (int i = 0; i < numSamples; ++i)
            result[static_cast<std::size_t> (i)] = static_cast<SampleType> (std::sin (MathConstants<double>::twoPi * normalizedFrequency * (i + offset)));

        return result;
    }

    /** Streams the input through upsample/downsample using the block sizes in schedule (cycled). */
    template <typename Os, typename SampleType>
    static Streams<SampleType> process (Os& os, const std::vector<SampleType>& input, const std::vector<int>& schedule)
    {
        Streams<SampleType> streams;
        const auto total = static_cast<int> (input.size());
        std::size_t step = 0;

        for (int position = 0; position < total;)
        {
            const int numSamples = jmin (schedule[step++ % schedule.size()], total - position);

            const SampleType* inputPtrs[] = { input.data() + position };
            os.upsample (inputPtrs, 1, numSamples);

            const auto* up = os.getOversampledChannelData (0);
            streams.upsampled.insert (streams.upsampled.end(), up, up + os.getOversampledNumSamples());

            std::vector<SampleType> output (static_cast<std::size_t> (numSamples));
            SampleType* outputPtrs[] = { output.data() };
            os.downsample (outputPtrs, 1, numSamples);
            streams.roundTrip.insert (streams.roundTrip.end(), output.begin(), output.end());

            position += numSamples;
        }

        return streams;
    }

    template <typename SampleType>
    static void expectNear (const std::vector<SampleType>& actual, const std::vector<SampleType>& expected, double tolerance)
    {
        ASSERT_EQ (expected.size(), actual.size());

        for (std::size_t i = 0; i < actual.size(); ++i)
            ASSERT_NEAR (expected[i], actual[i], tolerance) << "at index " << i;
    }

    static double rms (const float* data, int numSamples)
    {
        return FloatVectorOperations::rms (data, numSamples);
    }

    static std::vector<double> magnitudeSpectrum (const std::vector<double>& signal)
    {
        const auto size = static_cast<int> (signal.size());
        FFTProcessor<double> fft (size);

        std::vector<double> spectrum (static_cast<std::size_t> (size) * 2);
        fft.performRealFFTForward (signal.data(), spectrum.data());

        std::vector<double> magnitude (static_cast<std::size_t> (size) / 2 + 1);

        for (int k = 0; k <= size / 2; ++k)
            magnitude[static_cast<std::size_t> (k)] = std::hypot (spectrum[static_cast<std::size_t> (2 * k)], spectrum[static_cast<std::size_t> (2 * k + 1)]);

        return magnitude;
    }

    /** Level of the strongest bin outside the fundamental's guard band, relative to the fundamental. */
    static double worstSpuriousDb (const std::vector<double>& magnitude, int fundamentalBin, int guardBins)
    {
        const auto fundamental = magnitude[static_cast<std::size_t> (fundamentalBin)];
        double worst = 0.0;

        for (std::size_t k = 0; k < magnitude.size(); ++k)
        {
            if (std::abs (static_cast<int> (k) - fundamentalBin) <= guardBins)
                continue;

            worst = jmax (worst, magnitude[k]);
        }

        return 20.0 * std::log10 (jmax (worst, 1e-12 * fundamental) / fundamental);
    }

    /** Upsamples a bin-exact tone and returns the strongest image relative to it, in dB. */
    template <int Factor>
    static double upsampledWorstImageDb (const Design& design, int fundamentalBin)
    {
        constexpr int blockSize = 1024;
        const double frequency = fundamentalBin / static_cast<double> (blockSize);

        HalfbandOversampler<float, Factor> os;
        os.prepare (sampleRate, 1, blockSize, design);

        std::vector<double> steadyState;

        for (int block = 0; block < 3; ++block)
        {
            const auto input = makeSine<float> (blockSize, frequency, block * blockSize);
            const float* inputPtrs[] = { input.data() };
            os.upsample (inputPtrs, 1, blockSize);

            const auto* up = os.getOversampledChannelData (0);
            steadyState.assign (up, up + os.getOversampledNumSamples());

            std::vector<float> output (blockSize);
            float* outputPtrs[] = { output.data() };
            os.downsample (outputPtrs, 1, blockSize);
        }

        return worstSpuriousDb (magnitudeSpectrum (steadyState), fundamentalBin, 2);
    }

    /** Generates a tone in the oversampled domain and returns the decimated level in dB relative to it. */
    template <int Factor>
    static double decimatedToneLevelDb (const Design& design, double frequencyRatioOfInputRate)
    {
        constexpr int blockSize = 2048;

        HalfbandOversampler<float, Factor> os;
        os.prepare (sampleRate, 1, blockSize, design);

        std::vector<float> output (blockSize);
        float* outputPtrs[] = { output.data() };

        for (int block = 0; block < 2; ++block)
        {
            if (! os.beginGeneration (1, blockSize))
            {
                ADD_FAILURE() << "beginGeneration refused a block within the prepared capacity";
                return 0.0;
            }

            auto* internal = os.getOversampledChannelData (0);

            for (int i = 0; i < os.getOversampledNumSamples(); ++i)
                internal[i] = static_cast<float> (std::sin (MathConstants<double>::twoPi * frequencyRatioOfInputRate * (block * blockSize * Factor + i) / Factor));

            os.downsample (outputPtrs, 1, blockSize);
        }

        return 20.0 * std::log10 (jmax (rms (output.data(), blockSize), 1e-12) * MathConstants<double>::sqrt2);
    }

    struct RoundTripAccuracy
    {
        double maxError = 0.0;
        double snrDb = 0.0;
        double amplitudeRatio = 0.0;
    };

    /** Round-trips a unit sine and compares it with the input delayed by the reported latency. */
    template <int Factor>
    static RoundTripAccuracy roundTripAccuracy (const Design& design, double normalizedFrequency)
    {
        constexpr int blockSize = 1024;

        HalfbandOversampler<float, Factor> os;
        os.prepare (sampleRate, 1, blockSize, design);

        RoundTripAccuracy accuracy;
        double signalEnergy = 0.0;
        double errorEnergy = 0.0;
        double outputEnergy = 0.0;

        for (int block = 0; block < 4; ++block)
        {
            const auto input = makeSine<float> (blockSize, normalizedFrequency, block * blockSize);
            const float* inputPtrs[] = { input.data() };
            os.upsample (inputPtrs, 1, blockSize);

            std::vector<float> output (blockSize);
            float* outputPtrs[] = { output.data() };
            os.downsample (outputPtrs, 1, blockSize);

            if (block == 0)
                continue;

            for (int i = 0; i < blockSize; ++i)
            {
                const auto expected = std::sin (MathConstants<double>::twoPi * normalizedFrequency * (block * blockSize + i - os.getLatencyInSamples()));
                const auto error = output[static_cast<std::size_t> (i)] - expected;
                accuracy.maxError = jmax (accuracy.maxError, std::abs (error));
                signalEnergy += expected * expected;
                errorEnergy += error * error;
                outputEnergy += output[static_cast<std::size_t> (i)] * output[static_cast<std::size_t> (i)];
            }
        }

        accuracy.snrDb = 10.0 * std::log10 (signalEnergy / jmax (errorEnergy, 1e-30));
        accuracy.amplitudeRatio = std::sqrt (outputEnergy / signalEnergy);
        return accuracy;
    }

    /** Frequency of an FFT bin for the 1024-sample blocks used by the round-trip helpers. */
    static constexpr double bin (int index) noexcept
    {
        return index / 1024.0;
    }
};

//==============================================================================
TEST_F (HalfbandOversamplerTest, StageCountFollowsTheFactor)
{
    EXPECT_EQ (1, (HalfbandOversampler<float, 2>::numStages));
    EXPECT_EQ (2, (HalfbandOversampler<float, 4>::numStages));
    EXPECT_EQ (3, (HalfbandOversampler<float, 8>::numStages));
    EXPECT_EQ (5, (HalfbandOversampler<float, 32>::numStages));
}

TEST_F (HalfbandOversamplerTest, AliasesUseTheHalfbandDesign)
{
    static_assert (std::is_same_v<HalfbandOversampler4xFloat, HalfbandOversampler<float, 4>>);
    static_assert (std::is_same_v<HalfbandOversampler32xDouble, HalfbandOversampler<double, 32>>);

    HalfbandOversampler2xFloat a;
    HalfbandOversampler4xFloat b;
    HalfbandOversampler8xFloat c;
    HalfbandOversampler16xFloat d;
    HalfbandOversampler32xFloat e;
    HalfbandOversampler2xDouble f;
    HalfbandOversampler4xDouble g;
    HalfbandOversampler8xDouble h;
    HalfbandOversampler16xDouble i;
    HalfbandOversampler32xDouble j;

    const auto prepareAll = [] (auto&... oversamplers)
    {
        (oversamplers.prepare (44100.0, 1, 64), ...);
    };

    prepareAll (a, b, c, d, e, f, g, h, i, j);

    EXPECT_EQ (HalfbandFilterType::linearPhaseFIR, a.getDesign().filterType);
    EXPECT_GT (e.getLatencyInSamples(), 0);
}

TEST_F (HalfbandOversamplerTest, DefaultConstructionReportsNoPendingBlock)
{
    HalfbandOversampler<float, 4> os;
    EXPECT_EQ (0, os.getOversampledNumSamples());
    EXPECT_EQ (nullptr, os.getOversampledChannelData (0));
    EXPECT_FALSE (os.beginGeneration (1, 16));
}

TEST_F (HalfbandOversamplerTest, FirLatencyIsAnIntegerRoundTripOfTwoGenerationDelays)
{
    HalfbandOversampler<float, 4> os;
    os.prepare (sampleRate, 1, 256);

    EXPECT_GT (os.getGenerationLatencyInSamples(), 0);
    EXPECT_EQ (2 * os.getGenerationLatencyInSamples(), os.getLatencyInSamples());

    HalfbandOversampler<float, 32> wide;
    wide.prepare (sampleRate, 1, 256);
    EXPECT_GE (wide.getLatencyInSamples(), os.getLatencyInSamples());
}

TEST_F (HalfbandOversamplerTest, FirstStageIsTheLongest)
{
    HalfbandOversampler<float, 8> os;
    os.prepare (sampleRate, 1, 256);

    EXPECT_GT (os.getStageFilterOrder (0), os.getStageFilterOrder (1));
    EXPECT_GE (os.getStageFilterOrder (1), os.getStageFilterOrder (2));
    EXPECT_EQ (1, os.getStageFilterOrder (0) % 2);
    EXPECT_EQ (3, os.getStageFilterOrder (1) % 4);
    EXPECT_EQ (0, os.getStageFilterOrder (3));
}

TEST_F (HalfbandOversamplerTest, HalfbandFirstStageIsCheaperButFoldsBackAboveNyquist)
{
    Design halfband;
    halfband.stopbandEdge = 1.0 - halfband.passbandEdge;

    HalfbandOversampler<float, 4> strict, relaxed;
    strict.prepare (sampleRate, 1, 256);
    relaxed.prepare (sampleRate, 1, 256, halfband);

    EXPECT_EQ (3, relaxed.getStageFilterOrder (0) % 4);
    EXPECT_LT (relaxed.getStageFilterOrder (0), strict.getStageFilterOrder (0));
    EXPECT_LT (relaxed.getLatencyInSamples(), strict.getLatencyInSamples());

    // A tone just above Nyquist: rejected by the default, folded back by the halfband.
    EXPECT_LT (decimatedToneLevelDb<2> (firDesign(), 0.52), -95.0);
    EXPECT_GT (decimatedToneLevelDb<2> (halfband, 0.52), -40.0);
}

TEST_F (HalfbandOversamplerTest, HigherStagesProtectTheWholeBandUpToNyquist)
{
    // 1.52 fs at 4x folds to 0.48 fs at the second stage: inside the base band
    // but above the passband edge, so only a Nyquist-protecting stage rejects it.
    EXPECT_LT (decimatedToneLevelDb<4> (firDesign(), 1.52), -95.0);
    EXPECT_LT (decimatedToneLevelDb<8> (firDesign(), 3.52), -95.0);
}

TEST_F (HalfbandOversamplerTest, WiderTransitionAndLowerAttenuationShortenTheFilters)
{
    HalfbandOversampler<float, 4> reference, relaxedEdge, relaxedAttenuation, stricter;

    Design edge;
    edge.passbandEdge = 0.40;

    Design attenuation;
    attenuation.stopbandAttenuationDb = 80.0;

    Design strict;
    strict.stopbandAttenuationDb = 120.0;

    reference.prepare (sampleRate, 1, 256);
    relaxedEdge.prepare (sampleRate, 1, 256, edge);
    relaxedAttenuation.prepare (sampleRate, 1, 256, attenuation);
    stricter.prepare (sampleRate, 1, 256, strict);

    EXPECT_LT (relaxedEdge.getLatencyInSamples(), reference.getLatencyInSamples());
    EXPECT_LT (relaxedAttenuation.getStageFilterOrder (0), reference.getStageFilterOrder (0));
    EXPECT_GT (stricter.getStageFilterOrder (0), reference.getStageFilterOrder (0));
    EXPECT_GT (stricter.getLatencyInSamples(), reference.getLatencyInSamples());
}

TEST_F (HalfbandOversamplerTest, IirIsCheaperAndHasLowerLatencyThanFir)
{
    HalfbandOversampler<float, 4> fir, iir;
    fir.prepare (sampleRate, 1, 256, firDesign());
    iir.prepare (sampleRate, 1, 256, iirDesign());

    EXPECT_LT (iir.getStageFilterOrder (0), fir.getStageFilterOrder (0));
    EXPECT_LT (iir.getLatencyInSamples(), fir.getLatencyInSamples());
    EXPECT_GT (iir.getLatencyInSamples(), 0);
    EXPECT_EQ (1, iir.getStageFilterOrder (0) % 2);
}

TEST_F (HalfbandOversamplerTest, InvalidGenerationRequestsPreserveThePendingBlock)
{
    HalfbandOversampler<float, 4> os;
    os.prepare (sampleRate, 2, 256);

    ASSERT_TRUE (os.beginGeneration (1, 16));
    EXPECT_FALSE (os.beginGeneration (0, 16));
    EXPECT_FALSE (os.beginGeneration (1, 0));
    EXPECT_FALSE (os.beginGeneration (3, 16));
    EXPECT_FALSE (os.beginGeneration (1, 257));
    EXPECT_EQ (64, os.getOversampledNumSamples());
}

TEST_F (HalfbandOversamplerTest, ChannelCapacityDoesNotShrinkAfterAMonoBlock)
{
    HalfbandOversampler<float, 2> os;
    os.prepare (sampleRate, 2, 64);

    std::vector<float> mono (64, 0.5f);
    const float* monoPtrs[] = { mono.data() };
    std::vector<float> out (64);
    float* outPtrs[] = { out.data() };
    os.upsample (monoPtrs, 1, 64);
    os.downsample (outPtrs, 1, 64);

    EXPECT_TRUE (os.beginGeneration (2, 64));
    EXPECT_NE (nullptr, os.getOversampledChannelData (1));
}

//==============================================================================
TEST_F (HalfbandOversamplerTest, FirImpulsePeaksAtTheReportedLatencies)
{
    constexpr int total = 320;
    constexpr int impulsePosition = 11;

    std::vector<float> input (total, 0.0f);
    input[impulsePosition] = 1.0f;

    HalfbandOversampler<float, 4> os;
    os.prepare (sampleRate, 1, total);
    const auto streams = process (os, input, { 7 });

    const auto roundTripPeak = std::max_element (streams.roundTrip.begin(), streams.roundTrip.end());
    EXPECT_EQ (impulsePosition + os.getLatencyInSamples(), static_cast<int> (roundTripPeak - streams.roundTrip.begin()));
    EXPECT_NEAR (0.9, *roundTripPeak, 0.1);

    const auto upsampledPeak = std::max_element (streams.upsampled.begin(), streams.upsampled.end());
    EXPECT_EQ (4 * (impulsePosition + os.getGenerationLatencyInSamples()), static_cast<int> (upsampledPeak - streams.upsampled.begin()));

    os.reset();
    ASSERT_TRUE (os.beginGeneration (1, total));
    auto* internal = os.getOversampledChannelData (0);
    FloatVectorOperations::clear (internal, total * 4);
    internal[0] = 1.0f;

    std::vector<float> generated (total);
    float* generatedPtrs[] = { generated.data() };
    os.downsample (generatedPtrs, 1, total);

    const auto generatedPeak = std::max_element (generated.begin(), generated.end());
    EXPECT_EQ (os.getGenerationLatencyInSamples(), static_cast<int> (generatedPeak - generated.begin()));
}

TEST_F (HalfbandOversamplerTest, DcIsPreservedExactlyByBothFamilies)
{
    for (const auto& design : { firDesign(), iirDesign() })
    {
        constexpr int blockSize = 256;
        HalfbandOversampler<float, 8> os;
        os.prepare (sampleRate, 1, blockSize, design);

        std::vector<float> input (blockSize, 0.5f);
        std::vector<float> output (blockSize);
        const float* inputPtrs[] = { input.data() };
        float* outputPtrs[] = { output.data() };

        for (int block = 0; block < 4; ++block)
        {
            os.upsample (inputPtrs, 1, blockSize);

            if (block == 3)
            {
                const auto* up = os.getOversampledChannelData (0);

                for (int i = 0; i < os.getOversampledNumSamples(); ++i)
                    ASSERT_NEAR (0.5f, up[i], 1e-5f) << "oversampled index " << i;
            }

            os.downsample (outputPtrs, 1, blockSize);
        }

        for (int i = 0; i < blockSize; ++i)
            ASSERT_NEAR (0.5f, output[static_cast<std::size_t> (i)], 1e-5f) << "output index " << i;
    }
}

TEST_F (HalfbandOversamplerTest, BlockSizeIndependence)
{
    for (const auto& design : { firDesign(), iirDesign() })
    {
        constexpr int total = 512;
        const auto input = makeNoise<float> (total, 7);

        HalfbandOversampler<float, 8> wholeBlock, irregularBlocks;
        wholeBlock.prepare (sampleRate, 1, total, design);
        irregularBlocks.prepare (sampleRate, 1, total, design);

        const auto reference = process (wholeBlock, input, { total });
        const auto irregular = process (irregularBlocks, input, { 1, 3, 7, 5, 2, 13, 64, 17, 31, 9, 128 });

        expectNear (irregular.upsampled, reference.upsampled, 1e-6);
        expectNear (irregular.roundTrip, reference.roundTrip, 1e-6);
    }
}

TEST_F (HalfbandOversamplerTest, ChannelsAreIndependent)
{
    constexpr int blockSize = 100;
    constexpr int total = 300;
    const auto left = makeNoise<float> (total, 1);
    const auto right = makeNoise<float> (total, 2);

    for (const auto& design : { firDesign(), iirDesign() })
    {
        HalfbandOversampler<float, 4> stereo, monoRight;
        stereo.prepare (sampleRate, 2, blockSize, design);
        monoRight.prepare (sampleRate, 1, blockSize, design);

        Streams<float> stereoRight;

        for (int position = 0; position < total; position += blockSize)
        {
            const float* inputPtrs[] = { left.data() + position, right.data() + position };
            stereo.upsample (inputPtrs, 2, blockSize);

            const auto* upRight = stereo.getOversampledChannelData (1);
            stereoRight.upsampled.insert (stereoRight.upsampled.end(), upRight, upRight + stereo.getOversampledNumSamples());

            std::vector<float> outLeft (blockSize), outRight (blockSize);
            float* outputPtrs[] = { outLeft.data(), outRight.data() };
            stereo.downsample (outputPtrs, 2, blockSize);
            stereoRight.roundTrip.insert (stereoRight.roundTrip.end(), outRight.begin(), outRight.end());
        }

        const auto expectedRight = process (monoRight, right, { blockSize });
        expectNear (stereoRight.upsampled, expectedRight.upsampled, 1e-7);
        expectNear (stereoRight.roundTrip, expectedRight.roundTrip, 1e-7);
    }
}

TEST_F (HalfbandOversamplerTest, ResetMatchesFreshInstance)
{
    for (const auto& design : { firDesign(), iirDesign() })
    {
        constexpr int blockSize = 128;

        HalfbandOversampler<float, 4> reused, fresh;
        reused.prepare (sampleRate, 1, blockSize, design);
        fresh.prepare (sampleRate, 1, blockSize, design);

        process (reused, makeNoise<float> (512, 5), { blockSize });
        reused.reset();

        const auto input = makeNoise<float> (256, 6);
        const auto reusedResult = process (reused, input, { blockSize });
        const auto freshResult = process (fresh, input, { blockSize });

        expectNear (reusedResult.upsampled, freshResult.upsampled, 1e-7);
        expectNear (reusedResult.roundTrip, freshResult.roundTrip, 1e-7);
    }
}

TEST_F (HalfbandOversamplerTest, GenerationDoesNotDisturbUpsampleHistory)
{
    constexpr int blockSize = 128;
    const auto blockA = makeNoise<float> (blockSize, 8);
    const auto blockB = makeNoise<float> (blockSize, 9);

    HalfbandOversampler<float, 4> withGeneration, withoutGeneration;
    withGeneration.prepare (sampleRate, 1, blockSize);
    withoutGeneration.prepare (sampleRate, 1, blockSize);

    const auto roundTrip = [] (auto& os, const std::vector<float>& input)
    {
        const float* inputPtrs[] = { input.data() };
        os.upsample (inputPtrs, 1, blockSize);

        const auto* up = os.getOversampledChannelData (0);
        std::vector<float> upsampled (up, up + os.getOversampledNumSamples());

        std::vector<float> output (blockSize);
        float* outputPtrs[] = { output.data() };
        os.downsample (outputPtrs, 1, blockSize);
        return upsampled;
    };

    roundTrip (withGeneration, blockA);
    roundTrip (withoutGeneration, blockA);

    ASSERT_TRUE (withGeneration.beginGeneration (1, blockSize));
    FloatVectorOperations::fill (withGeneration.getOversampledChannelData (0), 0.7f, withGeneration.getOversampledNumSamples());
    std::vector<float> generated (blockSize);
    float* generatedPtrs[] = { generated.data() };
    withGeneration.downsample (generatedPtrs, 1, blockSize);

    expectNear (roundTrip (withGeneration, blockB), roundTrip (withoutGeneration, blockB), 1e-7);
}

//==============================================================================
TEST_F (HalfbandOversamplerTest, UpsampledImagesAreRejected)
{
    for (const auto& design : { firDesign(), iirDesign() })
    {
        // 0.25 fs: images at 0.75, 1.25 and 1.75 fs, deep in every stage's stopband.
        EXPECT_LT (upsampledWorstImageDb<4> (design, 256), -95.0);

        // 0.4 fs: the nearest image at 0.6 fs sits just past the 0.55 fs stopband edge.
        EXPECT_LT (upsampledWorstImageDb<4> (design, 410), -95.0);

        // 8x: the higher stages must not let their own images through either.
        EXPECT_LT (upsampledWorstImageDb<8> (design, 300), -95.0);
    }
}

TEST_F (HalfbandOversamplerTest, DecimationRejectsTonesAboveTheStopbandEdge)
{
    for (const auto& design : { firDesign(), iirDesign() })
    {
        EXPECT_LT (decimatedToneLevelDb<2> (design, 0.6), -95.0);
        EXPECT_LT (decimatedToneLevelDb<4> (design, 0.9), -95.0);
        EXPECT_LT (decimatedToneLevelDb<8> (design, 1.7), -95.0);
        EXPECT_LT (decimatedToneLevelDb<8> (design, 3.7), -95.0);
    }
}

TEST_F (HalfbandOversamplerTest, FirPassbandIsFlatAndLinearPhaseUpToTheDesignEdge)
{
    EXPECT_LT (roundTripAccuracy<4> (firDesign(), bin (20)).maxError, 1e-3);
    EXPECT_LT (roundTripAccuracy<4> (firDesign(), bin (307)).maxError, 1e-3);
    EXPECT_LT (roundTripAccuracy<4> (firDesign(), bin (450)).maxError, 1e-3);
    EXPECT_GT (roundTripAccuracy<4> (firDesign(), bin (102)).snrDb, 95.0);
}

TEST_F (HalfbandOversamplerTest, IirPassbandAmplitudeIsFlatUpToTheDesignEdge)
{
    EXPECT_NEAR (1.0, roundTripAccuracy<4> (iirDesign(), bin (20)).amplitudeRatio, 1e-3);
    EXPECT_NEAR (1.0, roundTripAccuracy<4> (iirDesign(), bin (307)).amplitudeRatio, 1e-3);
    EXPECT_NEAR (1.0, roundTripAccuracy<4> (iirDesign(), bin (448)).amplitudeRatio, 1e-3);
}

TEST_F (HalfbandOversamplerTest, LowerAttenuationTargetIsStillMet)
{
    Design design;
    design.stopbandAttenuationDb = 60.0;

    EXPECT_LT (upsampledWorstImageDb<4> (design, 256), -58.0);
    EXPECT_LT (decimatedToneLevelDb<2> (design, 0.6), -58.0);
}

} // namespace yup::test
