/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

AudioProcessor::AudioProcessor (StringRef name, AudioBusLayout busLayout)
    : processorName (name)
    , busLayout (std::move (busLayout))
{
}

//==============================================================================

AudioProcessor::~AudioProcessor() = default;

//==============================================================================

void AudioProcessor::addParameter (AudioParameter::Ptr parameter)
{
    jassert (parameter != nullptr);
    if (parameter == nullptr)
        return;

    if (parameterMap.find (parameter->getID()) != parameterMap.end())
        return;

    const auto hostParameterID = parameter->hasExplicitHostParameterID()
                                   ? parameter->getHostParameterID()
                                   : static_cast<uint32> (parameters.size());
    jassert (hostParameterID != AudioParameter::invalidHostParameterID);
    jassert (hostParameterID <= AudioParameter::maximumHostParameterID);

    if (parameterHostIDMap.find (hostParameterID) != parameterHostIDMap.end())
    {
        jassertfalse;
        return;
    }

    parameter->setIndexInContainer (static_cast<int> (parameters.size()));

    parameterMap.emplace (parameter->getID(), parameter);
    parameterHostIDMap.emplace (hostParameterID, parameter);
    parameters.emplace_back (std::move (parameter));
}

AudioParameter::Ptr AudioProcessor::getParameterByID (StringRef parameterID) const
{
    const auto iterator = parameterMap.find (String (parameterID));
    return iterator != parameterMap.end() ? iterator->second : nullptr;
}

AudioParameter::Ptr AudioProcessor::getParameterByHostID (uint32 hostParameterID) const
{
    const auto iterator = parameterHostIDMap.find (hostParameterID);
    return iterator != parameterHostIDMap.end() ? iterator->second : nullptr;
}

int AudioProcessor::getParameterIndexByHostID (uint32 hostParameterID) const
{
    if (auto parameter = getParameterByHostID (hostParameterID))
        return parameter->getIndexInContainer();

    return -1;
}

void AudioProcessor::addListener (Listener* listener)
{
    listeners.add (listener);
}

void AudioProcessor::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

void AudioProcessor::updateHostDisplay (ChangeDetails details)
{
    listeners.call (&Listener::audioProcessorChanged, this, details);
}

//==============================================================================

int AudioProcessor::getNumAudioOutputs() const
{
    int count = 0;

    for (const auto& bus : busLayout.getOutputBuses())
        if (bus.getType() == AudioBus::Type::Audio)
            ++count;

    return count;
}

int AudioProcessor::getNumAudioInputs() const
{
    int count = 0;

    for (const auto& bus : busLayout.getInputBuses())
        if (bus.getType() == AudioBus::Type::Audio)
            ++count;

    return count;
}

//==============================================================================

void AudioProcessor::suspendProcessing (bool shouldSuspend)
{
    auto lock = CriticalSection::ScopedLockType (processLock);

    processIsSuspended.store (shouldSuspend);
}

bool AudioProcessor::isSuspended() const
{
    return processIsSuspended.load();
}

//==============================================================================

void AudioProcessor::setLatencySamples (int newLatencySamples)
{
    const auto clampedLatencySamples = jmax (0, newLatencySamples);
    const auto oldLatencySamples = latencySamples.exchange (clampedLatencySamples);

    if (oldLatencySamples != clampedLatencySamples)
        updateHostDisplay (ChangeDetails().withLatencyChanged (true));
}

//==============================================================================

void AudioProcessor::setProcessingPrecision (ProcessingPrecision precision)
{
    if (precision == ProcessingPrecision::doublePrecision && ! supportsDoublePrecisionProcessing())
    {
        jassertfalse;
        processingPrecision = ProcessingPrecision::singlePrecision;
        return;
    }

    processingPrecision = precision;
}

//==============================================================================

void AudioProcessor::setPlaybackConfiguration (float sampleRate, int samplesPerBlock)
{
    releaseResources();
    this->sampleRate = sampleRate;
    this->samplesPerBlock = samplesPerBlock;
    prepareToPlay (sampleRate, samplesPerBlock);
}

} // namespace yup
