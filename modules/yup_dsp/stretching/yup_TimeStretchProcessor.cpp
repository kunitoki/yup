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

//==============================================================================
class TimeStretchProcessor::Engine
{
public:
    virtual ~Engine() = default;

    virtual Result prepare (const TimeStretchProcessor::ProcessSpec& spec) = 0;
    virtual void reset() = 0;
    virtual void setInputPosition (int64 newInputPosition) = 0;
    virtual void setParameters (const TimeStretchProcessor::Parameters& parameters) = 0;
    virtual void setInputProvider (TimeStretchProcessor::InputProvider provider) = 0;
    virtual int getMaxInputFrameCount() const = 0;
    virtual int process (const float* const* inputChannels,
                         int inputFrameCount,
                         float* const* outputChannels,
                         int outputFrameCount) = 0;
    virtual String getBackendName() const = 0;
    virtual double getLatencyInFrames() const = 0;
};

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
        jassert (channels.empty() || requiredSize <= channels.front().size());
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

class TimeDomainEngine : public TimeStretchProcessor::Engine
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

        const int maximumSamplesPerRequest = calculateSamplesPerRequest (maximumPreallocatedTempo);
        const int fifoCapacity = maximumSamplesPerRequest + (spec.maximumBlockSize * 4) + (sequenceWindowLength * 4);
        inputBuffer.prepare (channelCount, fifoCapacity);
        outputBuffer.prepare (channelCount, fifoCapacity);

        reset();
        return Result::ok();
    }

    void reset() override
    {
        inputBuffer.clear();
        outputBuffer.clear();

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
        parameters = newParameters;
        updateTempo();
    }

    void setInputProvider (TimeStretchProcessor::InputProvider provider) override
    {
        inputProvider = std::move (provider);
    }

    int getMaxInputFrameCount() const override
    {
        return jmax (samplesPerRequest, spec.maximumBlockSize + overlapLength);
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

    String getBackendName() const override
    {
        return "Time Domain";
    }

    double getLatencyInFrames() const override
    {
        return jmax (0.0, static_cast<double> (inputBuffer.getNumSamples() - outputBuffer.getNumSamples()));
    }

private:
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
        const auto timeRatio = parameters.timeRatio > 0.0 ? parameters.timeRatio : 1.0;
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
    std::vector<std::vector<float>> midBuffers;
    std::vector<std::vector<float>> referenceMidBuffers;
    std::vector<float*> providerPointers;
};

//==============================================================================
#if YUP_ENABLE_BUNGEE

class BungeeEngine : public TimeStretchProcessor::Engine
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

//==============================================================================
static std::unique_ptr<TimeStretchProcessor::Engine> createEngineForBackend (TimeStretchProcessor::Backend backend)
{
    if (backend == TimeStretchProcessor::Backend::timeDomain)
        return std::make_unique<TimeDomainEngine>();

#if YUP_ENABLE_BUNGEE
    if (backend == TimeStretchProcessor::Backend::bungee)
        return std::make_unique<BungeeEngine>();
#else
    (void) backend;
#endif

    return {};
}

//==============================================================================
TimeStretchProcessor::TimeStretchProcessor() = default;

TimeStretchProcessor::~TimeStretchProcessor() = default;

TimeStretchProcessor::TimeStretchProcessor (TimeStretchProcessor&& other) noexcept = default;

TimeStretchProcessor& TimeStretchProcessor::operator= (TimeStretchProcessor&& other) noexcept = default;

Result TimeStretchProcessor::prepare (const ProcessSpec& newSpec, Backend preferredBackend)
{
    ProcessSpec validatedSpec;
    auto result = validateSpec (newSpec, validatedSpec);
    if (result.failed())
        return result;

    spec = validatedSpec;
    return rebuildEngine (preferredBackend);
}

void TimeStretchProcessor::reset()
{
    if (engine != nullptr)
        engine->reset();
}

void TimeStretchProcessor::setInputPosition (int64 newInputPosition)
{
    if (engine != nullptr)
        engine->setInputPosition (newInputPosition);
}

void TimeStretchProcessor::setInputProvider (InputProvider provider)
{
    inputProvider = std::move (provider);

    if (engine != nullptr)
        engine->setInputProvider (inputProvider);
}

int TimeStretchProcessor::getMaxInputFrameCount() const
{
    if (engine == nullptr)
        return 0;

    return engine->getMaxInputFrameCount();
}

String TimeStretchProcessor::getBackendName() const
{
    if (engine == nullptr)
        return "None";

    return engine->getBackendName();
}

bool TimeStretchProcessor::isBackendAvailable (Backend backendToCheck) noexcept
{
    if (backendToCheck == Backend::automatic)
        return ! getAvailableBackends().empty();

    if (backendToCheck == Backend::timeDomain)
        return true;

#if YUP_ENABLE_BUNGEE
    if (backendToCheck == Backend::bungee)
        return true;
#endif

    return false;
}

std::vector<TimeStretchProcessor::Backend> TimeStretchProcessor::getAvailableBackends()
{
    std::vector<Backend> backends;

    backends.push_back (Backend::timeDomain);

#if YUP_ENABLE_BUNGEE
    backends.push_back (Backend::bungee);
#endif

    return backends;
}

Result TimeStretchProcessor::setBackend (Backend newBackend)
{
    if (backend == newBackend)
        return Result::ok();

    if (! isBackendAvailable (newBackend) && newBackend != Backend::automatic)
        return Result::fail ("Requested backend is not available");

    if (! prepared)
    {
        backend = newBackend;
        return Result::ok();
    }

    return rebuildEngine (newBackend);
}

void TimeStretchProcessor::setParameters (const Parameters& newParameters)
{
    setTimeRatio (newParameters.timeRatio);
    setPitchRatio (newParameters.pitchRatio);
}

void TimeStretchProcessor::setTimeRatio (double newTimeRatio)
{
    jassert (newTimeRatio > 0.0);
    parameters.timeRatio = newTimeRatio > 0.0 ? newTimeRatio : 1.0;

    if (engine != nullptr)
        engine->setParameters (parameters);
}

void TimeStretchProcessor::setPitchRatio (double newPitchRatio)
{
    jassert (newPitchRatio > 0.0);
    parameters.pitchRatio = newPitchRatio > 0.0 ? newPitchRatio : 1.0;

    if (engine != nullptr)
        engine->setParameters (parameters);
}

int TimeStretchProcessor::getExpectedOutputFrameCount (int inputFrameCount) const noexcept
{
    if (inputFrameCount <= 0)
        return 0;

    return static_cast<int> (std::round (static_cast<double> (inputFrameCount) * parameters.timeRatio));
}

ResultValue<int> TimeStretchProcessor::process (const float* const* inputChannels,
                                                int inputFrameCount,
                                                float* const* outputChannels,
                                                int outputFrameCount)
{
    if (! prepared || engine == nullptr)
        return makeResultValueFail ("TimeStretchProcessor is not prepared");

    if (inputFrameCount < 0 || outputFrameCount < 0)
        return makeResultValueFail ("Invalid frame count");

    if (outputFrameCount == 0)
        return makeResultValueOk (0);

    const bool hasInputProvider = (inputProvider != nullptr);
    if (! hasInputProvider && inputFrameCount == 0)
        return makeResultValueOk (0);

    if ((! hasInputProvider && inputChannels == nullptr) || outputChannels == nullptr)
        return makeResultValueFail ("Null channel pointers");

    if (! hasInputProvider && inputFrameCount > spec.maximumBlockSize)
        return makeResultValueFail ("Input frame count exceeds maximum block size");

    const auto processed = engine->process (inputChannels, inputFrameCount, outputChannels, outputFrameCount);
    return makeResultValueOk (processed);
}

ResultValue<int> TimeStretchProcessor::process (const AudioBuffer<float>& input,
                                                AudioBuffer<float>& output,
                                                int outputFrameCount)
{
    if (input.getNumChannels() != spec.numChannels || output.getNumChannels() != spec.numChannels)
        return makeResultValueFail ("Channel count mismatch");

    if (outputFrameCount > output.getNumSamples())
        return makeResultValueFail ("Output buffer too small");

    return process (input.getArrayOfReadPointers(),
                    input.getNumSamples(),
                    output.getArrayOfWritePointers(),
                    outputFrameCount);
}

ResultValue<int> TimeStretchProcessor::processUsingTimeRatio (const float* const* inputChannels,
                                                              int inputFrameCount,
                                                              float* const* outputChannels,
                                                              int outputFrameCapacity)
{
    const auto expectedOutput = getExpectedOutputFrameCount (inputFrameCount);
    if (expectedOutput > outputFrameCapacity)
        return makeResultValueFail ("Output buffer too small for the current time ratio");

    return process (inputChannels, inputFrameCount, outputChannels, expectedOutput);
}

ResultValue<int> TimeStretchProcessor::processUsingTimeRatio (const AudioBuffer<float>& input,
                                                              AudioBuffer<float>& output)
{
    const auto expectedOutput = getExpectedOutputFrameCount (input.getNumSamples());
    if (expectedOutput > output.getNumSamples())
        return makeResultValueFail ("Output buffer too small for the current time ratio");

    return process (input, output, expectedOutput);
}

double TimeStretchProcessor::getLatencyInFrames() const
{
    if (engine == nullptr)
        return 0.0;

    return engine->getLatencyInFrames();
}

TimeStretchProcessor::Backend TimeStretchProcessor::resolveBackend (Backend preferredBackend) noexcept
{
    if (preferredBackend != Backend::automatic)
        return preferredBackend;

#if YUP_ENABLE_BUNGEE
    return Backend::bungee;
#else
    return Backend::timeDomain;
#endif
}

Result TimeStretchProcessor::validateSpec (const ProcessSpec& specToValidate, ProcessSpec& validatedSpec)
{
    if (specToValidate.inputSampleRate <= 0.0)
        return Result::fail ("Input sample rate must be greater than zero");

    if (specToValidate.maximumBlockSize <= 0)
        return Result::fail ("Maximum block size must be greater than zero");

    if (specToValidate.numChannels <= 0)
        return Result::fail ("Number of channels must be greater than zero");

    validatedSpec = specToValidate;

    if (validatedSpec.outputSampleRate <= 0.0)
        validatedSpec.outputSampleRate = validatedSpec.inputSampleRate;

    return Result::ok();
}

Result TimeStretchProcessor::rebuildEngine (Backend preferredBackend)
{
    backend = resolveBackend (preferredBackend);
    engine = createEngineForBackend (backend);

    if (engine == nullptr)
    {
        prepared = false;
        return Result::fail ("No time-stretch backend available");
    }

    const auto result = engine->prepare (spec);
    prepared = result.wasOk();

    if (prepared)
    {
        engine->setParameters (parameters);
        engine->setInputProvider (inputProvider);
    }

    return result;
}

} // namespace yup
