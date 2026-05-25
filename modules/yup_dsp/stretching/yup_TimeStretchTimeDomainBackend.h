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

namespace yup
{

//==============================================================================
static constexpr int defaultSequenceLengthMs = 82;
static constexpr int defaultSeekWindowLengthMs = 14;
static constexpr int defaultOverlapLengthMs = 12;
static constexpr float maximumPreallocatedTempo = 4.0f;
static constexpr double minimumCorrelationValue = std::numeric_limits<double>::lowest();

static constexpr int quickScanOffsets[4][24] = {
    { 124, 186, 248, 310, 372, 434, 496, 558, 620, 682, 744, 806, 868, 930, 992, 1054, 1116, 1178, 1240, 1302, 1364, 1426, 1488, 0 },
    { -100, -75, -50, -25, 25, 50, 75, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { -20, -15, -10, -5, 5, 10, 15, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { -4, -3, -2, -1, 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

class MultiChannelSampleFifo
{
public:
    void prepare (int numChannels, int capacityPerChannel)
    {
        channels.resize (static_cast<size_t> (numChannels));
        for (auto& channel : channels)
        {
            channel.assign (static_cast<size_t> (capacityPerChannel), 0.0f);
        }

        readPosition = 0;
        numSamples = 0;
    }

    void clear()
    {
        readPosition = 0;
        numSamples = 0;
    }

    int getNumChannels() const noexcept
    {
        return static_cast<int> (channels.size());
    }

    int getNumSamples() const noexcept
    {
        return numSamples;
    }

    const float* getReadPointer (int channel, int offset = 0) const noexcept
    {
        return channels[static_cast<size_t> (channel)].data() + readPosition + offset;
    }

    float* getReadPointer (int channel, int offset = 0) noexcept
    {
        return channels[static_cast<size_t> (channel)].data() + readPosition + offset;
    }

    float* getWritePointer (int channel, int samplesToWrite)
    {
        ensureWritable (samplesToWrite);
        return channels[static_cast<size_t> (channel)].data() + readPosition + numSamples;
    }

    void ensureWritable (int samplesToWrite)
    {
        const auto requiredSize = static_cast<size_t> (readPosition + numSamples + samplesToWrite);
        if (channels.empty() || requiredSize <= channels.front().size())
            return;

        const auto newSize = std::max (requiredSize, channels.front().size() * 2);
        for (auto& channel : channels)
            channel.resize (newSize, 0.0f);
    }

    void advanceWritePosition (int samplesWritten) noexcept
    {
        numSamples += samplesWritten;
    }

    void advanceReadPosition (int samplesRead)
    {
        samplesRead = jlimit (0, numSamples, samplesRead);
        readPosition += samplesRead;
        numSamples -= samplesRead;

        if (numSamples == 0)
        {
            clear();
            return;
        }

        if (readPosition > 4096 && readPosition > static_cast<int> (channels.front().size() / 2))
            compact();
    }

private:
    void compact()
    {
        for (auto& channel : channels)
        {
            std::move (channel.begin() + readPosition,
                       channel.begin() + readPosition + numSamples,
                       channel.begin());
        }

        readPosition = 0;
    }

    std::vector<std::vector<float>> channels;
    int readPosition = 0;
    int numSamples = 0;
};

class TimeDomainTimeStretchBackend : public TimeStretchProcessor::Engine
{
public:
    Result prepare (const TimeStretchProcessor::ProcessSpec& specToUse) override
    {
        spec = specToUse;
        channelCount = spec.numChannels;
        inputStartPosition = 0;
        pendingInputPosition = 0;

        setTimeConstants (static_cast<int> (std::round (spec.inputSampleRate)),
                          defaultSequenceLengthMs,
                          defaultSeekWindowLengthMs,
                          defaultOverlapLengthMs);

        midBuffers.assign (static_cast<size_t> (channelCount), std::vector<float> (static_cast<size_t> (overlapLength)));
        referenceMidBuffers.assign (static_cast<size_t> (channelCount), std::vector<float> (static_cast<size_t> (overlapLength)));
        providerPointers.resize (static_cast<size_t> (channelCount));

        maximumSamplesPerRequest = calculateSamplesPerRequest (maximumPreallocatedTempo);
        const int fifoCapacity = maximumSamplesPerRequest + (spec.maximumBlockSize * 4) + (sequenceWindowLength * 4);
        inputBuffer.prepare (channelCount, fifoCapacity);
        outputBuffer.prepare (channelCount, fifoCapacity);
        preparePitchShifter (fifoCapacity);

        reset();
        return Result::ok();
    }

    void reset() override
    {
        inputBuffer.clear();
        outputBuffer.clear();
        pitchOutputBuffer.clear();
        pitchResampler.reset();

        for (auto& channel : midBuffers)
            std::fill (channel.begin(), channel.end(), 0.0f);

        midBufferDirty = false;
        skipFraction = 0.0f;
        inputStartPosition = pendingInputPosition;
    }

    void setInputPosition (int64 newInputPosition) override
    {
        pendingInputPosition = newInputPosition;
        reset();
    }

    void setParameters (const TimeStretchProcessor::Parameters& newParameters) override
    {
        const auto previousPitchRatio = getPitchRatio();

        parameters = newParameters;
        updateTempo();

        if (std::abs (previousPitchRatio - getPitchRatio()) > 0.000001)
            resetPitchShifter();
    }

    void setInputProvider (TimeStretchProcessor::InputProvider provider) override
    {
        inputProvider = std::move (provider);
    }

    int getMaxInputFrameCount() const override
    {
        return jmax (maximumSamplesPerRequest, jmax (samplesPerRequest, spec.maximumBlockSize + overlapLength));
    }

    int process (const float* const* inputChannels,
                 int inputFrameCount,
                 float* const* outputChannels,
                 int outputFrameCount) override
    {
        if (outputChannels == nullptr || outputFrameCount <= 0)
            return 0;

        if (inputProvider == nullptr && inputChannels != nullptr && inputFrameCount > 0)
            appendDirectInput (inputChannels, inputFrameCount);

        if (isPitchShiftEnabled())
            return processPitchShifted (outputChannels, outputFrameCount);

        return renderTimeDomainOutput (outputChannels, outputFrameCount);
    }

    String getBackendName() const override
    {
        return "Time Domain";
    }

    double getLatencyInFrames() const override
    {
        return jmax (0.0, static_cast<double> (inputBuffer.getNumSamples() - outputBuffer.getNumSamples()));
    }

private:
    int renderTimeDomainOutput (float* const* outputChannels, int outputFrameCount)
    {
        const int framesToCalculate = outputFrameCount + overlapLength;
        while (outputBuffer.getNumSamples() < framesToCalculate)
        {
            if (isUnityTempo())
                processUnity (framesToCalculate - outputBuffer.getNumSamples());
            else
                processStretchedSequence();
        }

        for (int channel = 0; channel < channelCount; ++channel)
            std::copy (outputBuffer.getReadPointer (channel),
                       outputBuffer.getReadPointer (channel) + outputFrameCount,
                       outputChannels[channel]);

        outputBuffer.advanceReadPosition (outputFrameCount);
        return outputFrameCount;
    }

    void setTimeConstants (int sampleRate, int sequenceMs, int seekWindowMs, int overlapMs)
    {
        seekLength = jmax (1, (sampleRate * seekWindowMs) / 1000);
        sequenceWindowLength = jmax (32, (sampleRate * sequenceMs) / 1000);
        overlapLength = jmax (16, (sampleRate * overlapMs) / 1000);

        if (sequenceWindowLength <= overlapLength * 2)
            sequenceWindowLength = overlapLength * 2 + 1;

        updateTempo();
    }

    void updateTempo()
    {
        const auto timeRatio = getEffectiveTimeRatio();
        const auto requestedTempo = static_cast<float> (1.0 / timeRatio);
        jassert (requestedTempo <= maximumPreallocatedTempo);
        tempo = jmin (requestedTempo, maximumPreallocatedTempo);
        nominalSkip = tempo * static_cast<float> (sequenceWindowLength - overlapLength);
        skipFraction = 0.0f;

        samplesPerRequest = calculateSamplesPerRequest (tempo);
    }

    int calculateSamplesPerRequest (float tempoToUse) const
    {
        const auto skip = tempoToUse * static_cast<float> (sequenceWindowLength - overlapLength);
        const int integerSkip = static_cast<int> (skip + 0.5f);
        return jmax (integerSkip + overlapLength, sequenceWindowLength) + seekLength;
    }

    bool isUnityTempo() const noexcept
    {
        return std::abs (static_cast<double> (tempo) - 1.0) <= 0.000001;
    }

    double getTimeRatio() const noexcept
    {
        return parameters.timeRatio > 0.0 ? parameters.timeRatio : 1.0;
    }

    double getPitchRatio() const noexcept
    {
        return parameters.pitchRatio > 0.0 ? parameters.pitchRatio : 1.0;
    }

    double getEffectiveTimeRatio() const noexcept
    {
        return getTimeRatio() * getPitchRatio();
    }

    bool isPitchShiftEnabled() const noexcept
    {
        return std::abs (getPitchRatio() - 1.0) > 0.000001;
    }

    void preparePitchShifter (int fifoCapacity)
    {
        pitchOutputBuffer.prepare (channelCount, fifoCapacity);

        pitchStretchedBuffers.resize (static_cast<size_t> (channelCount));
        pitchResampledBuffers.resize (static_cast<size_t> (channelCount));
        pitchStretchedReadPointers.resize (static_cast<size_t> (channelCount));
        pitchStretchedWritePointers.resize (static_cast<size_t> (channelCount));
        pitchResampledWritePointers.resize (static_cast<size_t> (channelCount));

        ensurePitchProcessingCapacity (jmax (1, spec.maximumBlockSize * 4));
        resetPitchShifter();
    }

    void resetPitchShifter()
    {
        if (channelCount <= 0 || spec.outputSampleRate <= 0.0)
            return;

        pitchOutputBuffer.clear();
        pitchResampler.prepare (spec.outputSampleRate * getPitchRatio(),
                                spec.outputSampleRate,
                                channelCount,
                                pitchResamplerInputCapacity);
        pitchResampler.reset();
    }

    int processPitchShifted (float* const* outputChannels, int outputFrameCount)
    {
        while (pitchOutputBuffer.getNumSamples() < outputFrameCount)
        {
            const int missingFrames = outputFrameCount - pitchOutputBuffer.getNumSamples();
            const int stretchedFramesNeeded = calculatePitchInputFrameCount (missingFrames);

            ensurePitchProcessingCapacity (stretchedFramesNeeded);
            preparePitchPointers();

            const int stretchedFrames = renderTimeDomainOutput (pitchStretchedWritePointers.data(), stretchedFramesNeeded);
            if (stretchedFrames <= 0)
                break;

            const int resampledFrames = pitchResampler.resample (pitchStretchedReadPointers.data(),
                                                                 pitchResampledWritePointers.data(),
                                                                 channelCount,
                                                                 stretchedFrames);
            if (resampledFrames <= 0)
                continue;

            appendPitchOutput (resampledFrames);
        }

        const int framesToCopy = jmin (outputFrameCount, pitchOutputBuffer.getNumSamples());
        for (int channel = 0; channel < channelCount; ++channel)
            std::copy (pitchOutputBuffer.getReadPointer (channel),
                       pitchOutputBuffer.getReadPointer (channel) + framesToCopy,
                       outputChannels[channel]);

        pitchOutputBuffer.advanceReadPosition (framesToCopy);
        return framesToCopy;
    }

    int calculatePitchInputFrameCount (int outputFramesNeeded) const
    {
        return jmax (1, static_cast<int> (std::ceil (static_cast<double> (outputFramesNeeded) * getPitchRatio())) + 2);
    }

    int calculatePitchOutputCapacity (int inputFrameCount) const
    {
        return jmax (1, static_cast<int> (std::ceil (static_cast<double> (inputFrameCount) / getPitchRatio())) + 2);
    }

    void ensurePitchProcessingCapacity (int inputFrameCount)
    {
        if (inputFrameCount > pitchResamplerInputCapacity)
        {
            pitchResamplerInputCapacity = inputFrameCount;
            resetPitchShifter();
        }

        const int outputFrameCapacity = calculatePitchOutputCapacity (inputFrameCount);
        for (int channel = 0; channel < channelCount; ++channel)
        {
            pitchStretchedBuffers[static_cast<size_t> (channel)].resize (static_cast<size_t> (inputFrameCount));
            pitchResampledBuffers[static_cast<size_t> (channel)].resize (static_cast<size_t> (outputFrameCapacity));
        }
    }

    void preparePitchPointers()
    {
        for (int channel = 0; channel < channelCount; ++channel)
        {
            auto& stretchedBuffer = pitchStretchedBuffers[static_cast<size_t> (channel)];
            auto& resampledBuffer = pitchResampledBuffers[static_cast<size_t> (channel)];

            pitchStretchedReadPointers[static_cast<size_t> (channel)] = stretchedBuffer.data();
            pitchStretchedWritePointers[static_cast<size_t> (channel)] = stretchedBuffer.data();
            pitchResampledWritePointers[static_cast<size_t> (channel)] = resampledBuffer.data();
        }
    }

    void appendPitchOutput (int frameCount)
    {
        for (int channel = 0; channel < channelCount; ++channel)
        {
            auto* dest = pitchOutputBuffer.getWritePointer (channel, frameCount);
            const auto& source = pitchResampledBuffers[static_cast<size_t> (channel)];
            std::copy (source.begin(), source.begin() + frameCount, dest);
        }

        pitchOutputBuffer.advanceWritePosition (frameCount);
    }

    void appendDirectInput (const float* const* inputChannels, int inputFrameCount)
    {
        for (int channel = 0; channel < channelCount; ++channel)
        {
            auto* dest = inputBuffer.getWritePointer (channel, inputFrameCount);
            if (inputChannels[channel] != nullptr)
                std::copy (inputChannels[channel], inputChannels[channel] + inputFrameCount, dest);
            else
                std::fill (dest, dest + inputFrameCount, 0.0f);
        }

        inputBuffer.advanceWritePosition (inputFrameCount);
    }

    void appendInputFromProvider (int numFrames)
    {
        if (numFrames <= 0)
            return;

        inputBuffer.ensureWritable (numFrames);

        for (int channel = 0; channel < channelCount; ++channel)
        {
            providerPointers[static_cast<size_t> (channel)] = inputBuffer.getWritePointer (channel, numFrames);
            std::fill (providerPointers[static_cast<size_t> (channel)],
                       providerPointers[static_cast<size_t> (channel)] + numFrames,
                       0.0f);
        }

        int muteHead = 0;
        int muteTail = 0;

        if (inputProvider != nullptr)
        {
            inputProvider (inputStartPosition + inputBuffer.getNumSamples(),
                           numFrames,
                           providerPointers.data(),
                           numFrames,
                           muteHead,
                           muteTail);
        }

        muteHead = jlimit (0, numFrames, muteHead);
        muteTail = jlimit (0, numFrames - muteHead, muteTail);

        for (int channel = 0; channel < channelCount; ++channel)
        {
            auto* dest = providerPointers[static_cast<size_t> (channel)];
            if (muteHead > 0)
                std::fill (dest, dest + muteHead, 0.0f);

            if (muteTail > 0)
                std::fill (dest + numFrames - muteTail, dest + numFrames, 0.0f);
        }

        inputBuffer.advanceWritePosition (numFrames);
    }

    void ensureInputFrames (int minimumFrames)
    {
        if (inputBuffer.getNumSamples() >= minimumFrames)
            return;

        const int framesToRead = minimumFrames - inputBuffer.getNumSamples();
        if (inputProvider != nullptr)
        {
            appendInputFromProvider (framesToRead);
            return;
        }

        for (int channel = 0; channel < channelCount; ++channel)
        {
            auto* dest = inputBuffer.getWritePointer (channel, framesToRead);
            std::fill (dest, dest + framesToRead, 0.0f);
        }

        inputBuffer.advanceWritePosition (framesToRead);
    }

    void processUnity (int framesNeeded)
    {
        if (midBufferDirty)
        {
            ensureInputFrames (overlapLength);
            writeOverlap (0);
            inputBuffer.advanceReadPosition (overlapLength);
            inputStartPosition += overlapLength;
            midBufferDirty = false;
            return;
        }

        const int framesToCopy = jmax (1, framesNeeded);
        ensureInputFrames (framesToCopy);

        for (int channel = 0; channel < channelCount; ++channel)
        {
            std::copy (inputBuffer.getReadPointer (channel),
                       inputBuffer.getReadPointer (channel) + framesToCopy,
                       outputBuffer.getWritePointer (channel, framesToCopy));
        }

        inputBuffer.advanceReadPosition (framesToCopy);
        outputBuffer.advanceWritePosition (framesToCopy);
        inputStartPosition += framesToCopy;
    }

    void processStretchedSequence()
    {
        ensureInputFrames (samplesPerRequest);

        const int offset = midBufferDirty ? seekBestOverlapPosition() : 0;
        writeOverlap (offset);

        const int nonOverlappedSamples = sequenceWindowLength - 2 * overlapLength;
        if (nonOverlappedSamples > 0)
        {
            for (int channel = 0; channel < channelCount; ++channel)
            {
                std::copy (inputBuffer.getReadPointer (channel, offset + overlapLength),
                           inputBuffer.getReadPointer (channel, offset + overlapLength + nonOverlappedSamples),
                           outputBuffer.getWritePointer (channel, nonOverlappedSamples));
            }

            outputBuffer.advanceWritePosition (nonOverlappedSamples);
        }

        for (int channel = 0; channel < channelCount; ++channel)
        {
            std::copy (inputBuffer.getReadPointer (channel, offset + sequenceWindowLength - overlapLength),
                       inputBuffer.getReadPointer (channel, offset + sequenceWindowLength),
                       midBuffers[static_cast<size_t> (channel)].begin());
        }

        midBufferDirty = true;

        skipFraction += nominalSkip;
        const int framesToSkip = static_cast<int> (skipFraction);
        skipFraction -= static_cast<float> (framesToSkip);

        inputBuffer.advanceReadPosition (framesToSkip);
        inputStartPosition += framesToSkip;
    }

    void writeOverlap (int inputOffset)
    {
        const float scale = 1.0f / static_cast<float> (overlapLength);

        for (int channel = 0; channel < channelCount; ++channel)
        {
            const auto* input = inputBuffer.getReadPointer (channel, inputOffset);
            const auto& mid = midBuffers[static_cast<size_t> (channel)];
            auto* output = outputBuffer.getWritePointer (channel, overlapLength);

            for (int i = 0; i < overlapLength; ++i)
            {
                output[i] = (input[i] * static_cast<float> (i)
                             + mid[static_cast<size_t> (i)] * static_cast<float> (overlapLength - i))
                          * scale;
            }
        }

        outputBuffer.advanceWritePosition (overlapLength);
    }

    int seekBestOverlapPosition()
    {
        for (int channel = 0; channel < channelCount; ++channel)
        {
            const auto& mid = midBuffers[static_cast<size_t> (channel)];
            auto& referenceMid = referenceMidBuffers[static_cast<size_t> (channel)];

            for (int i = 0; i < overlapLength; ++i)
            {
                const float slope = static_cast<float> (i * (overlapLength - i));
                referenceMid[static_cast<size_t> (i)] = mid[static_cast<size_t> (i)] * slope;
            }
        }

        double bestCorrelation = minimumCorrelationValue;
        int bestOffset = 0;
        int correlationOffset = 0;

        for (const auto& scanOffsets : quickScanOffsets)
        {
            for (int j = 0; scanOffsets[j] != 0; ++j)
            {
                const int offset = correlationOffset + scanOffsets[j];
                if (offset >= seekLength)
                    break;

                if (offset < 0)
                    continue;

                const double correlation = calculateCrossCorrelation (offset);
                if (correlation > bestCorrelation)
                {
                    bestCorrelation = correlation;
                    bestOffset = offset;
                }
            }

            correlationOffset = bestOffset;
        }

        return bestOffset;
    }

    double calculateCrossCorrelation (int inputOffset) const
    {
        double correlation = 0.0;

        for (int channel = 0; channel < channelCount; ++channel)
        {
            const auto* input = inputBuffer.getReadPointer (channel, inputOffset);
            const auto& referenceMid = referenceMidBuffers[static_cast<size_t> (channel)];

            for (int i = 0; i < overlapLength; ++i)
                correlation += static_cast<double> (input[i]) * static_cast<double> (referenceMid[static_cast<size_t> (i)]);
        }

        return jmax (correlation, minimumCorrelationValue);
    }

    TimeStretchProcessor::ProcessSpec spec;
    TimeStretchProcessor::Parameters parameters;
    TimeStretchProcessor::InputProvider inputProvider;

    int channelCount = 0;
    int samplesPerRequest = 0;
    int maximumSamplesPerRequest = 0;
    int overlapLength = 0;
    int seekLength = 0;
    int sequenceWindowLength = 0;

    float tempo = 1.0f;
    float nominalSkip = 0.0f;
    float skipFraction = 0.0f;
    bool midBufferDirty = false;

    int64 inputStartPosition = 0;
    int64 pendingInputPosition = 0;

    MultiChannelSampleFifo inputBuffer;
    MultiChannelSampleFifo outputBuffer;
    MultiChannelSampleFifo pitchOutputBuffer;
    std::vector<std::vector<float>> midBuffers;
    std::vector<std::vector<float>> referenceMidBuffers;
    std::vector<std::vector<float>> pitchStretchedBuffers;
    std::vector<std::vector<float>> pitchResampledBuffers;
    std::vector<const float*> pitchStretchedReadPointers;
    std::vector<float*> pitchStretchedWritePointers;
    std::vector<float*> pitchResampledWritePointers;
    std::vector<float*> providerPointers;
    ResamplerFloat pitchResampler;
    int pitchResamplerInputCapacity = 1;
};

} // namespace yup
