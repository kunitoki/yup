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

namespace
{
std::atomic<AudioPluginWrapperType> currentWrapperType { AudioPluginWrapperType::unknown };
std::atomic<PluginHostType::Host> currentHostType { PluginHostType::Host::unknown };
} // namespace

PluginHostType AudioProcessor::getHostType() noexcept
{
    return PluginHostType (currentHostType.load (std::memory_order_relaxed));
}

void AudioProcessor::setHostType (PluginHostType hostType) noexcept
{
    currentHostType.store (hostType.getHost(), std::memory_order_relaxed);
}

AudioPluginWrapperType AudioProcessor::getWrapperType() noexcept
{
    return currentWrapperType.load (std::memory_order_relaxed);
}

void AudioProcessor::setWrapperType (AudioPluginWrapperType wrapperType) noexcept
{
    currentWrapperType.store (wrapperType, std::memory_order_relaxed);
}

StringRef AudioProcessor::getWrapperTypeName (AudioPluginWrapperType wrapperType) noexcept
{
    switch (wrapperType)
    {
        case AudioPluginWrapperType::clap:
            return "clap";
        case AudioPluginWrapperType::vst3:
            return "vst3";
        case AudioPluginWrapperType::au:
            return "au";
        case AudioPluginWrapperType::auv3:
            return "auv3";
        case AudioPluginWrapperType::aax:
            return "aax";
        case AudioPluginWrapperType::lv2:
            return "lv2";
        case AudioPluginWrapperType::standalone:
            return "standalone";
        case AudioPluginWrapperType::unknown:
            break;
    }

    return "unknown";
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

bool AudioProcessor::isBusLayoutSupported (const AudioBusLayout& layout) const
{
    return layout == busLayout;
}

Result AudioProcessor::setBusLayout (const AudioBusLayout& newLayout)
{
    if (newLayout == busLayout)
        return Result::ok();

    if (! newLayout.hasSameBusStructure (busLayout))
        return Result::fail ("Bus layout has a different bus structure");

    if (! isBusLayoutSupported (newLayout))
        return Result::fail ("Bus layout is not supported by the processor");

    busLayout = newLayout;
    updateHostDisplay (ChangeDetails().withBusLayoutChanged (true));

    return Result::ok();
}

int AudioProcessor::getParameterIndexForMidiController (int channel, int controller) const
{
    ignoreUnused (channel, controller);
    return -1;
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
