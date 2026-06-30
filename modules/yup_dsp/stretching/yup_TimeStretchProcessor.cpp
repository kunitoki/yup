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
static std::unique_ptr<TimeStretchProcessor::Engine> createEngineForBackend (TimeStretchProcessor::Backend backend)
{
    if (backend == TimeStretchProcessor::Backend::timeDomain)
        return std::make_unique<TimeDomainTimeStretchBackend>();

#if YUP_ENABLE_BUNGEE
    if (backend == TimeStretchProcessor::Backend::bungee)
        return std::make_unique<BungeeTimeStretchBackend>();
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
