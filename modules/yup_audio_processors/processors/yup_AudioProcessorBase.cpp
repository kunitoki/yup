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

AudioProcessorBase::AudioProcessorBase (StringRef name)
    : processorName (name)
{
}

//==============================================================================

AudioProcessorBase::~AudioProcessorBase() = default;

//==============================================================================

void AudioProcessorBase::addParameter (AudioParameter::Ptr parameter)
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

AudioParameter::Ptr AudioProcessorBase::getParameterByID (StringRef parameterID) const
{
    const auto iterator = parameterMap.find (String (parameterID));
    return iterator != parameterMap.end() ? iterator->second : nullptr;
}

AudioParameter::Ptr AudioProcessorBase::getParameterByHostID (uint32 hostParameterID) const
{
    const auto iterator = parameterHostIDMap.find (hostParameterID);
    return iterator != parameterHostIDMap.end() ? iterator->second : nullptr;
}

int AudioProcessorBase::getParameterIndexByHostID (uint32 hostParameterID) const
{
    if (auto parameter = getParameterByHostID (hostParameterID))
        return parameter->getIndexInContainer();

    return -1;
}

void AudioProcessorBase::addListener (Listener* listener)
{
    listeners.add (listener);
}

void AudioProcessorBase::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

void AudioProcessorBase::updateHostDisplay (ChangeDetails details)
{
    listeners.call (&Listener::audioProcessorChanged, this, details);
}

//==============================================================================

Result AudioProcessorBase::loadStateFromDataTree (const DataTree& state)
{
    ignoreUnused (state);
    return Result::fail ("Processor does not support DataTree state");
}

Result AudioProcessorBase::saveStateIntoDataTree (DataTree& state)
{
    ignoreUnused (state);
    return Result::fail ("Processor does not support DataTree state");
}

Result AudioProcessorBase::loadStateFromMemory (const MemoryBlock& memoryBlock)
{
    if (! supportsDataTreeState())
        return Result::fail ("Processor does not support binary state");

    MemoryInputStream stream (memoryBlock, false);
    auto xml = parseXML (stream.readEntireStreamAsString());

    if (xml == nullptr)
        return Result::fail ("Processor state is not valid XML");

    auto state = DataTree::fromXml (*xml);
    if (! state.isValid())
        return Result::fail ("Processor state is not a valid DataTree");

    return loadStateFromDataTree (state);
}

Result AudioProcessorBase::saveStateIntoMemory (MemoryBlock& memoryBlock)
{
    if (! supportsDataTreeState())
        return Result::fail ("Processor does not support binary state");

    DataTree state;
    if (const auto result = saveStateIntoDataTree (state); result.failed())
        return result;

    auto xml = state.createXml();
    if (xml == nullptr)
        return Result::fail ("Processor DataTree state is invalid");

    MemoryOutputStream stream (memoryBlock, false);
    xml->writeTo (stream);
    stream.flush();

    return Result::ok();
}

//==============================================================================

void AudioProcessorBase::suspendProcessing (bool shouldSuspend)
{
    auto lock = CriticalSection::ScopedLockType (processLock);

    processIsSuspended.fetch_add (shouldSuspend ? 1 : -1);
}

bool AudioProcessorBase::isSuspended() const
{
    return processIsSuspended.load() > 0;
}

//==============================================================================

void AudioProcessorBase::setLatencySamples (int newLatencySamples)
{
    const auto clampedLatencySamples = jmax (0, newLatencySamples);
    const auto oldLatencySamples = latencySamples.exchange (clampedLatencySamples);

    if (oldLatencySamples != clampedLatencySamples)
        updateHostDisplay (ChangeDetails().withLatencyChanged (true));
}

//==============================================================================

void AudioProcessorBase::setPlaybackConfiguration (float sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    this->samplesPerBlock = samplesPerBlock;
}

} // namespace yup
