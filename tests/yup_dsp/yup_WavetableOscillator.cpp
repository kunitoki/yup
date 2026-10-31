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

TEST_F (WavetableOscillatorTests, DirectPhaseReadsAndDerivativesDoNotAdvancePlayback)
{
    WavetableOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, 16);
    oscillator.setWaveform (Waveform::sine);
    oscillator.render (false);

    EXPECT_NEAR (1.0, oscillator.getValueAtPhase (-0.75), 1e-6);
    EXPECT_NEAR (MathConstants<double>::twoPi, oscillator.getSlopeAtPhase (0.0), 0.005);
    EXPECT_EQ (0.0, oscillator.getPhase());
}

TEST_F (WavetableOscillatorTests, RenderedTableMatchesSeriesAfterImmediateRender)
{
    WavetableOscillator<double> oscillator;
    EXPECT_TRUE (oscillator.getRenderedTable().empty());

    FourierSeries<double> series (16);
    series.setHarmonic (1, 1.0, 0.0);

    oscillator.prepare (testSampleRate, 16);
    oscillator.setSeries (series);
    oscillator.render (false);

    const auto table = oscillator.getRenderedTable();

    ASSERT_EQ (static_cast<std::size_t> (oscillator.getTableSize()), table.size());
    EXPECT_NEAR (1.0, table[0], 1e-6);
}

TEST_F (WavetableOscillatorTests, RenderFromPlaysTheSourceTable)
{
    WavetableOscillator<double> source;
    WavetableOscillator<double> follower;

    source.prepare (testSampleRate, 32);
    follower.prepare (testSampleRate, 32);

    source.setSeries (FourierSeries<double>::create (Waveform::sawtooth, 32));
    source.setFrequency (440.0);
    follower.setFrequency (440.0);

    source.render (false);
    follower.renderFrom (source, false);

    EXPECT_EQ (source.getNumRenderedHarmonics(), follower.getNumRenderedHarmonics());

    for (int i = 0; i < 256; ++i)
        EXPECT_EQ (source.processSample(), follower.processSample()) << i;
}

TEST_F (WavetableOscillatorTests, RenderFromCrossfadesLikeRender)
{
    constexpr int crossfadeLength = 64;

    WavetableOscillator<double> source;
    WavetableOscillator<double> previous;
    WavetableOscillator<double> follower;

    source.prepare (testSampleRate, 32, crossfadeLength);
    previous.prepare (testSampleRate, 32, crossfadeLength);
    follower.prepare (testSampleRate, 32, crossfadeLength);

    source.setSeries (FourierSeries<double>::create (Waveform::square, 32));
    previous.setSeries (FourierSeries<double>::create (Waveform::sawtooth, 32));
    follower.setSeries (FourierSeries<double>::create (Waveform::sawtooth, 32));

    for (auto* oscillator : { &source, &previous, &follower })
    {
        oscillator->setFrequency (440.0);
        oscillator->setPhase (0.25);
        oscillator->render (false);
    }

    follower.renderFrom (source);

    // The crossfade starts from the table that was playing.
    EXPECT_EQ (previous.processSample(), follower.processSample());
    source.processSample();

    for (int i = 1; i < crossfadeLength; ++i)
    {
        follower.processSample();
        source.processSample();
    }

    for (int i = 0; i < 256; ++i)
        EXPECT_EQ (source.processSample(), follower.processSample()) << i;
}

TEST_F (WavetableOscillatorTests, RenderFromTakesTheLatestRendition)
{
    WavetableOscillator<double> source;
    WavetableOscillator<double> reference;
    WavetableOscillator<double> follower;

    source.prepare (testSampleRate, 32);
    reference.prepare (testSampleRate, 32);
    follower.prepare (testSampleRate, 32);

    source.setSeries (FourierSeries<double>::create (Waveform::sawtooth, 32));
    source.render (false);

    // The source is left mid-crossfade towards the square.
    source.setSeries (FourierSeries<double>::create (Waveform::square, 32));
    source.render();
    source.processSample();

    reference.setSeries (FourierSeries<double>::create (Waveform::square, 32));
    reference.render (false);

    follower.renderFrom (source, false);

    for (int i = 0; i < 256; ++i)
        EXPECT_EQ (reference.processSample(), follower.processSample()) << i;
}

TEST_F (WavetableOscillatorTests, RenderFromIgnoresMismatchedTableSize)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "Copying a table of a different size triggers an assertion when assertions are enabled or logged";
#endif

    WavetableOscillator<double> source;
    WavetableOscillator<double> reference;
    WavetableOscillator<double> follower;

    source.prepare (testSampleRate, 128);
    reference.prepare (testSampleRate, 16);
    follower.prepare (testSampleRate, 16);

    source.setWaveform (Waveform::sawtooth);
    source.render (false);

    reference.setWaveform (Waveform::sine);
    follower.setWaveform (Waveform::sine);
    reference.render (false);
    follower.render (false);

    follower.renderFrom (source, false);

    for (int i = 0; i < 256; ++i)
        EXPECT_EQ (reference.processSample(), follower.processSample()) << i;
}

TEST_F (WavetableOscillatorTests, ProcessSampleAtPhaseMatchesProcessSample)
{
    constexpr double frequency = 331.7;

    WavetableOscillator<double> reference;
    WavetableOscillator<double> follower;

    for (auto* oscillator : { &reference, &follower })
    {
        oscillator->prepare (testSampleRate, 64, 64);
        oscillator->setFrequency (frequency);
        oscillator->setWaveform (Waveform::sawtooth);
        oscillator->render (false);
        oscillator->setWaveform (Waveform::square);
        oscillator->render();
    }

    double phase = 0.0;

    for (int i = 0; i < 512; ++i)
    {
        EXPECT_EQ (reference.processSample(), follower.processSampleAtPhase (phase)) << i;

        phase += frequency / testSampleRate;
        phase -= std::floor (phase);
    }
}

TEST_F (WavetableOscillatorTests, ProcessSampleAtPhaseCompletesTheCrossfade)
{
    constexpr int crossfadeLength = 64;

    WavetableOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, 16, crossfadeLength);
    oscillator.setWaveform (Waveform::sine);
    oscillator.render (false);
    oscillator.setWaveform (Waveform::sawtooth);
    oscillator.render();

    for (int i = 0; i < crossfadeLength - 1; ++i)
        oscillator.processSampleAtPhase (0.1 * i);

    // Still fading: the table being played is the previous rendition.
    EXPECT_NEAR (1.0, oscillator.getRenderedTable()[static_cast<std::size_t> (oscillator.getTableSize() / 4)], 1e-6);

    oscillator.processSampleAtPhase (0.0);

    EXPECT_NEAR (oscillator.getValueAtPhase (0.25), oscillator.getRenderedTable()[static_cast<std::size_t> (oscillator.getTableSize() / 4)], 1e-12);
    EXPECT_GT (std::abs (oscillator.getValueAtPhase (0.25) - 1.0), 0.01);
    EXPECT_EQ (0.0, oscillator.getPhase());
}

TEST_F (WavetableOscillatorTests, AdvanceCrossfadeSkipsSamples)
{
    constexpr int crossfadeLength = 64;

    WavetableOscillator<double> skipped;
    WavetableOscillator<double> played;

    for (auto* oscillator : { &skipped, &played })
    {
        oscillator->prepare (testSampleRate, 16, crossfadeLength);
        oscillator->setWaveform (Waveform::sine);
        oscillator->render (false);
        oscillator->setWaveform (Waveform::square);
        oscillator->render();
    }

    skipped.advanceCrossfade (crossfadeLength / 2);

    for (int i = 0; i < crossfadeLength / 2; ++i)
        played.processSampleAtPhase (0.0);

    EXPECT_NEAR (played.getValueAtPhase (0.3), skipped.getValueAtPhase (0.3), 1e-12);

    skipped.advanceCrossfade (crossfadeLength);

    for (int i = 0; i < crossfadeLength / 2; ++i)
        played.processSampleAtPhase (0.0);

    for (const auto phase : { 0.1, 0.3, 0.7 })
        EXPECT_NEAR (played.getValueAtPhase (phase), skipped.getValueAtPhase (phase), 1e-12);

    EXPECT_EQ (0.0, skipped.getPhase());
}

TEST_F (WavetableOscillatorTests, PeakSlopeIsMeasuredAtRenderTime)
{
    WavetableOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, 64, 64);
    oscillator.setFrequency (100.0);

    EXPECT_EQ (0.0, oscillator.getPeakSlope());

    oscillator.setWaveform (Waveform::sine);
    oscillator.render (false);

    EXPECT_NEAR (MathConstants<double>::twoPi, oscillator.getPeakSlope(), 0.01 * MathConstants<double>::twoPi);

    WavetableOscillator<double> follower;
    follower.prepare (testSampleRate, 64, 64);
    follower.renderFrom (oscillator, false);

    EXPECT_EQ (oscillator.getPeakSlope(), follower.getPeakSlope());
}

TEST_F (WavetableOscillatorTests, PeakSlopeCoversBothTablesWhileCrossfading)
{
    constexpr int crossfadeLength = 64;

    WavetableOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, 64, crossfadeLength);
    oscillator.setFrequency (100.0);
    oscillator.setWaveform (Waveform::sawtooth);
    oscillator.render (false);

    const auto sawSlope = oscillator.getPeakSlope();
    EXPECT_GT (sawSlope, 10.0 * MathConstants<double>::twoPi);

    oscillator.setWaveform (Waveform::sine);
    oscillator.render();

    EXPECT_EQ (sawSlope, oscillator.getPeakSlope());

    oscillator.advanceCrossfade (crossfadeLength);

    EXPECT_NEAR (MathConstants<double>::twoPi, oscillator.getPeakSlope(), 0.01 * MathConstants<double>::twoPi);
}
