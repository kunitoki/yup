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
class SyncOscillatorTests : public ::testing::Test
{
protected:
    /**
        Level, in dB below the strongest bin, of every spectral bin that is not
        within guardBins of a multiple of harmonicBin.

        A signal whose only components sit on the harmonic grid of harmonicBin
        measures far below 0 dB, while a naive oscillator whose aliases fold onto
        other bins measures tens of dB higher.
    */
    static double offHarmonicEnergyDb (const std::vector<double>& signal, int harmonicBin, int guardBins)
    {
        const auto size = static_cast<int> (signal.size());

        std::vector<double> window (static_cast<std::size_t> (size));
        WindowFunctions<double>::generate (WindowType::blackmanHarris, window);

        std::vector<float> input (static_cast<std::size_t> (size));

        for (int i = 0; i < size; ++i)
            input[static_cast<std::size_t> (i)] = static_cast<float> (signal[static_cast<std::size_t> (i)] * window[static_cast<std::size_t> (i)]);

        FFTProcessor<float> fft (size);

        std::vector<float> spectrum (static_cast<std::size_t> (size) * 2);
        fft.performRealFFTForward (input.data(), spectrum.data());

        std::vector<double> magnitude (static_cast<std::size_t> (size) / 2 + 1, 0.0);

        for (int k = 0; k <= size / 2; ++k)
            magnitude[static_cast<std::size_t> (k)] = std::hypot (static_cast<double> (spectrum[static_cast<std::size_t> (2 * k)]),
                                                                  static_cast<double> (spectrum[static_cast<std::size_t> (2 * k + 1)]));

        const auto peak = *std::max_element (magnitude.begin(), magnitude.end());

        double worst = 0.0;

        for (int k = 1; k <= size / 2; ++k)
        {
            const auto remainder = k % harmonicBin;

            if (jmin (remainder, harmonicBin - remainder) <= guardBins)
                continue;

            worst = jmax (worst, magnitude[static_cast<std::size_t> (k)]);
        }

        worst = jmax (worst, 1e-12 * peak);

        return 20.0 * std::log10 (worst / peak);
    }

    static void configureSyncedSawtooth (SyncOscillator<double>& oscillator, SyncMode mode, double ratio)
    {
        oscillator.prepare (testSampleRate, testMaxHarmonics);
        oscillator.setWaveform (Waveform::sawtooth);
        oscillator.setSyncMode (mode);
        oscillator.setFollowerRatio (ratio);
        oscillator.setFrequency (440.0);
        oscillator.update();
    }

    static constexpr double testSampleRate = 48000.0;
    static constexpr int testMaxHarmonics = 256;
};

TEST_F (SyncOscillatorTests, AdditiveBackendMatchesManualPipeline)
{
    SyncOscillator<double> oscillator;
    configureSyncedSawtooth (oscillator, SyncMode::hard, 1.375);
    oscillator.setSynthesis (SyncOscillator<double>::Synthesis::additive);

    FourierSeries<double> follower (testMaxHarmonics);
    follower.setWaveform (Waveform::sawtooth);

    const auto numOutputHarmonics = jmin (testMaxHarmonics,
                                          SyncSpectralResampler<double>::getRecommendedOutputHarmonics (follower.getNumHarmonics(),
                                                                                                       1.375,
                                                                                                       SyncMode::hard));

    SyncSpectralResampler<double> resampler;
    resampler.prepare (testMaxHarmonics);

    FourierSeries<double> synced (testMaxHarmonics);
    resampler.transform (follower, 1.375, SyncMode::hard, synced, numOutputHarmonics);

    AdditiveOscillator<double> reference;
    reference.prepare (testSampleRate, testMaxHarmonics);
    reference.setSeries (synced);
    reference.setFrequency (440.0);

    for (int i = 0; i < 512; ++i)
        EXPECT_NEAR (reference.processSample(), oscillator.processSample(), 1e-12) << i;
}

TEST_F (SyncOscillatorTests, WavetableBackendApproximatesTheAdditiveOne)
{
    SyncOscillator<double> oscillator;
    configureSyncedSawtooth (oscillator, SyncMode::hard, 1.375);

    std::vector<double> wavetableBuffer (512);
    oscillator.processBlock (wavetableBuffer.data(), 512);
    oscillator.setPhase (0.0);
    oscillator.processBlock (wavetableBuffer.data(), 512);

    oscillator.setSynthesis (SyncOscillator<double>::Synthesis::additive);
    oscillator.setPhase (0.0);

    std::vector<double> additiveBuffer (512);
    oscillator.processBlock (additiveBuffer.data(), 512);

    for (int i = 0; i < 512; ++i)
        EXPECT_NEAR (additiveBuffer[static_cast<std::size_t> (i)], wavetableBuffer[static_cast<std::size_t> (i)], 1e-3) << i;
}

TEST_F (SyncOscillatorTests, MirroredRunsAtHalfTheLeaderFrequency)
{
    SyncOscillator<double> oscillator;
    configureSyncedSawtooth (oscillator, SyncMode::mirrored, 1.375);

    EXPECT_NEAR (220.0, oscillator.getOutputFrequency(), 1e-12);
    EXPECT_EQ (440.0, oscillator.getFrequency());

    SyncOscillator<double> reference;
    reference.prepare (testSampleRate, testMaxHarmonics);
    reference.setWaveform (Waveform::sawtooth);
    reference.setSyncMode (SyncMode::mirrored);
    reference.setFollowerRatio (1.375);
    reference.setFrequency (440.0);
    reference.setSynthesis (SyncOscillator<double>::Synthesis::additive);
    reference.update();

    // The mirrored output is periodic with twice the leader period, so it equals an
    // additive oscillator running the synced series at half the leader pitch.
    const auto numOutputHarmonics = jmin (testMaxHarmonics,
                                          SyncSpectralResampler<double>::getRecommendedOutputHarmonics (testMaxHarmonics,
                                                                                                       1.375,
                                                                                                       SyncMode::mirrored));

    FourierSeries<double> follower (testMaxHarmonics);
    follower.setWaveform (Waveform::sawtooth);

    SyncSpectralResampler<double> resampler;
    resampler.prepare (testMaxHarmonics);

    FourierSeries<double> synced (testMaxHarmonics);
    resampler.transform (follower, 1.375, SyncMode::mirrored, synced, numOutputHarmonics);

    AdditiveOscillator<double> additive;
    additive.prepare (testSampleRate, testMaxHarmonics);
    additive.setSeries (synced);
    additive.setFrequency (220.0);

    for (int i = 0; i < 512; ++i)
        EXPECT_NEAR (additive.processSample(), reference.processSample(), 1e-12) << i;
}

TEST_F (SyncOscillatorTests, UpdateWithNothingDirtyLeavesTheOutputUnchanged)
{
    SyncOscillator<double> updatedOnce;
    SyncOscillator<double> updatedTwice;

    configureSyncedSawtooth (updatedOnce, SyncMode::hard, 1.375);
    configureSyncedSawtooth (updatedTwice, SyncMode::hard, 1.375);

    updatedTwice.update();

    EXPECT_FALSE (updatedTwice.needsUpdate());

    for (int i = 0; i < 256; ++i)
        EXPECT_EQ (updatedOnce.processSample(), updatedTwice.processSample()) << i;
}

TEST_F (SyncOscillatorTests, SwitchingSynthesisKeepsThePhase)
{
    SyncOscillator<double> oscillator;
    configureSyncedSawtooth (oscillator, SyncMode::hard, 1.375);

    for (int i = 0; i < 100; ++i)
        oscillator.processSample();

    const auto phase = oscillator.getPhase();

    oscillator.setSynthesis (SyncOscillator<double>::Synthesis::additive);
    EXPECT_NEAR (phase, oscillator.getPhase(), 1e-12);

    oscillator.processSample();

    const auto additivePhase = oscillator.getPhase();

    oscillator.setSynthesis (SyncOscillator<double>::Synthesis::wavetable);
    EXPECT_NEAR (additivePhase, oscillator.getPhase(), 1e-12);
}

TEST_F (SyncOscillatorTests, FollowerFrequencySetsTheRatio)
{
    SyncOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, 64);
    oscillator.setFrequency (440.0);
    oscillator.setFollowerFrequency (605.0);

    EXPECT_NEAR (1.375, oscillator.getFollowerRatio(), 1e-12);

    // The ratio is a factor, so it survives a pitch change.
    oscillator.setFrequency (880.0);
    EXPECT_NEAR (1.375, oscillator.getFollowerRatio(), 1e-12);
    EXPECT_NEAR (880.0, oscillator.getOutputFrequency(), 1e-12);
}

TEST_F (SyncOscillatorTests, NoneModePlaysTheFollowerAtTheLeaderPitch)
{
    SyncOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, 64);
    oscillator.setWaveform (Waveform::square);
    oscillator.setFollowerRatio (1.375);
    oscillator.setFrequency (440.0);
    oscillator.update();

    EXPECT_EQ (SyncMode::none, oscillator.getSyncMode());
    EXPECT_NEAR (440.0, oscillator.getOutputFrequency(), 1e-12);
    EXPECT_EQ (64, oscillator.getSyncedSeries().getNumHarmonics());

    for (int n = 1; n <= 64; ++n)
        EXPECT_EQ (oscillator.getFollowerSeries().getSine (n), oscillator.getSyncedSeries().getSine (n)) << n;
}

//==============================================================================
TEST_F (SyncOscillatorTests, HardSyncedSawtoothIsAliasFree)
{
    constexpr int fftSize = 4096;
    constexpr int harmonicBin = 63;
    constexpr int guardBins = 4; // Exclude the Blackman-Harris main lobe.

    // Bin exact leader pitch, with 63 coprime with the FFT size so that folded
    // aliases of a naive oscillator cannot land on a harmonic by accident.
    const auto leaderFrequency = testSampleRate * harmonicBin / fftSize;

    SyncOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, testMaxHarmonics);
    oscillator.setWaveform (Waveform::sawtooth);
    oscillator.setSyncMode (SyncMode::hard);
    oscillator.setFollowerRatio (1.375);
    oscillator.setFrequency (leaderFrequency);
    oscillator.update();

    std::vector<double> buffer (fftSize);

    oscillator.processBlock (buffer.data(), fftSize);
    oscillator.setPhase (0.0);
    oscillator.processBlock (buffer.data(), fftSize);
    const auto wavetableEnergy = offHarmonicEnergyDb (buffer, harmonicBin, guardBins);

    oscillator.setSynthesis (SyncOscillator<double>::Synthesis::additive);
    oscillator.setPhase (0.0);
    oscillator.processBlock (buffer.data(), fftSize);
    const auto additiveEnergy = offHarmonicEnergyDb (buffer, harmonicBin, guardBins);

    // A naive phase-reset sawtooth at the follower pitch is not alias free, and the
    // same measurement catches it.
    std::vector<double> naive (fftSize);
    auto phase = 0.0;

    for (int i = 0; i < fftSize; ++i)
    {
        naive[static_cast<std::size_t> (i)] = 2.0 * phase - 1.0;

        phase += leaderFrequency * 1.375 / testSampleRate;
        phase -= std::floor (phase);
    }

    const auto naiveEnergy = offHarmonicEnergyDb (naive, harmonicBin, guardBins);

    EXPECT_LT (additiveEnergy, -80.0) << "additive backend";
    EXPECT_LT (wavetableEnergy, -60.0) << "wavetable backend";
    EXPECT_GT (naiveEnergy, -40.0) << "naive reference";
}
