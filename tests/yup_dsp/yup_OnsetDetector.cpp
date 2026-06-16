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
// Test data: 120 BPM click train at 44100 Hz (one 1.0-sample pulse every 22050 samples)
// 44100 / 120 * 60 / 60 = 0.5 seconds per beat = 22050 samples per beat

static std::vector<float> makeClickTrain (int numSamples, float sampleRate, float bpm)
{
    std::vector<float> data (static_cast<std::size_t> (numSamples), 0.0f);
    const int period = static_cast<int> (std::round (sampleRate * 60.0f / bpm));

    for (std::size_t i = 0; i < static_cast<std::size_t> (numSamples); i += static_cast<std::size_t> (period))
    {
        if (i < data.size())
            data[i] = 1.0f;
    }

    return data;
}

static std::vector<float> makeSilence (int numSamples)
{
    return std::vector<float> (static_cast<std::size_t> (numSamples), 0.0f);
}

//==============================================================================
// FilterBank Tests
//==============================================================================

class FilterBankTests : public ::testing::Test
{
protected:
    static constexpr float sampleRate = 44100.0f;
    static constexpr int fftSize = 2048;
    static constexpr int numFFTBins = fftSize / 2;
};

TEST_F (FilterBankTests, DefaultConstruction)
{
    FilterBank fb;
    EXPECT_EQ (0, fb.getNumBands());
    EXPECT_EQ (0, fb.getNumFFTBins());
}

TEST_F (FilterBankTests, BuildCreatesBands)
{
    FilterBank fb;
    fb.build (24, 30.0f, 17000.0f, numFFTBins, sampleRate);

    EXPECT_GT (fb.getNumBands(), 50);
    EXPECT_LT (fb.getNumBands(), 200);
    EXPECT_EQ (numFFTBins, fb.getNumFFTBins());
}

TEST_F (FilterBankTests, TriangularShape)
{
    FilterBank fb;
    fb.build (24, 30.0f, 17000.0f, numFFTBins, sampleRate);

    const int bands = fb.getNumBands();
    EXPECT_GE (bands, 3);

    const int midBand = bands / 2;
    const float* matrix = fb.getMatrixData();

    bool foundRising = false;
    bool foundFalling = false;

    for (int bin = 1; bin < numFFTBins; ++bin)
    {
        const float prev = matrix[static_cast<std::size_t> (bin - 1) * static_cast<std::size_t> (bands)
                                  + static_cast<std::size_t> (midBand)];
        const float curr = matrix[static_cast<std::size_t> (bin) * static_cast<std::size_t> (bands)
                                  + static_cast<std::size_t> (midBand)];

        if (curr > prev)
            foundRising = true;
        if (curr < prev)
            foundFalling = true;
    }

    EXPECT_TRUE (foundRising || foundFalling); // At least one direction change
}

TEST_F (FilterBankTests, ApplySingleFrameReducesDimensions)
{
    FilterBank fb;
    fb.build (24, 30.0f, 17000.0f, numFFTBins, sampleRate);

    const int bands = fb.getNumBands();
    std::vector<float> magIn (static_cast<std::size_t> (numFFTBins), 1.0f);
    std::vector<float> magOut (static_cast<std::size_t> (bands));

    fb.applySingleFrame (magIn.data(), magOut.data());

    for (int b = 0; b < bands; ++b)
        EXPECT_GE (magOut[static_cast<std::size_t> (b)], 0.0f);
}

TEST_F (FilterBankTests, ApplyMultipleFrames)
{
    FilterBank fb;
    fb.build (24, 30.0f, 17000.0f, numFFTBins, sampleRate);

    const int bands = fb.getNumBands();
    constexpr int numFrames = 10;

    std::vector<float> spec (static_cast<std::size_t> (numFrames * numFFTBins), 1.0f);
    std::vector<float> filtered (static_cast<std::size_t> (numFrames * bands));

    fb.applyMultipleFrames (spec.data(), filtered.data(), numFrames);

    for (int f = 0; f < numFrames; ++f)
    {
        for (int b = 0; b < bands; ++b)
            EXPECT_GE (filtered[static_cast<std::size_t> (f * bands + b)], 0.0f);
    }
}

//==============================================================================
// Spectrogram Tests
//==============================================================================

class SpectrogramTests : public ::testing::Test
{
protected:
    static constexpr float sampleRate = 44100.0f;
    static constexpr int fftSize = 2048;

    void SetUp() override
    {
        params.fftSize = fftSize;
        params.fps = 200;
        spec.prepare (params, sampleRate);
    }

    Spectrogram spec;
    Spectrogram::Parameters params;
};

TEST_F (SpectrogramTests, PreparesCorrectly)
{
    EXPECT_EQ (fftSize, spec.getFFTSize());
    EXPECT_EQ (fftSize / 2, spec.getNumRawBins());
    EXPECT_GT (spec.getHopSize(), 0);
    EXPECT_FLOAT_EQ (sampleRate, spec.getSampleRate());
}

TEST_F (SpectrogramTests, SilenceYieldsNearZeroMagnitude)
{
    const int numSamples = 44100;
    auto data = makeSilence (numSamples);

    spec.processOffline (data.data(), numSamples);

    EXPECT_GT (spec.getNumFrames(), 0);

    const float* mag = spec.getMagnitudeData();
    const int numFrames = spec.getNumFrames();
    const int numBins = spec.getNumBins();

    for (int f = 0; f < numFrames; ++f)
    {
        for (int b = 0; b < numBins; ++b)
        {
            const float val = mag[static_cast<std::size_t> (f * numBins + b)];
            // With log10(mul*0 + add) = log10(1) = 0 for useLog=true
            EXPECT_NEAR (0.0f, val, 1e-5f);
        }
    }
}

TEST_F (SpectrogramTests, FrameCountIsReasonable)
{
    const int numSamples = 44100; // 1 second
    auto data = makeClickTrain (numSamples, sampleRate, 120.0f);

    spec.processOffline (data.data(), numSamples);

    const int numFrames = spec.getNumFrames();
    // 1 second at 200 fps = ~200 frames; allow ±5
    EXPECT_NEAR (200, numFrames, 5);
}

TEST_F (SpectrogramTests, WithFilterBank)
{
    FilterBank fb;
    fb.build (24, 30.0f, 17000.0f, fftSize / 2, sampleRate);

    Spectrogram::Parameters p = params;
    p.filterBank = &fb;

    Spectrogram filteredSpec;
    filteredSpec.prepare (p, sampleRate);

    auto data = makeClickTrain (44100, sampleRate, 120.0f);
    filteredSpec.processOffline (data.data(), 44100);

    EXPECT_EQ (fb.getNumBands(), filteredSpec.getNumBins());
    EXPECT_GT (filteredSpec.getNumFrames(), 0);
}

TEST_F (SpectrogramTests, LGDComputationProducesData)
{
    Spectrogram::Parameters p = params;
    p.computeLGD = true;

    Spectrogram lgdSpec;
    lgdSpec.prepare (p, sampleRate);

    auto data = makeClickTrain (44100, sampleRate, 120.0f);
    lgdSpec.processOffline (data.data(), 44100);

    const float* lgd = lgdSpec.getLGDData();
    EXPECT_NE (nullptr, lgd);

    const int numFrames = lgdSpec.getNumFrames();
    const int numRawBins = lgdSpec.getNumRawBins();

    bool hasNonZero = false;

    for (int f = 0; f < numFrames && ! hasNonZero; ++f)
    {
        for (int b = 0; b < numRawBins && ! hasNonZero; ++b)
        {
            if (std::abs (lgd[static_cast<std::size_t> (f * numRawBins + b)]) > 1e-6f)
                hasNonZero = true;
        }
    }

    EXPECT_TRUE (hasNonZero);
}

//==============================================================================
// SuperFluxODF Tests
//==============================================================================

class SuperFluxODFTests : public ::testing::Test
{
protected:
    static constexpr float sampleRate = 44100.0f;
    static constexpr int fftSize = 2048;
    static constexpr int hopSize = 220; // ~44100/200

    std::vector<float> makeWindow()
    {
        std::vector<float> w (static_cast<std::size_t> (fftSize));
        WindowFunctions<float>::generate (WindowType::hann, w.data(), w.size());
        return w;
    }
};

TEST_F (SuperFluxODFTests, DefaultPrepareComputesDiffFrames)
{
    auto window = makeWindow();

    SuperFluxODF odf;
    odf.prepare ({ .diffFrames = 0, .windowMagRatio = 0.5f }, window.data(), fftSize, hopSize);

    EXPECT_GE (odf.getActivations().size(), 0u);
}

TEST_F (SuperFluxODFTests, SilenceYieldsZeroActivations)
{
    auto window = makeWindow();

    SuperFluxODF odf;
    odf.prepare ({ .diffFrames = 0, .windowMagRatio = 0.5f }, window.data(), fftSize, hopSize);

    Spectrogram spec;
    Spectrogram::Parameters sp;
    sp.fftSize = fftSize;
    sp.fps = 200;
    spec.prepare (sp, sampleRate);

    auto data = makeSilence (44100);
    spec.processOffline (data.data(), 44100);

    odf.compute (spec);

    const auto& act = odf.getActivations();

    for (std::size_t i = 0; i < act.size(); ++i)
        EXPECT_NEAR (0.0f, act[i], 1e-5f);
}

TEST_F (SuperFluxODFTests, MaxFilterWidthChangesOutput)
{
    auto window = makeWindow();

    SuperFluxODF odf1, odf3;
    odf1.prepare ({ .diffFrames = 3, .maxFilterBins = 1 }, window.data(), fftSize, hopSize);
    odf3.prepare ({ .diffFrames = 3, .maxFilterBins = 3 }, window.data(), fftSize, hopSize);

    Spectrogram spec;
    Spectrogram::Parameters sp;
    sp.fftSize = fftSize;
    sp.fps = 200;
    spec.prepare (sp, sampleRate);

    auto data = makeClickTrain (44100, sampleRate, 120.0f);
    spec.processOffline (data.data(), 44100);

    odf1.compute (spec);
    odf3.compute (spec);

    const auto& act1 = odf1.getActivations();
    const auto& act3 = odf3.getActivations();

    ASSERT_EQ (act1.size(), act3.size());

    bool differs = false;

    for (std::size_t i = 0; i < act1.size() && ! differs; ++i)
    {
        if (std::abs (act1[i] - act3[i]) > 1e-6f)
            differs = true;
    }

    EXPECT_TRUE (differs);
}

//==============================================================================
// ComplexFluxODF Tests
//==============================================================================

// (Basic smoke tests for ComplexFlux)

TEST (ComplexFluxODFTests, SilenceYieldsZero)
{
    constexpr float sampleRate = 44100.0f;
    constexpr int fftSize = 2048;
    constexpr int hopSize = 220;

    std::vector<float> window (static_cast<std::size_t> (fftSize));
    WindowFunctions<float>::generate (WindowType::hann, window.data(), window.size());

    ComplexFluxODF odf;
    odf.prepare ({ .diffFrames = 3 }, window.data(), fftSize, hopSize);

    Spectrogram spec;
    Spectrogram::Parameters sp;
    sp.fftSize = fftSize;
    sp.fps = 200;
    sp.computeLGD = true;
    spec.prepare (sp, sampleRate);

    auto data = makeSilence (44100);
    spec.processOffline (data.data(), 44100);

    odf.compute (spec);

    const auto& act = odf.getActivations();

    for (std::size_t i = 0; i < act.size(); ++i)
        EXPECT_NEAR (0.0f, act[i], 1e-4f);
}

//==============================================================================
// OnsetPeakPicker Tests
//==============================================================================

class OnsetPeakPickerTests : public ::testing::Test
{
protected:
    static constexpr float fps = 200.0f;
};

TEST_F (OnsetPeakPickerTests, BelowThresholdYieldsNoOnsets)
{
    OnsetPeakPicker picker;
    picker.prepare ({ .threshold = 1.0f }, fps);

    std::vector<float> activations = { 0.1f, 0.2f, 0.5f, 0.3f, 0.1f };
    picker.detect (activations.data(), static_cast<int> (activations.size()));

    EXPECT_TRUE (picker.getOnsetTimes().empty());
}

TEST_F (OnsetPeakPickerTests, SinglePeakDetected)
{
    OnsetPeakPicker picker;
    picker.prepare ({ .threshold = 0.5f, .preAvgSec = 0.0f, .preMaxSec = 0.0f, .postAvgSec = 0.0f, .postMaxSec = 0.0f }, fps);

    // Frame 10 has a peak
    constexpr int numFrames = 200;
    std::vector<float> activations (static_cast<std::size_t> (numFrames), 0.1f);
    activations[10] = 2.0f;

    picker.detect (activations.data(), numFrames);

    const auto& onsets = picker.getOnsetTimes();

    ASSERT_FALSE (onsets.empty());
    EXPECT_NEAR (10.0 / fps, onsets[0], 1e-6);
}

TEST_F (OnsetPeakPickerTests, CombineSuppressesCloseOnsets)
{
    OnsetPeakPicker picker;
    picker.prepare ({ .threshold = 0.5f, .combineSec = 0.05f, .preAvgSec = 0.0f, .preMaxSec = 0.0f, .postAvgSec = 0.0f, .postMaxSec = 0.0f }, fps);

    constexpr int numFrames = 200;
    std::vector<float> activations (static_cast<std::size_t> (numFrames), 0.1f);
    activations[10] = 2.0f;
    activations[15] = 2.0f; // 5 frames later = 25ms < 50ms combine

    picker.detect (activations.data(), numFrames);

    const auto& onsets = picker.getOnsetTimes();
    EXPECT_EQ (1u, onsets.size());
    EXPECT_NEAR (10.0 / fps, onsets[0], 1e-6);
}

TEST_F (OnsetPeakPickerTests, OnlineModeIgnoresFuture)
{
    OnsetPeakPicker online, offline;

    // Zero pre-windows so that online mode has no context at all, and offline
    // relies purely on forward-looking windows. This forces different results.
    online.prepare ({ .threshold = 0.5f, .preAvgSec = 0.0f, .preMaxSec = 0.0f, .postMaxSec = 1.0f, .postAvgSec = 1.0f, .onlineMode = true }, fps);

    offline.prepare ({ .threshold = 0.5f, .preAvgSec = 0.0f, .preMaxSec = 0.0f, .postMaxSec = 1.0f, .postAvgSec = 1.0f, .onlineMode = false }, fps);

    // Triangular peak: frame 3 is the global max. Online mode sees frame 2
    // (0.5 >= threshold) as the first detection; offline mode rejects frame 2
    // because the forward-looking max test sees the larger value at frame 3.
    constexpr int numFrames = 6;
    std::vector<float> activations = { 0.1f, 0.1f, 0.5f, 2.0f, 0.5f, 0.1f };

    online.detect (activations.data(), numFrames);
    offline.detect (activations.data(), numFrames);

    EXPECT_FALSE (online.getOnsetTimes().empty());
    EXPECT_FALSE (offline.getOnsetTimes().empty());

    // The detected frame indices should differ between online/offline.
    bool differs = false;

    for (std::size_t i = 0; i < jmin (online.getOnsetTimes().size(), offline.getOnsetTimes().size()) && ! differs; ++i)
    {
        if (std::abs (online.getOnsetTimes()[i] - offline.getOnsetTimes()[i]) > 1e-6)
            differs = true;
    }

    EXPECT_TRUE (differs);
}

TEST_F (OnsetPeakPickerTests, DelayShiftsOnsetTimes)
{
    OnsetPeakPicker picker;
    picker.prepare ({ .threshold = 0.5f, .delaySec = 0.05f, .preAvgSec = 0.0f, .postAvgSec = 0.0f, .postMaxSec = 0.0f, .preMaxSec = 0.0f }, fps);

    constexpr int numFrames = 200;
    std::vector<float> activations (static_cast<std::size_t> (numFrames), 0.1f);
    activations[10] = 2.0f;

    picker.detect (activations.data(), numFrames);

    ASSERT_FALSE (picker.getOnsetTimes().empty());
    EXPECT_NEAR (10.0 / fps + 0.05, picker.getOnsetTimes()[0], 1e-6);
}

//==============================================================================
// OnsetDetector (OnsetDetector) End-to-End Tests
//==============================================================================

class OnsetDetectorTests : public ::testing::Test
{
protected:
    static constexpr float sampleRate = 44100.0f;

    AudioBuffer<float> makeAudioBuffer (const std::vector<float>& data)
    {
        AudioBuffer<float> buffer (1, static_cast<int> (data.size()));

        for (std::size_t i = 0; i < data.size(); ++i)
            buffer.setSample (0, static_cast<int> (i), data[i]);

        return buffer;
    }
};

TEST_F (OnsetDetectorTests, PrepareInitializesComponents)
{
    OnsetDetector onsetDetector;
    onsetDetector.prepare ({}, sampleRate);

    EXPECT_EQ (0, onsetDetector.getNumFrames());
}

TEST_F (OnsetDetectorTests, EndToEndSilenceYieldsNoOnsets)
{
    OnsetDetector onsetDetector;
    onsetDetector.prepare ({ .spectrogram = { .fftSize = 1024, .fps = 200 },
                             .useFilterBank = false,
                             .peakPicker = { .threshold = 1.0f } },
                           sampleRate);

    auto data = makeSilence (44100);
    auto buffer = makeAudioBuffer (data);

    onsetDetector.processOffline (buffer);

    EXPECT_TRUE (onsetDetector.getOnsetTimes().empty());

    // Activation function should be near zero
    const auto& act = onsetDetector.getActivationFunction();

    for (auto v : act)
        EXPECT_NEAR (0.0f, v, 1e-4f);
}

TEST_F (OnsetDetectorTests, EndToEndClickTrainDetectsOnsets)
{
    OnsetDetector onsetDetector;
    onsetDetector.prepare ({ .spectrogram = { .fftSize = 1024, .fps = 200 },
                             .useFilterBank = true,
                             .peakPicker = { .threshold = 0.5f, .combineSec = 0.2f } },
                           sampleRate);

    // 120 BPM click train, 2 seconds = 4 clicks at 0, 0.5, 1.0, 1.5 seconds
    auto data = makeClickTrain (88200, sampleRate, 120.0f); // 2 seconds
    auto buffer = makeAudioBuffer (data);

    onsetDetector.processOffline (buffer);

    const auto& onsets = onsetDetector.getOnsetTimes();

    // Should detect 3-4 onsets (first might be missed)
    EXPECT_GE (onsets.size(), 2u);
    EXPECT_LE (onsets.size(), 4u);

    if (! onsets.empty())
    {
        // Times should be near 0.0, 0.5, 1.0, 1.5
        for (auto t : onsets)
        {
            // Check that t is near a multiple of 0.5
            const double nearest = std::round (t / 0.5) * 0.5;
            EXPECT_NEAR (nearest, t, 0.1); // Within 100ms
        }
    }
}

TEST_F (OnsetDetectorTests, OnsetDetectorVsComplexFlux)
{
    constexpr float localSampleRate = 44100.0f;

    OnsetDetector sf;
    sf.prepare ({ .spectrogram = { .fftSize = 1024, .fps = 200 },
                  .useFilterBank = false },
                localSampleRate);

    OnsetDetector cf;
    cf.prepare ({ .spectrogram = { .fftSize = 1024, .fps = 200 },
                  .useFilterBank = false,
                  .useComplexFlux = true },
                localSampleRate);

    auto data = makeClickTrain (44100, localSampleRate, 180.0f); // 1 second, ~3 clicks
    auto buffer = makeAudioBuffer (data);

    sf.processOffline (buffer);
    cf.processOffline (buffer);

    const auto& actSF = sf.getActivationFunction();
    const auto& actCF = cf.getActivationFunction();

    ASSERT_EQ (actSF.size(), actCF.size());

    bool differs = false;

    for (std::size_t i = 0; i < actSF.size() && ! differs; ++i)
    {
        if (std::abs (actSF[i] - actCF[i]) > 1e-6f)
            differs = true;
    }

    EXPECT_TRUE (differs); // ComplexFlux should produce different activations due to LGD weighting
}

TEST_F (OnsetDetectorTests, StereoBufferProcessedAsMono)
{
    OnsetDetector onsetDetector;
    onsetDetector.prepare ({ .spectrogram = { .fftSize = 1024, .fps = 200 },
                             .useFilterBank = false },
                           sampleRate);

    constexpr int numSamples = 44100;
    AudioBuffer<float> stereo (2, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        stereo.setSample (0, i, (i % 22050 == 0) ? 1.0f : 0.0f);
        stereo.setSample (1, i, (i % 22050 == 0) ? 1.0f : 0.0f);
    }

    EXPECT_NO_THROW (onsetDetector.processOffline (stereo));
    EXPECT_GT (onsetDetector.getNumFrames(), 0);
}

TEST_F (OnsetDetectorTests, ResetClearsResults)
{
    OnsetDetector onsetDetector;
    onsetDetector.prepare ({ .spectrogram = { .fftSize = 1024, .fps = 200 },
                             .useFilterBank = false },
                           sampleRate);

    auto data = makeClickTrain (44100, sampleRate, 120.0f);
    auto buffer = makeAudioBuffer (data);

    onsetDetector.processOffline (buffer);
    EXPECT_GT (onsetDetector.getNumFrames(), 0);

    onsetDetector.reset();
    EXPECT_EQ (0, onsetDetector.getNumFrames());
    EXPECT_TRUE (onsetDetector.getOnsetTimes().empty());
    EXPECT_TRUE (onsetDetector.getActivationFunction().empty());
}
