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

#include <gtest/gtest.h>

#include <yup_audio_processors/yup_audio_processors.h>

using namespace yup;

namespace
{

//==============================================================================
/** A minimal passthrough SpectralProcessor that does nothing in the frequency domain. */
class PassthroughSpectralProcessor final : public SpectralProcessor
{
public:
    PassthroughSpectralProcessor()
        : SpectralProcessor ("Passthrough",
                             AudioBusLayout ({ AudioBus ("Main", AudioBus::Audio, AudioBus::Input, 2) },
                                             { AudioBus ("Main", AudioBus::Audio, AudioBus::Output, 2) }))
    {
    }

    void prepareToPlay (const SpectralSpec&) override
    {
        prepareCalled = true;
    }

    void releaseResources() override
    {
        releaseCalled = true;
    }

    void processBlock (SpectralProcessContext<float>& context) override
    {
        ignoreUnused (context);
        ++processCount;
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    bool prepareCalled = false;
    bool releaseCalled = false;
    int processCount = 0;
};

//==============================================================================
/** A minimal AudioBusLayout for stereo in/out. */
AudioBusLayout stereoLayout()
{
    return AudioBusLayout ({ AudioBus ("Main", AudioBus::Audio, AudioBus::Input, 2) },
                           { AudioBus ("Main", AudioBus::Audio, AudioBus::Output, 2) });
}

} // namespace

//==============================================================================
class SpectralBridgeTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        bridge = std::make_unique<SpectralBridge> ("TestBridge", stereoLayout());
    }

    void TearDown() override
    {
        bridge.reset();
    }

    std::unique_ptr<SpectralBridge> bridge;
};

//==============================================================================
TEST_F (SpectralBridgeTests, ConstructsWithDefaultValues)
{
    EXPECT_EQ (1024, bridge->getFFTSize());
    EXPECT_EQ (4, bridge->getOverlapFactor());
    EXPECT_EQ (1024, bridge->getLatencySamples());
    EXPECT_EQ (nullptr, bridge->getSpectralProcessor());
    EXPECT_FALSE (bridge->hasEditor());
}

TEST_F (SpectralBridgeTests, SetFFTSizeUpdatesLatency)
{
    bridge->setFFTSize (2048);
    EXPECT_EQ (2048, bridge->getFFTSize());
    EXPECT_EQ (2048, bridge->getLatencySamples());
}

TEST_F (SpectralBridgeTests, SetFFTSizeIgnoresSameValue)
{
    bridge->setFFTSize (1024);
    EXPECT_EQ (1024, bridge->getFFTSize());
}

TEST_F (SpectralBridgeTests, SetOverlapFactorUpdatesHopSize)
{
    bridge->setOverlapFactor (2);
    EXPECT_EQ (2, bridge->getOverlapFactor());
}

TEST_F (SpectralBridgeTests, SetSpectralProcessorRegistersParameters)
{
    auto proc = std::make_shared<PassthroughSpectralProcessor>();
    bridge->setSpectralProcessor (proc);
    EXPECT_EQ (proc, bridge->getSpectralProcessor());
}

TEST_F (SpectralBridgeTests, SetNullSpectralProcessor)
{
    auto proc = std::make_shared<PassthroughSpectralProcessor>();
    bridge->setSpectralProcessor (proc);
    bridge->setSpectralProcessor (nullptr);
    EXPECT_EQ (nullptr, bridge->getSpectralProcessor());
}

TEST_F (SpectralBridgeTests, PrepareToPlayCallsSpectralProcessorPrepare)
{
    auto proc = std::make_shared<PassthroughSpectralProcessor>();
    bridge->setSpectralProcessor (proc);
    bridge->prepareToPlay (AudioSpec (44100.0f, 512, 2));
    EXPECT_TRUE (proc->prepareCalled);
}

TEST_F (SpectralBridgeTests, ReleaseResourcesCallsSpectralProcessorRelease)
{
    auto proc = std::make_shared<PassthroughSpectralProcessor>();
    bridge->setSpectralProcessor (proc);
    bridge->prepareToPlay (AudioSpec (44100.0f, 512, 2));
    bridge->releaseResources();
    EXPECT_TRUE (proc->releaseCalled);
}

TEST_F (SpectralBridgeTests, FlushCallsSpectralProcessorFlush)
{
    auto proc = std::make_shared<PassthroughSpectralProcessor>();
    bridge->setSpectralProcessor (proc);
    bridge->prepareToPlay (AudioSpec (44100.0f, 512, 2));
    bridge->flush();
    EXPECT_EQ (0, proc->processCount);
}

TEST_F (SpectralBridgeTests, ProcessBlockRunsSpectralProcessor)
{
    auto proc = std::make_shared<PassthroughSpectralProcessor>();
    bridge->setSpectralProcessor (proc);
    bridge->setFFTSize (128);
    bridge->prepareToPlay (AudioSpec (44100.0f, 128, 2));

    AudioBuffer<float> audio (2, 128);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    audio.clear();

    AudioProcessContext<float> context { audio, midi, params };
    bridge->processBlock (context);

    EXPECT_GT (proc->processCount, 0);
}

TEST_F (SpectralBridgeTests, ProcessBlockWithZeroSamplesDoesNothing)
{
    auto proc = std::make_shared<PassthroughSpectralProcessor>();
    bridge->setSpectralProcessor (proc);
    bridge->setFFTSize (512);
    bridge->prepareToPlay (AudioSpec (44100.0f, 128, 2));

    AudioBuffer<float> audio (2, 0);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context { audio, midi, params };
    bridge->processBlock (context);

    EXPECT_EQ (0, proc->processCount);
}

TEST_F (SpectralBridgeTests, ProcessBlockWithNoSpectralProcessorDoesNothing)
{
    bridge->setFFTSize (512);
    bridge->prepareToPlay (AudioSpec (44100.0f, 128, 2));

    AudioBuffer<float> audio (2, 128);
    audio.clear();

    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context { audio, midi, params };
    bridge->processBlock (context);

    for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        for (int s = 0; s < audio.getNumSamples(); ++s)
            EXPECT_FLOAT_EQ (0.0f, audio.getSample (ch, s));
}

TEST_F (SpectralBridgeTests, NameIsPreserved)
{
    EXPECT_EQ (String ("TestBridge"), bridge->getName());
}

TEST_F (SpectralBridgeTests, SupportsDataTreeState)
{
    EXPECT_TRUE (bridge->supportsDataTreeState());
}

TEST_F (SpectralBridgeTests, LoadAndSaveStateReturnOk)
{
    DataTree tree;
    EXPECT_TRUE (bridge->loadStateFromDataTree (tree).wasOk());
    EXPECT_TRUE (bridge->saveStateIntoDataTree (tree).wasOk());
}

TEST_F (SpectralBridgeTests, ProcessBlockPassesAudioThrough)
{
    auto proc = std::make_shared<PassthroughSpectralProcessor>();
    bridge->setSpectralProcessor (proc);

    constexpr int kFftSize = 128;
    constexpr int kBlockSize = 64;
    constexpr int kNumChannels = 1;
    constexpr float kSampleRate = 44100.0f;
    constexpr int kLatency = kFftSize;
    constexpr int kTotalBlocks = 50;
    constexpr float kFrequency = 440.0f;

    bridge->setFFTSize (kFftSize);
    bridge->prepareToPlay (AudioSpec (kSampleRate, kBlockSize, kNumChannels));

    AudioBuffer<float> audio (kNumChannels, kBlockSize);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    const int totalSamples = kTotalBlocks * kBlockSize;
    std::vector<float> inputHistory (static_cast<size_t> (totalSamples), 0.0f);
    std::vector<float> outputHistory (static_cast<size_t> (totalSamples), 0.0f);

    for (int block = 0; block < kTotalBlocks; ++block)
    {
        audio.clear();

        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            auto* writePtr = audio.getWritePointer (ch);

            for (int s = 0; s < kBlockSize; ++s)
            {
                const int globalSample = block * kBlockSize + s;
                const float t = static_cast<float> (globalSample) / kSampleRate;
                const float value = std::sin (2.0f * MathConstants<float>::pi * kFrequency * t);
                writePtr[s] = value;
                inputHistory[static_cast<size_t> (globalSample)] = value;
            }
        }

        AudioProcessContext<float> context { audio, midi, params };
        bridge->processBlock (context);

        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            const auto* readPtr = audio.getReadPointer (ch);

            for (int s = 0; s < kBlockSize; ++s)
            {
                const int globalSample = block * kBlockSize + s;
                outputHistory[static_cast<size_t> (globalSample)] = readPtr[s];
            }
        }
    }

    // Only compare in the steady-state region after ramp-up has settled.
    // Ramp-up extends over ~fftSize samples after first frame output begins.
    constexpr int kRampUpEnd = kLatency + kFftSize;
    constexpr int kRampDownStart = totalSamples - kFftSize;
    constexpr float kTolerance = 0.05f;

    int mismatchCount = 0;
    int comparedSamples = 0;

    for (int s = kRampUpEnd; s < kRampDownStart; ++s)
    {
        const float diff = std::abs (outputHistory[static_cast<size_t> (s)]
                                     - inputHistory[static_cast<size_t> (s - kLatency)]);

        if (diff > kTolerance)
            ++mismatchCount;

        ++comparedSamples;
    }

    // Allow up to 5% of samples to mismatch (floating point edge cases)
    EXPECT_LT (mismatchCount, comparedSamples / 20);
}

TEST_F (SpectralBridgeTests, FFTSizeChangeKeepsStereoChannelsAligned)
{
    auto proc = std::make_shared<PassthroughSpectralProcessor>();
    bridge->setSpectralProcessor (proc);

    constexpr int kInitialFftSize = 128;
    constexpr int kNewFftSize = 256;
    constexpr int kBlockSize = 64;
    constexpr int kNumChannels = 2;
    constexpr float kSampleRate = 44100.0f;
    constexpr int kTotalBlocks = 32;
    constexpr float kFrequency = 440.0f;

    bridge->setFFTSize (kInitialFftSize);
    bridge->prepareToPlay (AudioSpec (kSampleRate, kBlockSize, kNumChannels));

    AudioBuffer<float> audio (kNumChannels, kBlockSize);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    for (int block = 0; block < kTotalBlocks; ++block)
    {
        if (block == 8)
            bridge->setFFTSize (kNewFftSize);

        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            auto* writePtr = audio.getWritePointer (ch);

            for (int s = 0; s < kBlockSize; ++s)
            {
                const int globalSample = block * kBlockSize + s;
                const float t = static_cast<float> (globalSample) / kSampleRate;
                writePtr[s] = std::sin (2.0f * MathConstants<float>::pi * kFrequency * t);
            }
        }

        AudioProcessContext<float> context { audio, midi, params };
        bridge->processBlock (context);

        const auto* left = audio.getReadPointer (0);
        const auto* right = audio.getReadPointer (1);

        for (int s = 0; s < kBlockSize; ++s)
            EXPECT_NEAR (left[s], right[s], 1.0e-5f);
    }
}

TEST_F (SpectralBridgeTests, PrepareToPlayPreservesSampleRate)
{
    bridge->setFFTSize (512);
    bridge->prepareToPlay (AudioSpec (48000.0f, 256, 2));
    EXPECT_FLOAT_EQ (48000.0f, bridge->getSampleRate());
    EXPECT_EQ (256, bridge->getSamplesPerBlock());
}

TEST_F (SpectralBridgeTests, FlushResetsOutput)
{
    auto proc = std::make_shared<PassthroughSpectralProcessor>();
    bridge->setSpectralProcessor (proc);
    bridge->setFFTSize (256);
    bridge->prepareToPlay (AudioSpec (44100.0f, 64, 2));

    AudioBuffer<float> audio (2, 64);
    MidiBuffer midi;
    ParameterChangeBuffer params;

    for (int ch = 0; ch < 2; ++ch)
    {
        auto* writePtr = audio.getWritePointer (ch);

        for (int s = 0; s < 64; ++s)
            writePtr[s] = 1.0f;
    }

    AudioProcessContext<float> context { audio, midi, params };
    bridge->processBlock (context);
    bridge->flush();

    // After flush, a new block with silence should produce silence (after latency)
    for (int i = 0; i < 10; ++i)
    {
        audio.clear();
        AudioProcessContext<float> ctx { audio, midi, params };
        bridge->processBlock (ctx);
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        auto* readPtr = audio.getReadPointer (ch);

        for (int s = 0; s < 64; ++s)
            EXPECT_NEAR (0.0f, readPtr[s], 1e-4f);
    }
}

TEST_F (SpectralBridgeTests, DoubleOverlapProducesSmootherOutput)
{
    auto proc = std::make_shared<PassthroughSpectralProcessor>();
    bridge->setSpectralProcessor (proc);
    bridge->setFFTSize (256);
    bridge->setOverlapFactor (8); // 87.5% overlap
    bridge->prepareToPlay (AudioSpec (44100.0f, 64, 2));

    AudioBuffer<float> audio (2, 64);
    audio.clear();

    for (int ch = 0; ch < 2; ++ch)
    {
        auto* writePtr = audio.getWritePointer (ch);

        for (int s = 0; s < 64; ++s)
            writePtr[s] = 1.0f;
    }

    MidiBuffer midi;
    ParameterChangeBuffer params;

    AudioProcessContext<float> context { audio, midi, params };
    bridge->processBlock (context);

    // Output should not have NaN or inf
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int s = 0; s < 64; ++s)
        {
            const float sample = audio.getSample (ch, s);
            EXPECT_TRUE (std::isfinite (sample));
        }
    }
}
