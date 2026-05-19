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

namespace
{

template <typename FloatType>
void processPluginBypassedBlock (const AudioPluginDescription& description,
                                 AudioBuffer<FloatType>& audioBuffer) noexcept
{
    const int numSamples = audioBuffer.getNumSamples();
    const int numBufferChannels = audioBuffer.getNumChannels();
    const int numInputs = jmin (description.numInputChannels, numBufferChannels);
    const int numOutputs = jmin (description.numOutputChannels, numBufferChannels);
    const int numChannelsToCopy = jmin (numInputs, numOutputs);

    for (int channel = 0; channel < numChannelsToCopy; ++channel)
    {
        const auto* source = audioBuffer.getReadPointer (channel);
        auto* destination = audioBuffer.getWritePointer (channel);

        if (source != destination)
            FloatVectorOperations::copy (destination, source, numSamples);
    }

    for (int channel = numChannelsToCopy; channel < numOutputs; ++channel)
        audioBuffer.clear (channel, 0, numSamples);
}

} // namespace

AudioPluginInstance::AudioPluginInstance (const AudioPluginDescription& description,
                                          AudioBusLayout busLayout)
    : AudioProcessor (description.name, std::move (busLayout))
    , pluginDescription (description)
{
}

AudioPluginInstance::~AudioPluginInstance() = default;

const AudioPluginDescription& AudioPluginInstance::getDescription() const noexcept
{
    return pluginDescription;
}

AudioPluginFormatType AudioPluginInstance::getFormatType() const noexcept
{
    return pluginDescription.formatType;
}

void AudioPluginInstance::setNonRealtime (bool shouldBeNonRealtime) noexcept
{
    if (nonRealtime == shouldBeNonRealtime)
        return;

    nonRealtime = shouldBeNonRealtime;
    nonRealtimeStateChanged();
}

bool AudioPluginInstance::isNonRealtime() const noexcept
{
    return nonRealtime;
}

void AudioPluginInstance::setBypassed (bool shouldBeBypassed) noexcept
{
    if (bypassed == shouldBeBypassed)
        return;

    bypassed = shouldBeBypassed;
    bypassStateChanged();
}

bool AudioPluginInstance::isBypassed() const noexcept
{
    return bypassed;
}

void AudioPluginInstance::processBlockBypassed (AudioBuffer<float>& audioBuffer, MidiBuffer& midiBuffer)
{
    ignoreUnused (midiBuffer);
    processPluginBypassedBlock (pluginDescription, audioBuffer);
}

void AudioPluginInstance::processBlockBypassed (AudioBuffer<double>& audioBuffer, MidiBuffer& midiBuffer)
{
    ignoreUnused (midiBuffer);
    processPluginBypassedBlock (pluginDescription, audioBuffer);
}

} // namespace yup
