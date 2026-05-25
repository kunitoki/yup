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
#if YUP_ENABLE_BUNGEE

class BungeeTimeStretchBackend : public TimeStretchProcessor::Engine
{
public:
    Result prepare (const TimeStretchProcessor::ProcessSpec& specToUse) override
    {
        sampleRates.input = static_cast<int> (std::round (specToUse.inputSampleRate));
        sampleRates.output = static_cast<int> (std::round (specToUse.outputSampleRate));
        channelCount = specToUse.numChannels;
        maximumBlockSize = specToUse.maximumBlockSize;

        stretcher = std::make_unique<Bungee::Stretcher<Bungee::Basic>> (sampleRates, channelCount, 0);
        const int maxFrames = stretcher->maxInputFrameCount();
        maxInputFrameCount = maxFrames;

        // Allocate contiguous buffer with strided layout
        // Layout: [ch0_frame0...ch0_frameN][ch1_frame0...ch1_frameN]...
        inputChunkBuffer.resize (static_cast<size_t> (channelCount * maxFrames));
        channelPtrs.resize (static_cast<size_t> (channelCount));

        resetState (0);
        return Result::ok();
    }

    void reset() override
    {
        if (stretcher == nullptr || channelCount <= 0 || maximumBlockSize <= 0)
            return;

        resetState (pendingInputPosition);
    }

    void setInputPosition (int64 newInputPosition) override
    {
        pendingInputPosition = newInputPosition;
        seekPending = true;
    }

    void setParameters (const TimeStretchProcessor::Parameters& newParameters) override
    {
        parameters = newParameters;
    }

    void setInputProvider (TimeStretchProcessor::InputProvider provider) override
    {
        inputProvider = std::move (provider);
    }

    int getMaxInputFrameCount() const override
    {
        return maxInputFrameCount;
    }

    int process (const float* const* inputChannels,
                 int inputFrameCount,
                 float* const* outputChannels,
                 int outputFrameCount) override
    {
        (void) inputChannels;
        (void) inputFrameCount;

        if (stretcher == nullptr || outputChannels == nullptr || outputFrameCount <= 0)
            return 0;

        if (inputProvider == nullptr)
            return 0;

        if (seekPending)
        {
            resetState (pendingInputPosition);
            seekPending = false;
        }

        if (! requestInitialized)
            initializeRequest();

        request.speed = parameters.timeRatio > 0.0 ? 1.0 / parameters.timeRatio : 1.0;
        request.pitch = parameters.pitchRatio;

        framesNeeded += static_cast<double> (outputFrameCount);
        int frameCounter = 0;
        const int totalNeededFrames = static_cast<int> (std::round (framesNeeded));

        while (frameCounter < totalNeededFrames)
        {
            const bool hasOutput = outputChunk.request[0] != nullptr
                                && ! std::isnan (outputChunk.request[0]->position)
                                && outputChunk.frameCount > 0
                                && outputChunkConsumed < outputChunk.frameCount;

            if (! hasOutput)
            {
                const auto inputChunk = stretcher->specifyGrain (request, 0.0);
                const int frameCount = inputChunk.end - inputChunk.begin;
                if (frameCount <= 0)
                    break;

                // Track current position from the grain center
                currentInputPosition = static_cast<int64> (request.position);

                for (int ch = 0; ch < channelCount; ++ch)
                    channelPtrs[static_cast<size_t> (ch)] = inputChunkBuffer.data() + ch * maxInputFrameCount;

                int muteHead = 0;
                int muteTail = 0;
                inputProvider (inputChunk.begin,
                               frameCount,
                               channelPtrs.data(),
                               maxInputFrameCount,
                               muteHead,
                               muteTail);

                stretcher->analyseGrain (inputChunkBuffer.data(),
                                         maxInputFrameCount,
                                         muteHead,
                                         muteTail);
                stretcher->synthesiseGrain (outputChunk);
                outputChunkConsumed = 0;
                stretcher->next (request);
                request.reset = false;
                continue;
            }

            const int need = totalNeededFrames - frameCounter;
            const int available = outputChunk.frameCount - outputChunkConsumed;
            const int numFrames = std::min (need, available);

            for (int channel = 0; channel < channelCount; ++channel)
            {
                std::copy (outputChunk.data + outputChunkConsumed + channel * outputChunk.channelStride,
                           outputChunk.data + outputChunkConsumed + channel * outputChunk.channelStride + numFrames,
                           outputChannels[channel] + frameCounter);
            }

            frameCounter += numFrames;
            outputChunkConsumed += numFrames;
        }

        framesNeeded -= frameCounter;
        return frameCounter;
    }

    String getBackendName() const override
    {
        return "Bungee";
    }

    double getLatencyInFrames() const override
    {
        if (outputChunk.request[0] == nullptr || outputChunk.frameCount <= 0)
            return 0.0;

        double outPosition = outputChunk.request[0]->position;
        if (outputChunk.request[1] != nullptr)
        {
            const double span = outputChunk.request[1]->position - outputChunk.request[0]->position;
            outPosition += outputChunkConsumed * span / static_cast<double> (outputChunk.frameCount);
        }

        return static_cast<double> (currentInputPosition) - outPosition;
    }

private:
    void resetState (int64 inputPosition)
    {
        request.position = static_cast<double> (inputPosition);
        request.speed = 1.0;
        request.pitch = parameters.pitchRatio;
        request.reset = true;
        request.resampleMode = resampleMode_autoOut;
        stretcher->preroll (request);

        outputChunk = {};
        outputChunkConsumed = 0;
        framesNeeded = 0.0;
        requestInitialized = true;
        currentInputPosition = inputPosition;
    }

    void initializeRequest()
    {
        request.position = static_cast<double> (pendingInputPosition);
        request.speed = 1.0;
        request.pitch = parameters.pitchRatio;
        request.reset = true;
        request.resampleMode = resampleMode_autoOut;
        stretcher->preroll (request);
        requestInitialized = true;
    }

    Bungee::SampleRates sampleRates {};
    int channelCount = 0;
    int maximumBlockSize = 0;
    int maxInputFrameCount = 0;
    TimeStretchProcessor::Parameters parameters;

    std::unique_ptr<Bungee::Stretcher<Bungee::Basic>> stretcher;
    TimeStretchProcessor::InputProvider inputProvider;

    Bungee::Request request {};
    Bungee::OutputChunk outputChunk {};
    int outputChunkConsumed = 0;
    double framesNeeded = 0.0;
    bool requestInitialized = false;
    bool seekPending = false;
    int64 pendingInputPosition = 0;
    int64 currentInputPosition = 0;

    std::vector<float> inputChunkBuffer;
    std::vector<float*> channelPtrs;
};

#endif

} // namespace yup
