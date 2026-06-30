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

namespace
{
constexpr float kPeakToAverageCorrectionDb = 3.0103f;
constexpr float kMeterMinimumDecibel = -(90.01f + 20.0f + kPeakToAverageCorrectionDb);
constexpr float kPeakHoldFallRateDbPerSecond = 26.0f / 3.0f;

void applyLogBallistics (float meterInertiaSeconds, float timePassedSeconds, float levelDb, float& readoutDb)
{
    if (meterInertiaSeconds <= 0.0f || timePassedSeconds <= 0.0f || levelDb == readoutDb)
        return;

    const float attackReleaseCoef = std::pow (0.01f, timePassedSeconds / meterInertiaSeconds);
    readoutDb = attackReleaseCoef * (readoutDb - levelDb) + levelDb;
}
} // namespace

//==============================================================================
KMeterState::KMeterState()
{
    prepare (48000.0, 2);
}

KMeterState::KMeterState (double sampleRate, int maxChannels)
{
    prepare (sampleRate, maxChannels);
}

KMeterState::~KMeterState()
{
}

//==============================================================================
void KMeterState::prepare (double newSampleRate, int maxChannels)
{
    jassert (newSampleRate > 0.0);
    jassert (maxChannels > 0 && maxChannels <= 32);

    sampleRate = newSampleRate;
    numChannels = maxChannels;

    // Initialize lock-free FIFO
    fifoSize = static_cast<int> (sampleRate * 2.0); // 2 seconds buffer
    sampleBuffer.resize (fifoSize * numChannels);   // Interleaved
    audioFifo = std::make_unique<AbstractFifo> (fifoSize);

    // Initialize per-channel state
    channels.resize (numChannels);

    // Initialize level processors (one per channel)
    levelProcessors.resize (numChannels);
    for (auto& processor : levelProcessors)
    {
        processor.setSampleRate (sampleRate);
        processor.setIntegrationTime (integrationTime);
        processor.setFallTime (peakFallTime);
    }

    // Initialize loudness filters (one per channel for ITU/EBU K-weighting)
    loudnessFilters.resize (numChannels);
    const int maxBlockSize = 512; // Match processPendingAudio chunk size
    for (auto& filter : loudnessFilters)
        filter.prepare (sampleRate, maxBlockSize);

    // Allocate temporary buffer for K-weighted samples
    filteredBuffer.resize (maxBlockSize);

    reset();
}

void KMeterState::reset() noexcept
{
    if (audioFifo)
        audioFifo->reset();

    std::fill (sampleBuffer.begin(), sampleBuffer.end(), 0.0f);

    const float scaleOffset = scaleOffsetForScale (scale);

    for (auto& channel : channels)
    {
        channel.currentPeak = 0.0f;
        channel.currentAverage = 0.0f;
        channel.currentAverageDb = kMeterMinimumDecibel;
        channel.peakHold = 0.0f;
        channel.peakHoldTimer = 0.0;
        channel.contiguousOverSamples = 0;
        channel.totalOverflows = 0;
    }

    for (auto& processor : levelProcessors)
        processor.reset();

    for (auto& filter : loudnessFilters)
        filter.reset();

    atomicPeakLevelDb.set (kMeterMinimumDecibel + scaleOffset);
    atomicAverageLevelDb.set (kMeterMinimumDecibel + scaleOffset);
    atomicPeakHoldLevelDb.set (kMeterMinimumDecibel + scaleOffset);
    atomicOverCount.set (0);
    atomicClipping.set (false);

    atomicIntegratedLoudness.set (-70.0f);
    atomicShortTermLoudness.set (-70.0f);
    atomicMomentaryLoudness.set (-70.0f);
    atomicLoudnessRange.set (0.0f);
}

//==============================================================================
void KMeterState::pushSamples (const float* const* channelData, int numChannelsToPush, int numSamples) noexcept
{
    jassert (channelData != nullptr);
    jassert (numChannelsToPush > 0 && numChannelsToPush <= numChannels);
    jassert (numSamples >= 0);

    if (! audioFifo || numSamples == 0)
        return;

    // Get write scope from FIFO
    const auto scope = audioFifo->write (numSamples);

    // Interleave samples into FIFO buffer
    for (int block = 0; block < 2; ++block)
    {
        const int startIndex = (block == 0) ? scope.startIndex1 : scope.startIndex2;
        const int blockSize = (block == 0) ? scope.blockSize1 : scope.blockSize2;

        if (blockSize == 0)
            continue;

        for (int i = 0; i < blockSize; ++i)
        {
            const int bufferIndex = (startIndex + i) * numChannels;

            for (int ch = 0; ch < numChannelsToPush; ++ch)
                sampleBuffer[bufferIndex + ch] = channelData[ch][i + (block == 0 ? 0 : scope.blockSize1)];

            // Zero remaining channels if pushing fewer channels than allocated
            for (int ch = numChannelsToPush; ch < numChannels; ++ch)
                sampleBuffer[bufferIndex + ch] = 0.0f;
        }
    }
}

void KMeterState::pushMonoSamples (const float* samples, int numSamples) noexcept
{
    const float* channelData[1] = { samples };
    pushSamples (channelData, 1, numSamples);
}

int KMeterState::getNumSamplesInFifo() const noexcept
{
    if (! audioFifo)
        return 0;

    return audioFifo->getNumReady();
}

//==============================================================================
void KMeterState::processPendingAudio() noexcept
{
    if (! audioFifo)
        return;

    const int numAvailable = audioFifo->getNumReady();
    if (numAvailable == 0)
        return;

    // Process in chunks for efficiency
    constexpr int maxChunkSize = 512;
    const int numToProcess = jmin (numAvailable, maxChunkSize);

    const auto scope = audioFifo->read (numToProcess);

    // Temporary buffers for de-interleaving
    std::vector<float> channelBuffers[32]; // Max 32 channels
    for (int ch = 0; ch < numChannels; ++ch)
        channelBuffers[ch].resize (numToProcess);

    // De-interleave samples from FIFO
    for (int block = 0; block < 2; ++block)
    {
        const int startIndex = (block == 0) ? scope.startIndex1 : scope.startIndex2;
        const int blockSize = (block == 0) ? scope.blockSize1 : scope.blockSize2;

        if (blockSize == 0)
            continue;

        const int destOffset = (block == 0) ? 0 : scope.blockSize1;

        for (int i = 0; i < blockSize; ++i)
        {
            const int bufferIndex = (startIndex + i) * numChannels;

            for (int ch = 0; ch < numChannels; ++ch)
                channelBuffers[ch][destOffset + i] = sampleBuffer[bufferIndex + ch];
        }
    }

    // Process each channel
    for (int ch = 0; ch < numChannels; ++ch)
        processChannelLevels (ch, channelBuffers[ch].data(), numToProcess);

    // Update global atomic values (maximum across channels)
    float maxPeak = kMeterMinimumDecibel;
    float maxAverage = kMeterMinimumDecibel;
    float maxPeakHold = kMeterMinimumDecibel;
    int totalOverCount = 0;
    int maxContiguousOverCount = 0;
    bool anyClipping = false;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto& channelState = channels[ch];

        // Convert to dB
        const float peakDb = channelState.currentPeak > 0.0f
                               ? Decibels::gainToDecibels (channelState.currentPeak)
                               : kMeterMinimumDecibel;
        const float averageDb = channelState.currentAverageDb;
        const float peakHoldDb = channelState.peakHold > 0.0f
                                   ? Decibels::gainToDecibels (channelState.peakHold)
                                   : kMeterMinimumDecibel;

        maxPeak = jmax (maxPeak, peakDb);
        maxAverage = jmax (maxAverage, averageDb);
        maxPeakHold = jmax (maxPeakHold, peakHoldDb);
        maxContiguousOverCount = jmax (maxContiguousOverCount, channelState.contiguousOverSamples);
        totalOverCount += channelState.totalOverflows;
    }

    // Write to atomic variables - all levels are calibrated by scale offset
    const float scaleOffset = scaleOffsetForScale (scale);
    const float calibratedPeak = maxPeak + scaleOffset;
    const float calibratedAverage = maxAverage + scaleOffset;
    const float calibratedPeakHold = maxPeakHold + scaleOffset;

    atomicPeakLevelDb.set (calibratedPeak);
    atomicAverageLevelDb.set (calibratedAverage);
    atomicPeakHoldLevelDb.set (calibratedPeakHold);
    const int overCountToReport = overCounterMode == OverCounterMode::contiguous
                                    ? maxContiguousOverCount
                                    : totalOverCount;

    atomicOverCount.set (overCountToReport);
    atomicClipping.set (overCountToReport > 0);
}

void KMeterState::processChannelLevels (int channel, const float* samples, int numSamples)
{
    jassert (channel >= 0 && channel < numChannels);

    auto& channelState = channels[channel];
    auto& processor = levelProcessors[channel];

    // Determine if we need K-weighting (ITU BS.1770-4 or EBU R128)
    const bool needsKWeighting = meteringStandard == MeteringStandard::ituBS1770_4
                              || meteringStandard == MeteringStandard::ebuR128;

    // Apply K-weighting filter for ITU/EBU modes
    const float* samplesToProcess = samples;
    if (needsKWeighting)
    {
        jassert (numSamples <= static_cast<int> (filteredBuffer.size()));
        std::copy (samples, samples + numSamples, filteredBuffer.begin());
        loudnessFilters[channel].processBlock (filteredBuffer.data(), numSamples);
        samplesToProcess = filteredBuffer.data();
    }

    // Process peak (always from original samples, never filtered)
    float peak = 0.0f;
    processor.processPeak (samples, numSamples, peak);

    // Update peak with fall
    const double timeDelta = numSamples / sampleRate;
    processor.processPeakWithFall (peak, timeDelta, channelState.currentPeak);

    // Process RMS (from K-weighted samples if ITU/EBU, otherwise from original)
    float rms = 0.0f;
    processor.processRMS (samplesToProcess, numSamples, rms);
    channelState.currentAverage = rms;

    // CRITICAL: Peak must never fall below RMS average (physically impossible)
    if (channelState.currentPeak < rms)
        channelState.currentPeak = rms;

    float averageDb = rms > 0.0f ? Decibels::gainToDecibels (rms) : kMeterMinimumDecibel;

    if (meteringStandard == MeteringStandard::rmsFlat)
        averageDb += kPeakToAverageCorrectionDb;

    if (averageDb < kMeterMinimumDecibel)
        averageDb = kMeterMinimumDecibel;

    applyLogBallistics (static_cast<float> (averageFallTime), static_cast<float> (timeDelta), averageDb, channelState.currentAverageDb);

    if (channelState.currentAverageDb < kMeterMinimumDecibel)
        channelState.currentAverageDb = kMeterMinimumDecibel;

    channelState.currentAverage = Decibels::decibelsToGain (channelState.currentAverageDb);

    // Update peak hold (tracks the ballistic peak, not instant peak)
    const float peakHoldCandidate = jmin (channelState.currentPeak, 1.0f);
    if (peakHoldCandidate > channelState.peakHold)
    {
        channelState.peakHold = peakHoldCandidate;
        channelState.peakHoldTimer = 0.0;
    }
    else if (peakHoldTime >= 0.0)
    {
        // Update hold timer
        channelState.peakHoldTimer += timeDelta;

        // After hold time (10 seconds), apply linear fall
        if (channelState.peakHoldTimer >= peakHoldTime)
        {
            // Apply linear fall in dB space: 26 dB in 3 seconds (same as peak fall)
            const float fallAmountDb = kPeakHoldFallRateDbPerSecond * static_cast<float> (timeDelta);

            // Convert to dB, apply fall, convert back
            const float peakHoldDb = channelState.peakHold > 0.0f
                                       ? Decibels::gainToDecibels (channelState.peakHold)
                                       : kMeterMinimumDecibel;
            const float newPeakHoldDb = peakHoldDb - fallAmountDb;
            channelState.peakHold = Decibels::decibelsToGain (newPeakHoldDb);

            // Never fall below current peak
            if (peakHoldCandidate > channelState.peakHold)
                channelState.peakHold = peakHoldCandidate;
        }
    }

    // Update OVER counter (counts contiguous or total samples at or above threshold)
    // IMPORTANT: Always use ORIGINAL samples for clipping detection, never filtered
    int overflowsInBlock = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        if (std::abs (samples[i]) >= overThreshold)
        {
            ++overflowsInBlock;
            ++channelState.contiguousOverSamples;
        }
        else
        {
            channelState.contiguousOverSamples = 0;
        }
    }

    channelState.totalOverflows += overflowsInBlock;
}

//==============================================================================
void KMeterState::setMeteringStandard (MeteringStandard standard)
{
    if (meteringStandard != standard)
    {
        meteringStandard = standard;
        // Reset filters when switching metering standards
        for (auto& filter : loudnessFilters)
            filter.reset();
    }
}

void KMeterState::setScale (Scale newScale)
{
    scale = newScale;
}

void KMeterState::setIntegrationTime (double seconds)
{
    if (seconds > 0.0 && seconds != integrationTime)
    {
        integrationTime = seconds;
        for (auto& processor : levelProcessors)
            processor.setIntegrationTime (seconds);
    }
}

void KMeterState::setPeakFallTime (double seconds)
{
    if (seconds > 0.0 && seconds != peakFallTime)
    {
        peakFallTime = seconds;
        for (auto& processor : levelProcessors)
            processor.setFallTime (seconds);
    }
}

void KMeterState::setAverageFallTime (double seconds)
{
    if (seconds > 0.0)
        averageFallTime = seconds;
}

void KMeterState::setPeakHoldTime (double seconds)
{
    peakHoldTime = seconds;
}

void KMeterState::setOverThreshold (float threshold)
{
    overThreshold = jlimit (0.0f, 1.0f, threshold);
}

void KMeterState::setOverCounterMode (OverCounterMode mode)
{
    overCounterMode = mode;
}

//==============================================================================
float KMeterState::getPeakLevel (int channel) const noexcept
{
    if (channel >= 0 && channel < numChannels)
    {
        const float peak = channels[channel].currentPeak;
        const float peakDb = peak > 0.0f ? Decibels::gainToDecibels (peak) : kMeterMinimumDecibel;
        return peakDb + scaleOffsetForScale (scale);
    }

    return atomicPeakLevelDb.get();
}

float KMeterState::getAverageLevel (int channel) const noexcept
{
    if (channel >= 0 && channel < numChannels)
    {
        const float averageDb = channels[channel].currentAverageDb;
        const float scaleOffset = scaleOffsetForScale (scale);
        return averageDb + scaleOffset;
    }

    return atomicAverageLevelDb.get();
}

float KMeterState::getPeakHoldLevel (int channel) const noexcept
{
    if (channel >= 0 && channel < numChannels)
    {
        const float hold = channels[channel].peakHold;
        const float holdDb = hold > 0.0f ? Decibels::gainToDecibels (hold) : kMeterMinimumDecibel;
        return holdDb + scaleOffsetForScale (scale);
    }

    return atomicPeakHoldLevelDb.get();
}

int KMeterState::getOverCount() const noexcept
{
    return atomicOverCount.get();
}

bool KMeterState::isClipping() const noexcept
{
    return atomicClipping.get();
}

//==============================================================================
float KMeterState::getIntegratedLoudness() const noexcept
{
    return atomicIntegratedLoudness.get();
}

float KMeterState::getShortTermLoudness() const noexcept
{
    return atomicShortTermLoudness.get();
}

float KMeterState::getMomentaryLoudness() const noexcept
{
    return atomicMomentaryLoudness.get();
}

float KMeterState::getLoudnessRange() const noexcept
{
    return atomicLoudnessRange.get();
}

//==============================================================================
float KMeterState::scaleOffsetForScale (Scale scale) noexcept
{
    switch (scale)
    {
        case Scale::k20:
            return 20.0f; // 0dB meter = -20dBFS
        case Scale::k14:
            return 14.0f; // 0dB meter = -14dBFS
        case Scale::k12:
            return 12.0f; // 0dB meter = -12dBFS
        default:
            return 20.0f;
    }
}

float KMeterState::rangeMinForScale (Scale scale) noexcept
{
    switch (scale)
    {
        case Scale::k20:
            return -70.0f;
        case Scale::k14:
            return -64.0f;
        case Scale::k12:
            return -62.0f;
        default:
            return -70.0f;
    }
}

float KMeterState::rangeMaxForScale (Scale scale) noexcept
{
    switch (scale)
    {
        case Scale::k20:
            return 20.0f;
        case Scale::k14:
            return 26.0f;
        case Scale::k12:
            return 28.0f;
        default:
            return 20.0f;
    }
}

} // namespace yup
