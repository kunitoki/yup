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

//==============================================================================
class WavetableOscillatorTests : public ::testing::Test
{
protected:
    static double peakMagnitude (WavetableOscillator<double>& oscillator, int numSamples)
    {
        double peak = 0.0;

        for (int i = 0; i < numSamples; ++i)
            peak = jmax (peak, std::abs (oscillator.processSample()));

        return peak;
    }

    static constexpr double testSampleRate = 48000.0;
};

TEST_F (WavetableOscillatorTests, TableSizeIsOversizedButBounded)
{
    WavetableOscillator<double> oscillator;

    oscillator.prepare (testSampleRate, 128);
    EXPECT_EQ (1024, oscillator.getTableSize());

    oscillator.prepare (testSampleRate, 600);
    EXPECT_EQ (8192, oscillator.getTableSize());

    oscillator.prepare (testSampleRate, 4096);
    EXPECT_EQ (32768, oscillator.getTableSize());

    oscillator.prepare (testSampleRate, 8);
    EXPECT_EQ (64, oscillator.getTableSize());

    oscillator.prepare (testSampleRate, 1);
    EXPECT_EQ (64, oscillator.getTableSize());
}

TEST_F (WavetableOscillatorTests, MatchesTheAdditiveOscillator)
{
    constexpr int numHarmonics = 32;
    constexpr int crossfadeLength = 64;

    const auto series = FourierSeries<double>::create (Waveform::sawtooth, numHarmonics);

    WavetableOscillator<double> wavetable;
    wavetable.prepare (testSampleRate, numHarmonics, crossfadeLength);
    wavetable.setSeries (series);
    wavetable.setFrequency (1000.0);
    wavetable.render();

    for (int i = 0; i < crossfadeLength; ++i)
        wavetable.processSample();

    wavetable.reset();

    AdditiveOscillator<double> additive;
    additive.prepare (testSampleRate, numHarmonics);
    additive.setSeries (series);
    additive.setFrequency (1000.0);

    for (int i = 0; i < 512; ++i)
        EXPECT_NEAR (additive.processSample(), wavetable.processSample(), 1e-3) << i;
}

TEST_F (WavetableOscillatorTests, NeedsRenderTracksTheHarmonicLimit)
{
    WavetableOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, 64);
    oscillator.setWaveform (Waveform::sawtooth);
    oscillator.setFrequency (1000.0);

    EXPECT_TRUE (oscillator.needsRender());

    oscillator.render();
    EXPECT_FALSE (oscillator.needsRender());
    EXPECT_EQ (23, oscillator.getNumRenderedHarmonics());

    // Raising the pitch far enough that the rendered harmonics would alias
    // forces an immediate render.
    oscillator.setFrequency (20000.0);
    EXPECT_TRUE (oscillator.needsRender());

    oscillator.render();
    EXPECT_EQ (1, oscillator.getNumRenderedHarmonics());

    oscillator.setFrequency (15000.0);
    EXPECT_FALSE (oscillator.needsRender());

    // Dropping the pitch regains brightness lazily, so a small change is ignored.
    oscillator.setFrequency (2000.0);
    EXPECT_TRUE (oscillator.needsRender());
}

TEST_F (WavetableOscillatorTests, CrossfadeBetweenRendersIsContinuous)
{
    WavetableOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, 64, 64);
    oscillator.setSeries (FourierSeries<double>::create (Waveform::sawtooth, 16));
    oscillator.setFrequency (220.0);
    oscillator.render();

    for (int i = 0; i < 128; ++i)
        oscillator.processSample();

    oscillator.setSeries (FourierSeries<double>::create (Waveform::square, 16));
    oscillator.render();

    auto previous = oscillator.processSample();
    auto maxJump = 0.0;

    for (int i = 1; i < 4096; ++i)
    {
        const auto value = oscillator.processSample();

        maxJump = jmax (maxJump, std::abs (value - previous));
        previous = value;
    }

    // Swapping the table without a crossfade would jump by up to the full swing of
    // the two waveforms.
    EXPECT_LT (maxJump, 0.5);
}

TEST_F (WavetableOscillatorTests, KeepsPlayingUntilRenderedAgain)
{
    WavetableOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, 16);
    oscillator.setWaveform (Waveform::sine);
    oscillator.setFrequency (1000.0);
    oscillator.render();

    EXPECT_NEAR (1.0, peakMagnitude (oscillator, 480), 0.01);

    // Changing the series marks the table stale, but the previous rendition has to
    // keep playing until the next render.
    oscillator.setWaveform (Waveform::square);

    EXPECT_TRUE (oscillator.needsRender());
    EXPECT_NEAR (1.0, peakMagnitude (oscillator, 480), 0.01);
}

TEST_F (WavetableOscillatorTests, BlockMatchesSampleBySample)
{
    WavetableOscillator<double> block;
    WavetableOscillator<double> sample;

    block.prepare (testSampleRate, 32);
    sample.prepare (testSampleRate, 32);

    block.setSeries (FourierSeries<double>::create (Waveform::triangle, 32));
    sample.setSeries (FourierSeries<double>::create (Waveform::triangle, 32));

    block.setFrequency (660.0);
    sample.setFrequency (660.0);

    block.render();
    sample.render();

    std::vector<double> buffer (256);
    block.processBlock (buffer.data(), 256);

    for (int i = 0; i < 256; ++i)
        EXPECT_EQ (buffer[static_cast<std::size_t> (i)], sample.processSample()) << i;
}

TEST_F (WavetableOscillatorTests, FloatInstantiationRuns)
{
    WavetableOscillator<float> oscillator;
    oscillator.prepare (testSampleRate, 64);
    oscillator.setWaveform (Waveform::sine);
    oscillator.setFrequency (440.0);
    oscillator.render();

    float peak = 0.0f;

    for (int i = 0; i < 1024; ++i)
        peak = jmax (peak, std::abs (oscillator.processSample()));

    EXPECT_NEAR (1.0f, peak, 0.02f);
}

TEST_F (WavetableOscillatorTests, SilenceWithoutRenderedHarmonics)
{
    WavetableOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, 16);
    oscillator.setWaveform (Waveform::sine);

    // Above twice Nyquist every harmonic is out of range, so the table is silent.
    oscillator.setFrequency (30000.0);
    oscillator.render();

    EXPECT_EQ (0, oscillator.getNumRenderedHarmonics());
    EXPECT_EQ (0.0, peakMagnitude (oscillator, 64));
}
