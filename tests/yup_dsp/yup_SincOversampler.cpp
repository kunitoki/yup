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
#include <vector>

namespace yup::test
{

//==============================================================================
class SincOversamplerTest : public ::testing::Test
{
protected:
    static constexpr double sampleRate = 44100.0;
    static constexpr int maxChannels = 2;
    static constexpr int blockSize = 256;

    void SetUp() override
    {
        os2x.prepare (sampleRate, maxChannels, blockSize);
        os4x.prepare (sampleRate, maxChannels, blockSize);
    }

    float calculateRMS (const float* data, int numSamples) const
    {
        float sum = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            sum += data[i] * data[i];
        return std::sqrt (sum / static_cast<float> (numSamples));
    }

    void fillSine (std::vector<float>& buf, float frequency, float amplitude = 1.0f) const
    {
        for (std::size_t i = 0; i < buf.size(); ++i)
        {
            buf[i] = amplitude * std::sin (MathConstants<float>::twoPi * frequency * static_cast<float> (i) / static_cast<float> (sampleRate));
        }
    }

    void fillDC (std::vector<float>& buf, float value) const
    {
        std::fill (buf.begin(), buf.end(), value);
    }

    SincOversampler<float, 2, 8> os2x;
    SincOversampler<float, 4, 8> os4x;
};

//==============================================================================
TEST_F (SincOversamplerTest, DefaultConstructionDoesNotCrash)
{
    SincOversampler<float, 2, 8> os;
    EXPECT_EQ (os.getOversampledNumSamples(), 0);
    EXPECT_EQ (os.getLatencyInSamples(), 16);
    EXPECT_EQ (os.getOversampledChannelData (0), nullptr);
}

TEST_F (SincOversamplerTest, PrepareAllocatesOversampledBuffer)
{
    EXPECT_EQ (os2x.getOversampledNumSamples(), 0);

    std::vector<float> ch0 (blockSize, 0.0f);
    const float* inputPtrs[] = { ch0.data() };
    os2x.upsample (inputPtrs, 1, blockSize);

    EXPECT_EQ (os2x.getOversampledNumSamples(), blockSize * 2);
    EXPECT_NE (os2x.getOversampledChannelData (0), nullptr);
}

TEST_F (SincOversamplerTest, LatencyReturnsCorrectValue)
{
    EXPECT_EQ (os2x.getLatencyInSamples(), 16); // 2 * SincRadius = 2 * 8
    EXPECT_EQ (os4x.getLatencyInSamples(), 16);
}

TEST_F (SincOversamplerTest, ResetClearsOversampledSize)
{
    std::vector<float> ch0 (blockSize, 1.0f);
    const float* inputPtrs[] = { ch0.data() };
    os2x.upsample (inputPtrs, 1, blockSize);

    ASSERT_EQ (os2x.getOversampledNumSamples(), blockSize * 2);

    os2x.reset();
    EXPECT_EQ (os2x.getOversampledNumSamples(), 0);
}

TEST_F (SincOversamplerTest, UpsampleDCSignalHasCorrectMagnitude)
{
    constexpr float dcValue = 0.5f;
    std::vector<float> ch0 (blockSize, dcValue);
    const float* inputPtrs[] = { ch0.data() };

    // Warm up the filter (several blocks to flush transient)
    for (int b = 0; b < 5; ++b)
        os2x.upsample (inputPtrs, 1, blockSize);

    const float* outData = os2x.getOversampledChannelData (0);
    ASSERT_NE (outData, nullptr);

    // Second half of the block should be at steady state
    const int halfSize = blockSize; // = blockSize * 2 / 2
    float rms = calculateRMS (outData + halfSize, halfSize);
    EXPECT_NEAR (rms, dcValue, 0.05f);
}

TEST_F (SincOversamplerTest, ProcessOversampledBlockCallbackReceivesCorrectSize)
{
    constexpr int shortBlockSize = 64;
    std::vector<float> ch0 (shortBlockSize, 0.0f);
    const float* inputPtrs[] = { ch0.data() };
    os2x.upsample (inputPtrs, 1, shortBlockSize);

    int callbackChannels = 0;
    int callbackSamples = 0;
    os2x.processOversampledBlock ([&] (auto& buf)
    {
        callbackChannels = buf.getNumChannels();
        callbackSamples = buf.getNumSamples();
    });

    EXPECT_EQ (callbackChannels, 1);
    EXPECT_EQ (callbackSamples, shortBlockSize * 2);
}

TEST_F (SincOversamplerTest, ProcessOversampledBlockReceivesEmptyBufferWithoutPendingBlock)
{
    int callbackChannels = -1;
    int callbackSamples = -1;

    os2x.processOversampledBlock ([&] (auto& buf)
    {
        callbackChannels = buf.getNumChannels();
        callbackSamples = buf.getNumSamples();
    });

    EXPECT_EQ (callbackChannels, 0);
    EXPECT_EQ (callbackSamples, 0);
}

TEST_F (SincOversamplerTest, DownsampleConsumesPendingOversampledBlock)
{
    std::vector<float> ch0 (blockSize, 0.0f);
    std::vector<float> output (blockSize, 0.0f);
    const float* inputPtrs[] = { ch0.data() };
    float* outputPtrs[] = { output.data() };

    os2x.upsample (inputPtrs, 1, blockSize);
    ASSERT_EQ (os2x.getOversampledNumSamples(), blockSize * 2);

    os2x.downsample (outputPtrs, 1, blockSize);

    EXPECT_EQ (os2x.getOversampledNumSamples(), 0);
    EXPECT_EQ (os2x.getOversampledChannelData (0), nullptr);
}

TEST_F (SincOversamplerTest, UpsampleThenDownsamplePreservesDCMagnitude)
{
    constexpr float dcValue = 0.5f;
    std::vector<float> input (blockSize, dcValue);
    std::vector<float> output (blockSize, 0.0f);
    const float* inputPtrs[] = { input.data() };
    float* outputPtrs[] = { output.data() };

    for (int b = 0; b < 10; ++b)
    {
        os2x.upsample (inputPtrs, 1, blockSize);
        os2x.downsample (outputPtrs, 1, blockSize);
    }

    EXPECT_NEAR (calculateRMS (output.data(), blockSize), dcValue, 0.02f);
}

TEST_F (SincOversamplerTest, UpsampleThenDownsamplePreservesLowFrequencySine)
{
    constexpr float frequency = 440.0f; // A4 - well below Nyquist/4
    std::vector<float> input (blockSize);
    fillSine (input, frequency);

    std::vector<float> output (blockSize, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    // Warm up to flush latency
    for (int b = 0; b < 10; ++b)
    {
        os2x.upsample (inPtrs, 1, blockSize);
        os2x.downsample (outPtrs, 1, blockSize);
    }

    // Compare RMS energy - should be preserved
    float rmsIn = calculateRMS (input.data(), blockSize);
    float rmsOut = calculateRMS (output.data(), blockSize);
    EXPECT_NEAR (rmsOut, rmsIn, rmsIn * 0.1f); // within 10%
}

TEST_F (SincOversamplerTest, DecimationFiltersOversampledDomainHighFrequency)
{
    // Upsample silence to get a clean oversampled buffer
    std::vector<float> silence (blockSize, 0.0f);
    const float* silPtrs[] = { silence.data() };

    for (int b = 0; b < 5; ++b)
        os2x.upsample (silPtrs, 1, blockSize);

    // Inject a tone ABOVE original Nyquist directly into the oversampled buffer.
    // This simulates distortion harmonics created at the elevated sample rate.
    // The decimation filter should attenuate this before downsampling.
    const double oversampledRate = sampleRate * 2.0;
    const double highFreq = sampleRate * 0.6; // above original Nyquist (22050 Hz)
    float* oversampledData = os2x.getOversampledChannelData (0);
    const int oversampledLen = os2x.getOversampledNumSamples();
    constexpr float injectedAmplitude = 0.5f;

    for (int i = 0; i < oversampledLen; ++i)
        oversampledData[i] += injectedAmplitude * static_cast<float> (std::sin (MathConstants<double>::twoPi * highFreq * static_cast<double> (i) / oversampledRate));

    std::vector<float> output (blockSize, 0.0f);
    float* outPtrs[] = { output.data() };
    os2x.downsample (outPtrs, 1, blockSize);

    float rmsOut = calculateRMS (output.data(), blockSize);

    // The anti-aliasing filter should substantially reduce the injected tone
    EXPECT_LT (rmsOut, injectedAmplitude * 0.5f);
}

TEST_F (SincOversamplerTest, OversampledChannelDataNotNullAfterUpsample)
{
    std::vector<float> ch0 (blockSize, 0.0f);
    const float* inputPtrs[] = { ch0.data() };
    os2x.upsample (inputPtrs, 1, blockSize);

    EXPECT_NE (os2x.getOversampledChannelData (0), nullptr);
    EXPECT_EQ (os2x.getOversampledChannelData (1), nullptr);  // channel 1 not prepared
    EXPECT_EQ (os2x.getOversampledChannelData (-1), nullptr); // invalid index
}

TEST_F (SincOversamplerTest, FourXOversamplerHasCorrectOutputSize)
{
    std::vector<float> ch0 (blockSize, 0.0f);
    const float* inputPtrs[] = { ch0.data() };
    os4x.upsample (inputPtrs, 1, blockSize);

    EXPECT_EQ (os4x.getOversampledNumSamples(), blockSize * 4);
}

} // namespace yup::test

namespace yup::test
{

TEST_F (SincOversamplerTest, DirectGenerationPreservesDCWithoutInputInterpolation)
{
    ASSERT_TRUE (os4x.beginGeneration (1, blockSize));
    FloatVectorOperations::fill (os4x.getOversampledChannelData (0), 0.25f, blockSize * 4);
    std::vector<float> output (blockSize);
    float* channels[] = { output.data() };
    os4x.downsample (channels, 1, blockSize);

    EXPECT_EQ (8, os4x.getGenerationLatencyInSamples());
    EXPECT_EQ (0, os4x.getOversampledNumSamples());
    for (int i = 32; i < blockSize; ++i)
        EXPECT_NEAR (0.25f, output[static_cast<std::size_t> (i)], 1e-6f);
}

TEST_F (SincOversamplerTest, InvalidGenerationRequestsPreserveThePendingBlock)
{
    ASSERT_TRUE (os4x.beginGeneration (1, 16));
    EXPECT_FALSE (os4x.beginGeneration (0, 16));
    EXPECT_FALSE (os4x.beginGeneration (1, 0));
    EXPECT_FALSE (os4x.beginGeneration (maxChannels + 1, 16));
    EXPECT_FALSE (os4x.beginGeneration (1, blockSize + 1));
    EXPECT_EQ (64, os4x.getOversampledNumSamples());
}

TEST_F (SincOversamplerTest, DirectGenerationImpulseHasTheReportedLatency)
{
    ASSERT_TRUE (os4x.beginGeneration (1, blockSize));
    auto* internal = os4x.getOversampledChannelData (0);
    FloatVectorOperations::clear (internal, blockSize * 4);
    internal[0] = 1.0f;
    std::vector<float> output (blockSize);
    float* channels[] = { output.data() };
    os4x.downsample (channels, 1, blockSize);
    const auto peak = std::max_element (output.begin(), output.end());
    EXPECT_EQ (os4x.getGenerationLatencyInSamples(), static_cast<int> (peak - output.begin()));
}

} // namespace yup::test

namespace yup::test
{

//==============================================================================
class SincOversamplerAccuracyTest : public ::testing::Test
{
protected:
    static constexpr double sampleRate = 48000.0;
    static constexpr double kaiserBeta = 9.0;

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

    template <typename SampleType, int Factor, int Radius>
    static void checkBlockSizeIndependence (double tolerance)
    {
        constexpr int total = 512;
        const auto input = makeNoise<SampleType> (total, 7);

        SincOversampler<SampleType, Factor, Radius> wholeBlock, fixedBlocks, irregularBlocks;
        wholeBlock.prepare (sampleRate, 1, total);
        fixedBlocks.prepare (sampleRate, 1, total);
        irregularBlocks.prepare (sampleRate, 1, total);

        const auto reference = process (wholeBlock, input, { total });
        const auto fixedResult = process (fixedBlocks, input, { 256 });
        const auto irregularResult = process (irregularBlocks, input, { 1, 3, 7, 5, 2, 13, 64, 17, 31, 9, 128 });

        expectNear (fixedResult.upsampled, reference.upsampled, tolerance);
        expectNear (fixedResult.roundTrip, reference.roundTrip, tolerance);
        expectNear (irregularResult.upsampled, reference.upsampled, tolerance);
        expectNear (irregularResult.roundTrip, reference.roundTrip, tolerance);
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

    template <int Factor, int Radius>
    static double upsampledWorstImageDb (int fundamentalBin)
    {
        constexpr int blockSize = 1024;
        const double frequency = fundamentalBin / static_cast<double> (blockSize);

        SincOversampler<float, Factor, Radius> os;
        os.prepare (sampleRate, 1, blockSize);

        std::vector<double> steadyState;

        for (int block = 0; block < 2; ++block)
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

    struct RoundTripAccuracy
    {
        double maxError = 0.0;
        double snrDb = 0.0;
    };

    /** Compares the round trip of a unit sine against the input delayed by the reported latency. */
    template <int Factor, int Radius>
    static RoundTripAccuracy roundTripAccuracy (double normalizedFrequency)
    {
        constexpr int blockSize = 512;

        SincOversampler<float, Factor, Radius> os;
        os.prepare (sampleRate, 1, blockSize);

        RoundTripAccuracy accuracy;
        double signalEnergy = 0.0;
        double errorEnergy = 0.0;

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
            }
        }

        accuracy.snrDb = 10.0 * std::log10 (signalEnergy / jmax (errorEnergy, 1e-30));
        return accuracy;
    }
};

//==============================================================================
TEST_F (SincOversamplerAccuracyTest, BlockSizeIndependenceFloat2x)
{
    checkBlockSizeIndependence<float, 2, 8> (1e-6);
}

TEST_F (SincOversamplerAccuracyTest, BlockSizeIndependenceFloat4x)
{
    checkBlockSizeIndependence<float, 4, 8> (1e-6);
}

TEST_F (SincOversamplerAccuracyTest, BlockSizeIndependenceDouble4x)
{
    checkBlockSizeIndependence<double, 4, 8> (1e-12);
}

TEST_F (SincOversamplerAccuracyTest, ImpulseLatencyWithTinyBlocks)
{
    constexpr int radius = 8;
    constexpr int factor = 4;
    constexpr int total = 96;
    constexpr int impulsePosition = 11;

    std::vector<float> input (total, 0.0f);
    input[impulsePosition] = 1.0f;

    SincOversampler<float, factor, radius> os;
    os.prepare (sampleRate, 1, total);
    const auto streams = process (os, input, { 3 });

    for (int i = 0; i < total; ++i)
    {
        const float expected = (i == impulsePosition + radius) ? 1.0f : 0.0f;
        EXPECT_EQ (expected, streams.upsampled[static_cast<std::size_t> (i * factor)]) << "input index " << i;
    }

    const auto peak = std::max_element (streams.roundTrip.begin(), streams.roundTrip.end());
    EXPECT_EQ (impulsePosition + 2 * radius, static_cast<int> (peak - streams.roundTrip.begin()));
}

TEST_F (SincOversamplerAccuracyTest, UpsampleMatchesScalarSincReference)
{
    constexpr int radius = 8;
    constexpr int factor = 4;
    constexpr int total = 256;
    const auto input = makeNoise<float> (total, 3);

    SincOversampler<float, factor, radius> os;
    os.prepare (sampleRate, 1, total);
    const auto streams = process (os, input, { 64 });

    SincTable<double, factor, radius> table;
    table.configure (sampleRate);
    table.applyKaiserWindow (kaiserBeta);

    const auto sampleAt = [&] (int index)
    {
        return (index >= 0 && index < total) ? static_cast<double> (input[static_cast<std::size_t> (index)]) : 0.0;
    };

    for (int i = 0; i < total; ++i)
    {
        const int center = i - radius;
        EXPECT_NEAR (sampleAt (center), streams.upsampled[static_cast<std::size_t> (i * factor)], 1e-6);

        for (int delta = 1; delta < factor; ++delta)
        {
            double acc = 0.0;
            double sum = 0.0;

            for (int n = -radius; n <= radius; ++n)
            {
                const auto tap = table (n, delta);
                acc += tap * sampleAt (center - n);
                sum += tap;
            }

            EXPECT_NEAR (acc / sum, streams.upsampled[static_cast<std::size_t> (i * factor + delta)], 1e-6) << "sample " << i << " phase " << delta;
        }
    }
}

TEST_F (SincOversamplerAccuracyTest, ChannelsAreIndependent)
{
    constexpr int blockSize = 100;
    constexpr int total = 300;
    const auto left = makeNoise<float> (total, 1);
    const auto right = makeNoise<float> (total, 2);

    SincOversampler<float, 2, 8> stereo, monoLeft, monoRight;
    stereo.prepare (sampleRate, 2, blockSize);
    monoLeft.prepare (sampleRate, 1, blockSize);
    monoRight.prepare (sampleRate, 1, blockSize);

    Streams<float> stereoLeft, stereoRight;

    for (int position = 0; position < total; position += blockSize)
    {
        const float* inputPtrs[] = { left.data() + position, right.data() + position };
        stereo.upsample (inputPtrs, 2, blockSize);

        const auto* upLeft = stereo.getOversampledChannelData (0);
        const auto* upRight = stereo.getOversampledChannelData (1);
        stereoLeft.upsampled.insert (stereoLeft.upsampled.end(), upLeft, upLeft + stereo.getOversampledNumSamples());
        stereoRight.upsampled.insert (stereoRight.upsampled.end(), upRight, upRight + stereo.getOversampledNumSamples());

        std::vector<float> outLeft (blockSize), outRight (blockSize);
        float* outputPtrs[] = { outLeft.data(), outRight.data() };
        stereo.downsample (outputPtrs, 2, blockSize);
        stereoLeft.roundTrip.insert (stereoLeft.roundTrip.end(), outLeft.begin(), outLeft.end());
        stereoRight.roundTrip.insert (stereoRight.roundTrip.end(), outRight.begin(), outRight.end());
    }

    const auto expectedLeft = process (monoLeft, left, { blockSize });
    const auto expectedRight = process (monoRight, right, { blockSize });

    expectNear (stereoLeft.upsampled, expectedLeft.upsampled, 1e-7);
    expectNear (stereoLeft.roundTrip, expectedLeft.roundTrip, 1e-7);
    expectNear (stereoRight.upsampled, expectedRight.upsampled, 1e-7);
    expectNear (stereoRight.roundTrip, expectedRight.roundTrip, 1e-7);
}

TEST_F (SincOversamplerAccuracyTest, ResetMatchesFreshInstance)
{
    constexpr int blockSize = 128;

    SincOversampler<float, 4, 8> reused, fresh;
    reused.prepare (sampleRate, 1, blockSize);
    fresh.prepare (sampleRate, 1, blockSize);

    process (reused, makeNoise<float> (512, 5), { blockSize });
    reused.reset();

    const auto input = makeNoise<float> (256, 6);
    const auto reusedResult = process (reused, input, { blockSize });
    const auto freshResult = process (fresh, input, { blockSize });

    expectNear (reusedResult.upsampled, freshResult.upsampled, 1e-7);
    expectNear (reusedResult.roundTrip, freshResult.roundTrip, 1e-7);
}

TEST_F (SincOversamplerAccuracyTest, GenerationDoesNotDisturbUpsampleHistory)
{
    constexpr int blockSize = 128;
    const auto blockA = makeNoise<float> (blockSize, 8);
    const auto blockB = makeNoise<float> (blockSize, 9);

    SincOversampler<float, 4, 8> withGeneration, withoutGeneration;
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
TEST_F (SincOversamplerAccuracyTest, UpsampledImageIsRejected)
{
    // 0.25 fs tone: image at 0.75 fs sits deep in the interpolator's stopband.
    EXPECT_LT (upsampledWorstImageDb<4, 16> (256), -80.0);

    // 0.4 fs tone: image at 0.6 fs sits at the edge of the transition band.
    EXPECT_LT (upsampledWorstImageDb<4, 16> (410), -70.0);
}

TEST_F (SincOversamplerAccuracyTest, DecimationRejectsOversampledDomainToneWithRadius16)
{
    constexpr int factor = 2;
    constexpr int blockSize = 2048;
    constexpr double toneRatio = 0.6; // of the input sample rate, above the input Nyquist

    SincOversampler<float, factor, 16> os;
    os.prepare (sampleRate, 1, blockSize);

    ASSERT_TRUE (os.beginGeneration (1, blockSize));
    auto* internal = os.getOversampledChannelData (0);

    for (int i = 0; i < os.getOversampledNumSamples(); ++i)
        internal[i] = static_cast<float> (std::sin (MathConstants<double>::twoPi * toneRatio * i / factor));

    std::vector<float> output (blockSize);
    float* outputPtrs[] = { output.data() };
    os.downsample (outputPtrs, 1, blockSize);

    constexpr int measured = blockSize / 2;
    const auto rms = FloatVectorOperations::rms (output.data() + blockSize - measured, measured);
    const auto levelDb = 20.0 * std::log10 (jmax (static_cast<double> (rms), 1e-12) * MathConstants<double>::sqrt2);
    EXPECT_LT (levelDb, -70.0);
}

TEST_F (SincOversamplerAccuracyTest, RoundTripPassbandIsFlat)
{
    EXPECT_LT (roundTripAccuracy<4, 16> (1000.0 / sampleRate).maxError, 0.005);
    EXPECT_LT (roundTripAccuracy<4, 16> (0.3).maxError, 0.005);
}

TEST_F (SincOversamplerAccuracyTest, RoundTripSineSNR)
{
    EXPECT_GT (roundTripAccuracy<4, 16> (0.1).snrDb, 80.0);
}

} // namespace yup::test
