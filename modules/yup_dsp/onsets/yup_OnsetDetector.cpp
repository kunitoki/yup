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

namespace yup
{

void OnsetDetector::prepare (const Parameters& p, float sr)
{
    jassert (sr > 0.0f);
    jassert (p.spectrogram.fftSize >= 64 && (p.spectrogram.fftSize & (p.spectrogram.fftSize - 1)) == 0);
    jassert (p.spectrogram.fps > 0);
    jassert (p.spectrogram.logMul > 0.0f);
    jassert (p.spectrogram.logAdd > 0.0f);
    jassert (p.bandsPerOctave > 0);
    jassert (p.fMin > 0.0f);
    jassert (p.fMax > p.fMin);
    jassert (p.superFluxODF.maxFilterBins > 0);
    jassert (p.peakPicker.threshold >= 0.0f);
    jassert (p.peakPicker.combineSec >= 0.0f);
    jassert (p.peakPicker.preAvgSec >= 0.0f && p.peakPicker.preMaxSec >= 0.0f);
    jassert (p.peakPicker.postAvgSec >= 0.0f && p.peakPicker.postMaxSec >= 0.0f);

    params = p;
    sampleRate = sr;

    // --- Filter bank ---
    Spectrogram::Parameters specParams = p.spectrogram;
    specParams.filterBank = nullptr;

    if (p.useFilterBank)
    {
        filterBank.build (p.bandsPerOctave, p.fMin, p.fMax, p.spectrogram.fftSize / 2, sampleRate, p.equalizeFilterArea);

        specParams.filterBank = &filterBank;
    }

    // --- ODF ---
    specParams.computeLGD = p.useComplexFlux;
    spectrogram.prepare (specParams, sampleRate);

    if (p.useComplexFlux)
    {
        auto cf = std::make_unique<ComplexFluxODF>();
        cf->prepare (p.complexFluxODF, spectrogram.getWindowData(), spectrogram.getFFTSize(), spectrogram.getHopSize());
        odf = std::move (cf);
    }
    else
    {
        auto sf = std::make_unique<SuperFluxODF>();
        sf->prepare (p.superFluxODF, spectrogram.getWindowData(), spectrogram.getFFTSize(), spectrogram.getHopSize());
        odf = std::move (sf);
    }

    // --- Peak picker ---
    peakPicker.prepare (p.peakPicker, static_cast<float> (p.spectrogram.fps));
}

void OnsetDetector::processOffline (const AudioBuffer<float>& buffer)
{
    jassert (buffer.getNumSamples() > 0);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = jmin (buffer.getNumChannels(), 2);

    std::vector<float> mono (static_cast<std::size_t> (numSamples));

    if (numChannels == 2)
    {
        const float* left = buffer.getReadPointer (0);
        const float* right = buffer.getReadPointer (1);

        for (int i = 0; i < numSamples; ++i)
            mono[static_cast<std::size_t> (i)] = (left[i] + right[i]) * 0.5f;
    }
    else
    {
        const float* src = buffer.getReadPointer (0);

        for (int i = 0; i < numSamples; ++i)
            mono[static_cast<std::size_t> (i)] = src[i];
    }

    processOffline (mono.data(), numSamples);
}

void OnsetDetector::processOffline (const float* samples, int numSamples)
{
    jassert (samples != nullptr);
    jassert (numSamples > 0);

    spectrogram.processOffline (samples, numSamples);

    if (odf != nullptr)
    {
        odf->compute (spectrogram);

        const auto& act = odf->getActivations();

        if (! act.empty())
        {
            peakPicker.detect (act.data(), static_cast<int> (act.size()));

            if (params.refineOnsets)
                peakPicker.refineOnsetTimes (samples, numSamples, sampleRate, params.refineMaxSec, params.refineThreshold);
        }
    }
}

void OnsetDetector::reset()
{
    spectrogram.reset();
    peakPicker.reset();

    if (odf != nullptr)
        odf->reset();
}

//==============================================================================
Result OnsetDetector::prepareStreaming (const Parameters& p, float sr)
{
    if (p.useComplexFlux)
        return Result::fail ("Streaming onset detection supports the SuperFlux ODF only");

    Parameters streamingParams = p;
    streamingParams.peakPicker.onlineMode = true;
    streamingParams.refineOnsets = false;

    prepare (streamingParams, sr);

    jassert (odf != nullptr && odf->supportsStreaming());

    spectrogram.prepareStreaming();
    odf->prepareStreaming (spectrogram.getNumBins());
    peakPicker.prepareStreaming();

    resetStreaming();

    return Result::ok();
}

int OnsetDetector::processStreaming (const float* monoSamples, int numSamples) noexcept
{
    jassert (monoSamples != nullptr);

    int onsetsDetected = 0;
    int samplesConsumed = 0;

    while (samplesConsumed < numSamples)
    {
        // Feed at most one hop at a time so pending frames are consumed as
        // they appear and the spectrogram's pending ring can never overflow.
        const int chunk = jmin (numSamples - samplesConsumed, spectrogram.getHopSize());

        spectrogram.processStreaming (monoSamples + samplesConsumed, chunk);
        samplesConsumed += chunk;

        const int pending = spectrogram.getNumPendingStreamFrames();

        for (int index = 0; index < pending; ++index)
        {
            const float activation = odf->computeStreamingFrame (spectrogram.getPendingStreamFrame (index),
                                                                 spectrogram.getNumBins());

            double onsetTime = 0.0;

            if (peakPicker.detectStreamingFrame (activation, onsetTime))
            {
                const auto write = onsetQueueWrite.load (std::memory_order_relaxed);
                const auto read = onsetQueueRead.load (std::memory_order_acquire);

                if (write - read < static_cast<uint64> (onsetQueueSize))
                {
                    onsetQueue[static_cast<std::size_t> (write % onsetQueueSize)] = onsetTime;
                    onsetQueueWrite.store (write + 1, std::memory_order_release);
                    ++onsetsDetected;
                }
            }
        }

        spectrogram.consumePendingStreamFrames (pending);
    }

    return onsetsDetected;
}

int OnsetDetector::drainStreamingOnsets (double* outTimesSeconds, int maxOnsets) noexcept
{
    jassert (outTimesSeconds != nullptr);

    auto read = onsetQueueRead.load (std::memory_order_relaxed);
    const auto write = onsetQueueWrite.load (std::memory_order_acquire);

    int count = 0;

    while (read < write && count < maxOnsets)
    {
        outTimesSeconds[count++] = onsetQueue[static_cast<std::size_t> (read % onsetQueueSize)];
        ++read;
    }

    onsetQueueRead.store (read, std::memory_order_release);

    return count;
}

void OnsetDetector::resetStreaming() noexcept
{
    spectrogram.resetStreaming();
    peakPicker.resetStreaming();

    if (odf != nullptr)
        odf->resetStreaming();

    onsetQueueRead.store (onsetQueueWrite.load (std::memory_order_relaxed), std::memory_order_relaxed);
}

double OnsetDetector::getStreamingLatencySeconds() const noexcept
{
    return sampleRate > 0.0f
             ? static_cast<double> (spectrogram.getFFTSize()) * 0.5 / static_cast<double> (sampleRate)
             : 0.0;
}

const std::vector<float>& OnsetDetector::getActivationFunction() const noexcept
{
    static const std::vector<float> empty;

    if (odf != nullptr)
        return odf->getActivations();

    return empty;
}

const std::vector<double>& OnsetDetector::getOnsetTimes() const noexcept
{
    return peakPicker.getOnsetTimes();
}

int OnsetDetector::getNumFrames() const noexcept
{
    return spectrogram.getNumFrames();
}

} // namespace yup
