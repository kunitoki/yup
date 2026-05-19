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

                // Create channel pointers into strided buffer
                std::vector<float*> channelPtrs (static_cast<size_t> (channelCount));
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
};

#endif

//==============================================================================
static std::unique_ptr<TimeStretchProcessor::Engine> createEngineForBackend (TimeStretchProcessor::Backend backend)
{
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

#if YUP_ENABLE_BUNGEE
    if (backendToCheck == Backend::bungee)
        return true;
#endif

    return false;
}

std::vector<TimeStretchProcessor::Backend> TimeStretchProcessor::getAvailableBackends()
{
    std::vector<Backend> backends;

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
    return Backend::automatic;
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
