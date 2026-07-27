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
    : BaseDomainProcessor (name)
    , busLayout (std::move (busLayout))
{
}

//==============================================================================

AudioProcessor::~AudioProcessor() = default;

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

bool AudioProcessor::acceptsMidi() const noexcept
{
    for (const auto& bus : busLayout.getInputBuses())
        if (bus.getType() == AudioBus::Type::Midi)
            return true;

    return false;
}

bool AudioProcessor::producesMidi() const noexcept
{
    for (const auto& bus : busLayout.getOutputBuses())
        if (bus.getType() == AudioBus::Type::Midi)
            return true;

    return false;
}

//==============================================================================

AudioProcessorEditor* AudioProcessor::createEditor()
{
    jassert (hasEditor());
    return nullptr;
}

//==============================================================================

void AudioProcessor::setPlaybackConfiguration (float sampleRate, int samplesPerBlock)
{
    releaseResources();

    AudioProcessorBase::setPlaybackConfiguration (sampleRate, samplesPerBlock);

    prepareToPlay (AudioSpec (sampleRate, samplesPerBlock));
}

} // namespace yup
