/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include "ExamplePlugin.h"
#include "ExampleEditor.h"

//==============================================================================

ExamplePlugin::ExamplePlugin()
    : yup::AudioProcessor ("MyPlugin",
                           yup::AudioBusLayout ({}, { yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
{
    addParameter (gainParameter = yup::AudioParameterBuilder()
                                      .withID ("volume")
                                      .withName ("Volume")
                                      .withRange (0.0f, 1.0f)
                                      .withDefault (0.5f)
                                      .withSmoothing (20.0f)
                                      .build());
}

ExamplePlugin::~ExamplePlugin()
{
    voices.free();
}

//==============================================================================

void ExamplePlugin::prepareToPlay (float sampleRate, int maxBlockSize)
{
    this->sampleRate = sampleRate;

    gainHandle = yup::AudioParameterHandle (*gainParameter, sampleRate);
}

void ExamplePlugin::releaseResources()
{
    voices.free();
}

void ExamplePlugin::processBlock (yup::AudioSampleBuffer& audioBuffer, yup::MidiBuffer& midiBuffer)
{
    int numSamples = audioBuffer.getNumSamples();
    float* outputL = audioBuffer.getWritePointer (0);
    float* outputR = audioBuffer.getWritePointer (1);

    int nextEventSample = midiBuffer.getNumEvents() ? 0 : numSamples;
    auto midiIterator = midiBuffer.begin();

    gainHandle.updateNextAudioBlock();

    for (int currentSample = 0; currentSample < numSamples;)
    {
        while (midiIterator != midiBuffer.end() && nextEventSample == currentSample)
        {
            const auto& event = *midiIterator;

            if (event.samplePosition != currentSample)
            {
                nextEventSample = event.samplePosition;
                break;
            }

            ++midiIterator;

            const auto& message = event.getMessage();

            // Look through our voices array, and if the event matches any of them, it must have been released.
            if (message.isNoteOff())
            {
                for (int i = 0; i < voices.size(); i++)
                {
                    Voice* voice = &voices[i];

                    if (voice->key == message.getNoteNumber() && voice->channel == message.getChannel())
                    {
                        if (message.getVelocity() == 0)
                            voices.remove (i--); // Stop the voice immediately; don't process the release segment of any ADSR envelopes.
                        else
                            voice->held = false;
                    }
                }
            }

            // If this is a note on event, create a new voice and add it to our array.
            if (message.isNoteOn())
            {
                Voice voice;
                voice.held = true;
                voice.channel = static_cast<int16_t> (message.getChannel());
                voice.key = static_cast<int16_t> (message.getNoteNumber());
                voice.phase = 0.0f;
                voices.add (voice);
            }

            // If this is a controller, set the corresponding parameter
            if (message.isController())
            {
                const int controllerNumber = message.getControllerNumber();
                if (yup::isPositiveAndBelow (controllerNumber, static_cast<int> (getParameters().size())))
                {
                    getParameters()[controllerNumber]->setValue (message.getControllerValue() / 127.0f);
                }
            }

            // TODO - clap supports per voice modulations: clap_event_param_mod_t
        }

        if (midiIterator == midiBuffer.end())
            nextEventSample = numSamples;

        int remainingSamples = nextEventSample - currentSample;
        currentSample += remainingSamples;

        while (--remainingSamples >= 0)
        {
            const float gainValue = gainHandle.getNextValue();

            float sum = 0.0f;

            for (int i = 0; i < voices.size(); i++)
            {
                Voice* voice = &voices[i];
                if (! voice->held)
                    continue;

                float volume = yup::jlimit (0.0f, 1.0f, gainValue + 0.0f); // parameterOffsets[P_VOLUME]);
                sum += std::sin (voice->phase * 2.0f * 3.14159f) * 0.2f * volume;

                voice->phase += 440.0f * std::exp2 ((voice->key - 57.0f) / 12.0f) / sampleRate;
                voice->phase -= std::floor (voice->phase);
            }

            *outputL++ = sum;
            *outputR++ = sum;
        }
    }

    midiBuffer.clear();

    for (int i = 0; i < voices.size(); i++)
    {
        Voice* voice = &voices[i];

        if (voice->held)
            continue;

        midiBuffer.addEvent (yup::MidiMessage::noteOff (voice->channel, voice->key), 0);

        voices.remove (i--);
    }
}

void ExamplePlugin::flush()
{
    voices.free();
}

//==============================================================================

int ExamplePlugin::getCurrentPreset() const noexcept
{
    return 0;
}

void ExamplePlugin::setCurrentPreset (int index) noexcept
{
}

int ExamplePlugin::getNumPresets() const
{
    return 0;
}

yup::String ExamplePlugin::getPresetName (int index) const
{
    return {};
}

void ExamplePlugin::setPresetName (int index, yup::StringRef newName)
{
}

//==============================================================================

yup::Result ExamplePlugin::loadStateFromMemory (const yup::MemoryBlock& memoryBlock)
{
    return yup::Result::fail ("Not implemented");
}

yup::Result ExamplePlugin::saveStateIntoMemory (yup::MemoryBlock& memoryBlock)
{
    return yup::Result::fail ("Not implemented");
}

//==============================================================================

bool ExamplePlugin::hasEditor() const
{
    return true;
}

yup::AudioProcessorEditor* ExamplePlugin::createEditor()
{
    return new ExampleEditor (*this);
}

//==============================================================================

extern "C" yup::AudioProcessor* createPluginProcessor()
{
    return new ExamplePlugin();
}
