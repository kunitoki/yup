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

#include "RetraceAudioAnalysis.h"

#include <yup_dsp/yup_dsp.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace RetraceAudioAnalysis
{
namespace
{
constexpr double silenceDb = -144.0;
constexpr float onsetLevel = 0.001f;  // -60 dBFS
constexpr float silentLevel = 0.0001f; // -80 dBFS
constexpr float clipLevel = 0.9989f;  // -0.01 dBFS
constexpr int fftSize = 4096;

double toDb (double gain)
{
    return gain > 0.0 ? yup::jmax (silenceDb, 20.0 * std::log10 (gain)) : silenceDb;
}

double rounded (double value)
{
    return std::round (value * 1000.0) / 1000.0;
}

void set (yup::var& object, const yup::Identifier& name, const yup::var& value)
{
    if (auto* dynamicObject = object.getDynamicObject())
        dynamicObject->setProperty (name, value);
}

std::vector<float> mixToMono (const yup::AudioBuffer<float>& audio)
{
    std::vector<float> mono (static_cast<std::size_t> (audio.getNumSamples()), 0.0f);
    const auto scale = 1.0f / static_cast<float> (yup::jmax (1, audio.getNumChannels()));

    for (int channel = 0; channel < audio.getNumChannels(); ++channel)
    {
        const auto* samples = audio.getReadPointer (channel);

        for (std::size_t index = 0; index < mono.size(); ++index)
            mono[index] += samples[index] * scale;
    }

    return mono;
}

/** Averages the power spectrum of Hann windowed, half overlapping frames. */
std::vector<double> measurePowerSpectrum (const std::vector<float>& mono)
{
    yup::FFTProcessor<float> fft (fftSize);
    std::vector<float> frame (fftSize);
    std::vector<float> spectrum (fftSize * 2);
    std::vector<double> power (fftSize / 2 + 1, 0.0);
    const auto numSamples = static_cast<int> (mono.size());

    for (int start = 0; start == 0 || start + fftSize <= numSamples; start += fftSize / 2)
    {
        for (int index = 0; index < fftSize; ++index)
        {
            const auto window = 0.5f - 0.5f * std::cos (yup::MathConstants<float>::twoPi * static_cast<float> (index) / static_cast<float> (fftSize));
            frame[static_cast<std::size_t> (index)] = start + index < numSamples ? mono[static_cast<std::size_t> (start + index)] * window : 0.0f;
        }

        fft.performRealFFTForward (frame.data(), spectrum.data());

        for (std::size_t bin = 0; bin < power.size(); ++bin)
        {
            const auto real = static_cast<double> (spectrum[bin * 2]);
            const auto imaginary = static_cast<double> (spectrum[bin * 2 + 1]);
            power[bin] += real * real + imaginary * imaginary;
        }
    }

    return power;
}

void addSpectrum (yup::var& result, const std::vector<float>& mono, double sampleRate)
{
    const auto power = measurePowerSpectrum (mono);
    const auto binWidth = sampleRate / fftSize;

    double weighted = 0.0;
    double total = 0.0;

    for (std::size_t bin = 1; bin < power.size(); ++bin)
    {
        weighted += static_cast<double> (bin) * binWidth * power[bin];
        total += power[bin];
    }

    set (result, "spectralCentroid", total > 0.0 ? std::round (weighted / total) : 0.0);

    constexpr std::pair<const char*, double> centers[] { { "31.5", 31.5 }, { "63", 63.0 }, { "125", 125.0 }, { "250", 250.0 }, { "500", 500.0 },
                                                         { "1k", 1000.0 }, { "2k", 2000.0 }, { "4k", 4000.0 }, { "8k", 8000.0 }, { "16k", 16000.0 } };

    std::vector<double> energies;

    for (const auto& [label, center] : centers)
    {
        double energy = 0.0;

        for (std::size_t bin = 1; bin < power.size(); ++bin)
        {
            const auto frequency = static_cast<double> (bin) * binWidth;

            if (frequency >= center / yup::MathConstants<double>::sqrt2 && frequency < center * yup::MathConstants<double>::sqrt2)
                energy += power[bin];
        }

        energies.push_back (energy);
    }

    const auto loudest = *std::max_element (energies.begin(), energies.end());
    auto octaveBands = yup::var (std::make_unique<yup::DynamicObject>());

    for (std::size_t index = 0; index < energies.size(); ++index)
    {
        const auto energy = energies[index];
        const auto relative = loudest > 0.0 && energy > 0.0 ? yup::jmax (silenceDb, 10.0 * std::log10 (energy / loudest)) : silenceDb;
        set (octaveBands, centers[index].first, rounded (relative));
    }

    set (result, "octaveBands", octaveBands);
}
} // namespace

yup::var analyse (const yup::AudioBuffer<float>& audio, double sampleRate)
{
    auto result = yup::var (std::make_unique<yup::DynamicObject>());
    const auto numSamples = audio.getNumSamples();
    const auto numChannels = audio.getNumChannels();

    set (result, "duration", rounded (sampleRate > 0.0 ? numSamples / sampleRate : 0.0));
    set (result, "sampleRate", sampleRate);
    set (result, "channels", numChannels);

    if (numSamples == 0 || numChannels == 0 || sampleRate <= 0.0)
        return result;

    float peak = 0.0f;
    int peakIndex = 0;
    int onsetIndex = -1;
    int lastAudibleIndex = -1;
    int clipped = 0;
    double sumSquares = 0.0;
    double sum = 0.0;

    for (int index = 0; index < numSamples; ++index)
    {
        float loudest = 0.0f;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto sample = audio.getSample (channel, index);
            const auto magnitude = std::abs (sample);
            loudest = yup::jmax (loudest, magnitude);
            sumSquares += static_cast<double> (sample) * sample;
            sum += sample;

            if (magnitude >= clipLevel)
                ++clipped;
        }

        if (loudest > peak)
        {
            peak = loudest;
            peakIndex = index;
        }

        if (onsetIndex < 0 && loudest > onsetLevel)
            onsetIndex = index;

        if (loudest > silentLevel)
            lastAudibleIndex = index;
    }

    const auto count = static_cast<double> (numSamples) * numChannels;
    const auto rms = std::sqrt (sumSquares / count);

    set (result, "peakDb", rounded (toDb (peak)));
    set (result, "rmsDb", rounded (toDb (rms)));
    set (result, "crestDb", rms > 0.0 ? rounded (toDb (peak) - toDb (rms)) : 0.0);
    set (result, "dcOffset", std::round (sum / count * 1.0e6) / 1.0e6);
    set (result, "clippedSamples", clipped);
    set (result, "onset", onsetIndex >= 0 ? yup::var (rounded (onsetIndex / sampleRate)) : yup::var());
    set (result, "timeToPeak", rounded (peakIndex / sampleRate));
    set (result, "silentTail", rounded ((numSamples - 1 - lastAudibleIndex) / sampleRate));

    if (numChannels >= 2)
    {
        double leftRight = 0.0;
        double leftSquares = 0.0;
        double rightSquares = 0.0;

        for (int index = 0; index < numSamples; ++index)
        {
            const auto left = static_cast<double> (audio.getSample (0, index));
            const auto right = static_cast<double> (audio.getSample (1, index));
            leftRight += left * right;
            leftSquares += left * left;
            rightSquares += right * right;
        }

        const auto norm = std::sqrt (leftSquares * rightSquares);
        set (result, "stereoCorrelation", norm > 0.0 ? rounded (leftRight / norm) : 1.0);
    }

    addSpectrum (result, mixToMono (audio), sampleRate);
    return result;
}
} // namespace RetraceAudioAnalysis
