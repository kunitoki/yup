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

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>

//==============================================================================
/** A sine of a phase in cycles, from a table: cheap enough for many oscillators per sample. */
inline float sineOfCycles (float cycles)
{
    static constexpr int size = 4096;

    static const auto table = []
    {
        std::array<float, size + 1> values {};
        for (int i = 0; i <= size; ++i)
            values[(std::size_t) i] = static_cast<float> (std::sin (yup::MathConstants<double>::twoPi * i / size));

        return values;
    }();

    const auto position = (cycles - std::floor (cycles)) * size;
    const auto index = std::min (static_cast<int> (position), size - 1); // a phase just below 0 rounds up to 1
    const auto fraction = position - static_cast<float> (index);
    const auto a = table[(std::size_t) index];

    return a + (table[(std::size_t) index + 1] - a) * fraction;
}

/** A sawtooth from a phase in cycles, its jump smoothed by a polynomial band limited step so it
    does not alias much. The increment is the phase step per sample. */
inline float polyBlepSaw (float phase, float increment)
{
    auto out = 2.0f * phase - 1.0f;

    if (phase < increment)
    {
        const auto t = phase / increment;
        out -= t + t - t * t - 1.0f;
    }
    else if (phase > 1.0f - increment)
    {
        const auto t = (phase - 1.0f) / increment;
        out -= t * t + t + t + 1.0f;
    }

    return out;
}

//==============================================================================
/**
    What a demo plays while it is open. Both methods run on the audio thread.
*/
class SoundSource
{
public:
    virtual ~SoundSource() = default;

    /** Resets the source to silence, before it is played again or when the sample rate changes. */
    virtual void prepare (double sampleRate) = 0;

    /** Adds the next block into the two channels. */
    virtual void render (float* left, float* right, int numSamples) = 0;
};

//==============================================================================
/**
    The one audio output of the demos, silent until a demo is opened.

    The browser only starts audio from a user gesture, so the output is opened
    suspended and the page resumes it on the click that opens a demo (see home.js).
    play() fades out what was playing, then fades the new source in; play (nullptr)
    fades out to silence.
*/
class Sound : private yup::AudioIODeviceCallback
{
public:
    //==============================================================================
    Sound()
    {
        deviceManager.initialiseWithDefaultDevices (0, 2);

        // The page resumes the audio without the device knowing, so it may never announce its start.
        if (auto* device = deviceManager.getCurrentAudioDevice())
            sampleRate.store (device->getCurrentSampleRate(), std::memory_order_relaxed);

        deviceManager.addAudioCallback (this);
    }

    ~Sound() override
    {
        deviceManager.removeAudioCallback (this);
        deviceManager.closeAudioDevice();
    }

    //==============================================================================
    /** Fades the given source in, after fading out the one playing; nullptr fades out. */
    void play (SoundSource* source)
    {
        requested.store (source, std::memory_order_release);
    }

    /** What is heard of the output, for the demos to react to. */
    const AudioAnalysis& getAnalysis() const { return analysis; }

private:
    //==============================================================================
    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        sampleRate.store (device->getCurrentSampleRate(), std::memory_order_relaxed);
    }

    void audioDeviceStopped() override {}

    void audioDeviceIOCallbackWithContext (const float* const*,
                                           int,
                                           float* const* outputs,
                                           int numOutputs,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext&) override
    {
        for (int channel = 0; channel < numOutputs; ++channel)
        {
            if (outputs[channel] != nullptr)
                std::fill_n (outputs[channel], numSamples, 0.0f);
        }

        if (numOutputs <= 0 || outputs[0] == nullptr)
            return;

        const auto rate = sampleRate.load (std::memory_order_relaxed);
        auto* wanted = requested.load (std::memory_order_acquire);

        // A new source comes in once the previous one has faded out: quickly when it is handing over
        // to another demo, gently when the demo was closed.
        if (wanted != active && gain == 0.0f)
        {
            active = wanted;
            preparedRate = 0.0;
        }

        if (active == nullptr || rate <= 0.0)
        {
            analysis.push (outputs[0], numOutputs > 1 ? outputs[1] : nullptr, numSamples, rate > 0.0 ? rate : 48000.0);
            return;
        }

        if (preparedRate != rate)
        {
            active->prepare (rate);
            preparedRate = rate;
        }

        const auto target = wanted == active ? 1.0f : 0.0f;
        const auto handingOver = wanted != nullptr && wanted != active;
        const auto handoverStep = 1.0f / (handoverSeconds * static_cast<float> (rate));
        const auto fadeSeconds = target > gain ? fadeInSeconds : fadeOutSeconds;
        const auto follow = 1.0f - std::exp (-1.0f / (fadeSeconds * static_cast<float> (rate)));

        auto* right = numOutputs > 1 ? outputs[1] : nullptr;

        for (int start = 0; start < numSamples; start += chunkSize)
        {
            const auto count = std::min (chunkSize, numSamples - start);

            std::fill_n (leftScratch.data(), count, 0.0f);
            std::fill_n (rightScratch.data(), count, 0.0f);
            active->render (leftScratch.data(), rightScratch.data(), count);

            for (int i = 0; i < count; ++i)
            {
                gain = handingOver ? std::max (0.0f, gain - handoverStep) : gain + (target - gain) * follow;

                const auto l = clipper.processSample (leftScratch[(std::size_t) i] * gain);
                const auto r = clipper.processSample (rightScratch[(std::size_t) i] * gain);

                if (right != nullptr)
                {
                    outputs[0][start + i] = l;
                    right[start + i] = r;
                }
                else
                {
                    outputs[0][start + i] = 0.5f * (l + r);
                }
            }
        }

        if (target == 0.0f && gain < 1.0e-3f)
            gain = 0.0f;

        analysis.push (outputs[0], right, numSamples, rate);
    }

    //==============================================================================
    static constexpr float fadeInSeconds = 0.25f;
    static constexpr float handoverSeconds = 0.06f; // a linear fade out when another demo's sound is waiting
    static constexpr float fadeOutSeconds = 0.3f;
    static constexpr int chunkSize = 256;

    yup::AudioDeviceManager deviceManager;
    std::atomic<SoundSource*> requested { nullptr };
    std::atomic<double> sampleRate { 0.0 };

    // Audio thread only
    AudioAnalysis analysis;
    yup::SoftClipper<float> clipper { 1.0, 0.6 }; // rounds off peaks of overlapping notes instead of clipping them
    SoundSource* active = nullptr;
    double preparedRate = 0.0;
    float gain = 0.0f;
    std::array<float, chunkSize> leftScratch {}, rightScratch {};
};
