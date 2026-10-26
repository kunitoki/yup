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

#include <chrono>
#include <cstdio>
#include <vector>

using namespace yup;

//==============================================================================
class PhaseModulationMatrixTests : public ::testing::Test
{
protected:
    using Matrix = PhaseModulationMatrix<double>;
    using Table = WavetableOscillator<double>;

    static constexpr double sampleRate = 48000.0;
    static constexpr int factor = 4;
    static constexpr int blockSize = 32;
    static constexpr int analysisSize = 16384;
    static constexpr int warmup = 8192;

    std::array<Table, Matrix::numOperators> tables;
    Matrix matrix;

    void prepare (int oversampling, int maxHarmonics = 256)
    {
        matrix.prepare (sampleRate, oversampling);

        for (int i = 0; i < Matrix::numOperators; ++i)
        {
            tables[static_cast<std::size_t> (i)].prepare (sampleRate * oversampling, maxHarmonics);
            tables[static_cast<std::size_t> (i)].setWaveform (Waveform::sine);
            matrix.setTable (i, &tables[static_cast<std::size_t> (i)]);
        }
    }

    void renderTables()
    {
        for (auto& table : tables)
            if (table.needsRender())
                table.render (false);
    }

    /** Steps the matrix twice so tables rendered for the first time are accounted for. */
    void prime (const Matrix::Controls& controls)
    {
        for (int i = 0; i < 2; ++i)
        {
            matrix.setTargets (controls, 0);
            renderTables();
        }
    }

    /** Runs the matrix at the given oversampling and returns the decimated sum of all
        operator outputs.
    */
    std::vector<double> generate (const Matrix::Controls& controls, int numOutputSamples)
    {
        HalfbandOversampler<double, factor> oversampler;
        oversampler.prepare (sampleRate, 1, blockSize);

        const auto latency = oversampler.getGenerationLatencyInSamples();
        std::vector<double> output (static_cast<std::size_t> (numOutputSamples + latency));
        std::vector<std::vector<double>> operators (Matrix::numOperators, std::vector<double> (blockSize * factor));

        prime (controls);

        for (int start = 0; start < static_cast<int> (output.size()); start += blockSize)
        {
            const auto count = jmin (blockSize, static_cast<int> (output.size()) - start);

            matrix.setTargets (controls, count * factor);
            renderTables();

            double* outputs[Matrix::numOperators];

            for (int i = 0; i < Matrix::numOperators; ++i)
                outputs[i] = operators[static_cast<std::size_t> (i)].data();

            matrix.process (outputs, count * factor);

            oversampler.beginGeneration (1, count);
            auto* internal = oversampler.getOversampledChannelData (0);

            for (int s = 0; s < count * factor; ++s)
            {
                internal[s] = 0.0;

                for (const auto& op : operators)
                    internal[s] += op[static_cast<std::size_t> (s)];
            }

            double* channels[] = { output.data() + start };
            oversampler.downsample (channels, 1, count);
        }

        output.erase (output.begin(), output.begin() + latency);
        return output;
    }

    /** Energy outside the harmonics of f0 below 0.45 fs, relative to all of it, in dB.
        The same metric as the Phase 0 Python prototype: Blackman-Harris window, 4 guard
        bins, the first bins and the warm-up dropped.
    */
    static double inharmonicRatioDb (const std::vector<double>& signal, double f0)
    {
        FFTProcessor<double> fft (analysisSize);
        fft.setScaling (FFTProcessor<double>::FFTScaling::none);

        std::vector<double> windowed (analysisSize);
        std::vector<double> spectrum (analysisSize * 2);

        for (int i = 0; i < analysisSize; ++i)
            windowed[static_cast<std::size_t> (i)] = signal[static_cast<std::size_t> (warmup + i)]
                                                   * WindowFunctions<double>::blackmanHarris (i, analysisSize);

        fft.performRealFFTForward (windowed.data(), spectrum.data());

        const auto binWidth = sampleRate / analysisSize;
        double total = 0.0;
        double outside = 0.0;

        for (int bin = 0; bin <= analysisSize / 2; ++bin)
        {
            const auto frequency = bin * binWidth;

            if (frequency > 0.45 * sampleRate)
                break;

            const auto re = spectrum[static_cast<std::size_t> (2 * bin)];
            const auto im = spectrum[static_cast<std::size_t> (2 * bin + 1)];
            const auto power = re * re + im * im;
            const auto harmonic = std::round (frequency / f0);

            total += power;

            if (bin > 5 && (harmonic < 1.0 || std::abs (frequency - harmonic * f0) > 4.0 * binWidth))
                outside += power;
        }

        return 10.0 * std::log10 (outside / total + 1e-30);
    }

    /** Magnitude of one frequency over exactly one second, by direct correlation. */
    static double amplitudeAt (const std::vector<double>& signal, int start, double frequency)
    {
        double re = 0.0;
        double im = 0.0;

        for (int i = 0; i < static_cast<int> (sampleRate); ++i)
        {
            const auto angle = MathConstants<double>::twoPi * frequency * i / sampleRate;
            re += signal[static_cast<std::size_t> (start + i)] * std::cos (angle);
            im += signal[static_cast<std::size_t> (start + i)] * std::sin (angle);
        }

        return 2.0 * std::sqrt (re * re + im * im) / sampleRate;
    }

    static double maxJump (const std::vector<double>& values, std::size_t from = 1)
    {
        double result = 0.0;

        for (auto i = jmax<std::size_t> (1, from); i < values.size(); ++i)
            result = jmax (result, std::abs (values[i] - values[i - 1]));

        return result;
    }
};

//==============================================================================
TEST_F (PhaseModulationMatrixTests, SidebandsFollowBesselFunctions)
{
    constexpr double carrier = 1000.0;
    constexpr double modulator = 125.0;

    struct Case
    {
        double index;
        std::vector<double> magnitudes; // |J_n (index)| for n = 0, 1, 2, ...
    };

    const Case cases[] = {
        { 0.5, { 0.938470, 0.242268, 0.030604 } },
        { 1.0, { 0.765198, 0.440051, 0.114903, 0.019563 } },
        { 2.0, { 0.223891, 0.576725, 0.352834, 0.128943, 0.033996 } },
        { 5.0, { 0.177597, 0.327579, 0.046565, 0.364831, 0.391232, 0.261141, 0.131049 } }
    };

    for (const auto& c : cases)
    {
        prepare (factor);
        matrix.reset();

        Matrix::Controls controls;
        controls.frequencies = { carrier, modulator, 0.0, 0.0 };
        controls.gains = { 1.0, 0.0, 0.0, 0.0 };
        controls.amounts[0][1] = c.index / MathConstants<double>::twoPi;

        const auto output = generate (controls, static_cast<int> (sampleRate) + 4096);

        for (std::size_t n = 0; n < c.magnitudes.size(); ++n)
        {
            if (c.magnitudes[n] < 0.03)
                continue;

            for (const auto side : { -1.0, 1.0 })
            {
                const auto frequency = carrier + side * static_cast<double> (n) * modulator;
                const auto measured = amplitudeAt (output, 4096, frequency);

                EXPECT_NEAR (20.0 * std::log10 (c.magnitudes[n]), 20.0 * std::log10 (measured), 0.5)
                    << "index " << c.index << " sideband " << side * static_cast<double> (n);
            }
        }
    }
}

TEST_F (PhaseModulationMatrixTests, ZeroMatrixMatchesPlainWavetableOscillators)
{
    prepare (1);

    std::array<Table, 3> plain;
    const double frequencies[] = { 110.0, 233.3, 1201.7 };
    const Waveform waveforms[] = { Waveform::sawtooth, Waveform::square, Waveform::triangle };

    Matrix::Controls controls;

    for (int i = 0; i < 3; ++i)
    {
        auto& reference = plain[static_cast<std::size_t> (i)];
        reference.prepare (sampleRate, 256);
        reference.setWaveform (waveforms[i]);
        reference.setFrequency (frequencies[i]);
        reference.render (false);

        tables[static_cast<std::size_t> (i)].setWaveform (waveforms[i]);
        controls.frequencies[static_cast<std::size_t> (i)] = frequencies[i];
        controls.gains[static_cast<std::size_t> (i)] = 1.0;
    }

    matrix.reset();
    prime (controls);

    std::vector<std::vector<double>> outputs (Matrix::numOperators, std::vector<double> (blockSize));
    double* pointers[Matrix::numOperators];

    for (int i = 0; i < Matrix::numOperators; ++i)
        pointers[i] = outputs[static_cast<std::size_t> (i)].data();

    for (int block = 0; block < 64; ++block)
    {
        matrix.setTargets (controls, blockSize);
        renderTables();
        matrix.process (pointers, blockSize);

        for (int s = 0; s < blockSize; ++s)
            for (int i = 0; i < 3; ++i)
                ASSERT_EQ (plain[static_cast<std::size_t> (i)].processSample(), outputs[static_cast<std::size_t> (i)][static_cast<std::size_t> (s)])
                    << "block " << block << " sample " << s << " operator " << i;
    }

    EXPECT_FALSE (matrix.isOperatorActive (3));
}

TEST_F (PhaseModulationMatrixTests, MaximumSineFeedbackStaysHarmonic)
{
    prepare (factor);
    matrix.reset();

    Matrix::Controls controls;
    controls.frequencies = { 440.3, 0.0, 0.0, 0.0 };
    controls.gains = { 1.0, 0.0, 0.0, 0.0 };
    controls.amounts[0][0] = 1.0;

    EXPECT_LT (inharmonicRatioDb (generate (controls, warmup + analysisSize), 440.3), -60.0);
}

TEST_F (PhaseModulationMatrixTests, MaximumSawFeedbackStaysHarmonic)
{
    prepare (factor);
    tables[0].setWaveform (Waveform::sawtooth);
    matrix.reset();

    Matrix::Controls controls;
    controls.frequencies = { 110.3, 0.0, 0.0, 0.0 };
    controls.gains = { 1.0, 0.0, 0.0, 0.0 };
    controls.amounts[0][0] = 1.0;

    EXPECT_LT (inharmonicRatioDb (generate (controls, warmup + analysisSize), 110.3), -60.0);
}

TEST_F (PhaseModulationMatrixTests, FeedbackChangesTheSineSpectrum)
{
    prepare (factor);
    matrix.reset();

    Matrix::Controls controls;
    controls.frequencies = { 1000.0, 0.0, 0.0, 0.0 };
    controls.gains = { 1.0, 0.0, 0.0, 0.0 };

    const auto plain = generate (controls, static_cast<int> (sampleRate) + 4096);

    matrix.reset();
    controls.amounts[0][0] = 1.0;
    const auto fed = generate (controls, static_cast<int> (sampleRate) + 4096);

    EXPECT_LT (amplitudeAt (plain, 4096, 2000.0), 1e-3);
    EXPECT_GT (amplitudeAt (fed, 4096, 2000.0), 0.1);
}

TEST_F (PhaseModulationMatrixTests, AliasingStaysBelowTheFloorAcrossAnIndexSweep)
{
    for (const auto carrier : { 220.3, 1320.7 })
    {
        for (const auto index : { 0.25, 1.0, 4.0 })
        {
            prepare (factor, 512);
            tables[0].setWaveform (Waveform::sawtooth);
            matrix.reset();

            Matrix::Controls controls;
            controls.frequencies = { carrier, carrier, 0.0, 0.0 };
            controls.gains = { 1.0, 0.0, 0.0, 0.0 };
            controls.amounts[0][1] = index;

            EXPECT_LT (inharmonicRatioDb (generate (controls, warmup + analysisSize), carrier), -60.0)
                << "carrier " << carrier << " index " << index;
        }
    }
}

TEST_F (PhaseModulationMatrixTests, BandwidthKeepsAtLeastTheFundamental)
{
    prepare (factor, 512);
    tables[0].setWaveform (Waveform::sawtooth);
    matrix.reset();

    Matrix::Controls controls;
    controls.frequencies = { 3520.9, 3520.9, 0.0, 0.0 };
    controls.gains = { 1.0, 0.0, 0.0, 0.0 };
    controls.amounts[0][1] = 4.0;

    prime (controls);

    EXPECT_GE (tables[0].getNumRenderedHarmonics(), 1);
}

TEST_F (PhaseModulationMatrixTests, ParityCapLimitsHarmonicsToTheBaseRate)
{
    prepare (factor, 512);
    tables[0].setWaveform (Waveform::sawtooth);
    matrix.reset();

    Matrix::Controls controls;
    controls.frequencies = { 220.0, 0.0, 0.0, 0.0 };
    controls.gains = { 1.0, 0.0, 0.0, 0.0 };

    prime (controls);

    EXPECT_EQ (getNyquistHarmonicLimit (220.0, sampleRate, 512), tables[0].getNumRenderedHarmonics());
}

TEST_F (PhaseModulationMatrixTests, RampedAmountsDoNotStep)
{
    prepare (factor);

    Matrix::Controls controls;
    controls.frequencies = { 440.0, 440.0, 0.0, 0.0 };
    controls.gains = { 1.0, 0.0, 0.0, 0.0 };

    Matrix snapped;
    snapped.prepare (sampleRate, factor);

    std::array<Table, Matrix::numOperators> snappedTables;

    for (int i = 0; i < Matrix::numOperators; ++i)
    {
        snappedTables[static_cast<std::size_t> (i)].prepare (sampleRate * factor, 256);
        snappedTables[static_cast<std::size_t> (i)].setWaveform (Waveform::sine);
        snapped.setTable (i, &snappedTables[static_cast<std::size_t> (i)]);
    }

    matrix.reset();
    snapped.reset();
    prime (controls);

    for (int i = 0; i < 2; ++i)
    {
        snapped.setTargets (controls, 0);

        for (auto& table : snappedTables)
            if (table.needsRender())
                table.render (false);
    }

    constexpr int steps = 8;
    constexpr int stepLength = blockSize * factor;

    std::vector<double> rampedOutput;
    std::vector<double> snappedOutput;
    std::vector<double> buffer (stepLength);
    std::vector<double> unused (stepLength);
    double* pointers[] = { buffer.data(), unused.data(), unused.data(), unused.data() };

    for (int step = 0; step < steps; ++step)
    {
        if (step == 2)
            controls.amounts[0][1] = 0.5;

        matrix.setTargets (controls, stepLength);
        renderTables();
        matrix.process (pointers, stepLength);
        rampedOutput.insert (rampedOutput.end(), buffer.begin(), buffer.end());

        snapped.setTargets (controls, step == 2 ? 0 : stepLength);

        for (auto& table : snappedTables)
            if (table.needsRender())
                table.render (false);

        snapped.process (pointers, stepLength);
        snappedOutput.insert (snappedOutput.end(), buffer.begin(), buffer.end());
    }

    // Half a period of depth switched on at once moves the carrier phase by up to pi
    // in one sample. Ramped over a step, the steepest move is the modulated carrier's
    // own, 2 pi 440 (1 + pi) / 192000, plus the ramp's 2 pi 0.5 / 128 per sample.
    EXPECT_LT (maxJump (rampedOutput), 0.1);
    EXPECT_GT (maxJump (snappedOutput), maxJump (rampedOutput));

    // Once the ramp has finished both play the same thing.
    for (std::size_t i = 3 * stepLength; i < rampedOutput.size(); ++i)
        ASSERT_NEAR (snappedOutput[i], rampedOutput[i], 1e-9) << i;
}

TEST_F (PhaseModulationMatrixTests, RampedAmountsFadeOutToThePlainCarrier)
{
    prepare (factor);
    matrix.reset();

    Matrix::Controls controls;
    controls.frequencies = { 440.0, 440.0, 0.0, 0.0 };
    controls.gains = { 1.0, 0.0, 0.0, 0.0 };
    controls.amounts[0][1] = 0.5;

    prime (controls);

    constexpr int steps = 8;
    constexpr int stepLength = blockSize * factor;

    std::vector<double> output;
    std::vector<double> buffer (stepLength);
    std::vector<double> unused (stepLength);
    double* pointers[] = { buffer.data(), unused.data(), unused.data(), unused.data() };

    for (int step = 0; step < steps; ++step)
    {
        if (step == 2)
            controls.amounts[0][1] = 0.0;

        matrix.setTargets (controls, stepLength);
        renderTables();
        matrix.process (pointers, stepLength);
        output.insert (output.end(), buffer.begin(), buffer.end());
    }

    // The link fades out over its step instead of dropping at once.
    EXPECT_LT (maxJump (output), 0.1);

    // Once the ramp has finished the carrier plays unmodulated.
    for (std::size_t i = 3 * stepLength; i < output.size(); ++i)
        ASSERT_NEAR (std::sin (MathConstants<double>::twoPi * 440.0 * static_cast<double> (i) / (sampleRate * factor)), output[i], 1e-4) << i;
}

TEST_F (PhaseModulationMatrixTests, SilentOperatorStillModulates)
{
    prepare (factor);
    matrix.reset();

    Matrix::Controls controls;
    controls.frequencies = { 500.0, 750.0, 0.0, 0.0 };
    controls.gains = { 1.0, 0.0, 0.0, 0.0 };
    controls.amounts[0][1] = 0.5;

    prime (controls);

    EXPECT_TRUE (matrix.isOperatorActive (0));
    EXPECT_TRUE (matrix.isOperatorActive (1));
    EXPECT_TRUE (matrix.isOperatorAudible (0));
    EXPECT_FALSE (matrix.isOperatorAudible (1));

    const auto modulated = generate (controls, static_cast<int> (sampleRate) + 4096);

    // 500 + 750 Hz is a first order sideband, absent without the modulation.
    EXPECT_GT (amplitudeAt (modulated, 4096, 1250.0), 0.1);
}

TEST_F (PhaseModulationMatrixTests, SkippedOperatorKeepsItsCrossfadeTiming)
{
    prepare (factor);
    matrix.reset();

    Matrix::Controls controls;
    controls.frequencies = { 500.0, 500.0, 0.0, 0.0 };
    controls.gains = { 1.0, 0.0, 0.0, 0.0 };

    prime (controls);

    EXPECT_FALSE (matrix.isOperatorActive (1));

    tables[1].setWaveform (Waveform::sawtooth);
    tables[1].render();

    std::vector<double> buffer (4096);
    double* pointers[] = { buffer.data(), buffer.data(), buffer.data(), buffer.data() };

    matrix.setTargets (controls, 4096);
    matrix.process (pointers, 4096);

    // The default crossfade is 64 samples, so it has completed while skipped: the
    // table being played is the sawtooth, and no fade is mixed into the reads.
    const auto index = tables[1].getTableSize() * 3 / 10;
    const auto played = tables[1].getRenderedTable()[static_cast<std::size_t> (index)];
    const auto phase = static_cast<double> (index) / tables[1].getTableSize();

    EXPECT_GT (std::abs (played - std::sin (MathConstants<double>::twoPi * phase)), 0.1);
    EXPECT_NEAR (played, tables[1].getValueAtPhase (phase), 1e-9);
}

TEST_F (PhaseModulationMatrixTests, PhasesAdvanceForSkippedOperators)
{
    prepare (factor);
    matrix.reset();

    Matrix::Controls controls;
    controls.frequencies = { 0.0, 1200.0, 0.0, 0.0 };

    prime (controls);

    std::vector<double> buffer (120);
    double* pointers[] = { buffer.data(), buffer.data(), buffer.data(), buffer.data() };

    matrix.setTargets (controls, 120);
    matrix.process (pointers, 120);

    EXPECT_FALSE (matrix.isOperatorActive (1));
    EXPECT_NEAR (1200.0 * 120.0 / (sampleRate * factor), matrix.getPhase (1), 1e-12);
}

TEST_F (PhaseModulationMatrixTests, PhaseOffsetShiftsTheRead)
{
    prepare (1);
    matrix.reset();

    Matrix::Controls controls;
    controls.frequencies = { 440.0, 440.0, 0.0, 0.0 };
    controls.gains = { 1.0, 1.0, 0.0, 0.0 };
    controls.phaseOffsets = { 0.0, 0.25, 0.0, 0.0 };

    prime (controls);

    std::vector<double> plain (blockSize);
    std::vector<double> shifted (blockSize);
    std::vector<double> unused (blockSize);
    double* pointers[] = { plain.data(), shifted.data(), unused.data(), unused.data() };

    matrix.setTargets (controls, blockSize);
    renderTables();
    matrix.process (pointers, blockSize);

    for (int s = 0; s < blockSize; ++s)
    {
        const auto phase = 440.0 * s / sampleRate;

        EXPECT_NEAR (std::sin (MathConstants<double>::twoPi * phase), plain[static_cast<std::size_t> (s)], 1e-4) << s;
        EXPECT_NEAR (std::sin (MathConstants<double>::twoPi * (phase + 0.25)), shifted[static_cast<std::size_t> (s)], 1e-4) << s;
    }

    EXPECT_NEAR (440.0 * blockSize / sampleRate, matrix.getPhase (1), 1e-12);
}

TEST_F (PhaseModulationMatrixTests, PhaseOffsetRampsWhenNothingElseChanges)
{
    prepare (factor);
    matrix.reset();

    Matrix::Controls controls;
    controls.frequencies = { 440.0, 440.0, 0.0, 0.0 };
    controls.gains = { 1.0, 1.0, 0.0, 0.0 };

    prime (controls);

    constexpr int steps = 8;
    constexpr int stepLength = blockSize * factor;

    std::vector<double> shifted;
    std::vector<double> plain;
    std::vector<double> buffer (stepLength);
    std::vector<double> reference (stepLength);
    std::vector<double> unused (stepLength);
    double* pointers[] = { buffer.data(), reference.data(), unused.data(), unused.data() };

    for (int step = 0; step < steps; ++step)
    {
        if (step == 2)
            controls.phaseOffsets[0] = 0.5;

        matrix.setTargets (controls, stepLength);
        renderTables();
        matrix.process (pointers, stepLength);
        shifted.insert (shifted.end(), buffer.begin(), buffer.end());
        plain.insert (plain.end(), reference.begin(), reference.end());
    }

    // Half a period at once would jump by up to 2. Ramped over a step, the steepest move
    // is 2 pi (440 / 192000 + 0.5 / 128) per sample.
    EXPECT_LT (maxJump (shifted), 0.1);

    // Once the ramp has finished the operator plays half a period ahead of its twin.
    for (std::size_t i = 3 * stepLength; i < shifted.size(); ++i)
        ASSERT_NEAR (-plain[i], shifted[i], 1e-9) << i;
}

TEST_F (PhaseModulationMatrixTests, DISABLED_BenchmarkFourOperators)
{
    constexpr int voices = 8;
    constexpr int seconds = 2;

    using FloatMatrix = PhaseModulationMatrix<float>;
    using FloatTable = WavetableOscillator<float>;

    std::vector<std::array<FloatTable, FloatMatrix::numOperators>> voiceTables (voices);
    std::vector<FloatMatrix> matrices (voices);

    FloatMatrix::Controls controls;
    controls.frequencies = { 220.0, 440.0, 660.0, 110.0 };
    controls.gains = { 0.5, 0.5, 0.0, 0.0 };
    controls.amounts[0][2] = 0.3;
    controls.amounts[1][3] = 0.2;
    controls.amounts[2][2] = 0.5;
    controls.amounts[0][1] = 0.1;

    for (int v = 0; v < voices; ++v)
    {
        matrices[static_cast<std::size_t> (v)].prepare (sampleRate, factor);

        for (int i = 0; i < FloatMatrix::numOperators; ++i)
        {
            auto& table = voiceTables[static_cast<std::size_t> (v)][static_cast<std::size_t> (i)];
            table.prepare (sampleRate * factor, 256);
            table.setWaveform (i == 0 ? Waveform::sawtooth : Waveform::sine);
            matrices[static_cast<std::size_t> (v)].setTable (i, &table);
        }

        matrices[static_cast<std::size_t> (v)].reset();
    }

    std::vector<std::vector<float>> outputs (FloatMatrix::numOperators, std::vector<float> (blockSize * factor));
    float* pointers[FloatMatrix::numOperators];

    for (int i = 0; i < FloatMatrix::numOperators; ++i)
        pointers[i] = outputs[static_cast<std::size_t> (i)].data();

    const auto blocks = static_cast<int> (sampleRate) * seconds / blockSize;
    const auto begin = std::chrono::steady_clock::now();

    for (int block = 0; block < blocks; ++block)
    {
        for (int v = 0; v < voices; ++v)
        {
            auto& m = matrices[static_cast<std::size_t> (v)];
            m.setTargets (controls, blockSize * factor);

            for (auto& table : voiceTables[static_cast<std::size_t> (v)])
                if (table.needsRender())
                    table.render (false);

            m.process (pointers, blockSize * factor);
        }
    }

    const auto elapsed = std::chrono::duration<double, std::micro> (std::chrono::steady_clock::now() - begin).count();
    const auto perVoiceSample = elapsed / (static_cast<double> (voices) * blocks * blockSize);

    std::printf ("PhaseModulationMatrix 4 ops %dx: %.4f us per voice per output sample (budget 0.25)\n", factor, perVoiceSample);
    SUCCEED();
}
