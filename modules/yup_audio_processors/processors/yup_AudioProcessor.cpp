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

AudioProcessor::~AudioProcessor()
{
}

//==============================================================================

void AudioProcessor::addParameter (AudioParameter::Ptr parameter)
{
    jassert (parameter != nullptr);

    parameterMap[parameter->getID()] = parameter;

    parameters.emplace_back (std::move (parameter));
}

//==============================================================================

int AudioProcessor::getNumAudioOutputs() const
{
    return static_cast<int> (busLayout.getOutputBuses().size());
}

int AudioProcessor::getNumAudioInputs() const
{
    return static_cast<int> (busLayout.getInputBuses().size());
}

//==============================================================================

void AudioProcessor::setPlayHead (AudioPlayHead* playHead)
{
    this->playHead = playHead;
}

//==============================================================================

void AudioProcessor::suspendProcessing (bool shouldSuspend)
{
     auto lock = CriticalSection::ScopedLockType (processLock);

     processIsSuspended = shouldSuspend;
}

bool AudioProcessor::isSuspended() const
{
    return processIsSuspended;
}

//==============================================================================

void AudioProcessor::setPlaybackConfiguration (float sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    this->samplesPerBlock = samplesPerBlock;

    prepareToPlay (sampleRate, samplesPerBlock);
}

} // namespace yup
