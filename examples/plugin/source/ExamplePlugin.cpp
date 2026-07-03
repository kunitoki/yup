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

#include <memory>

//==============================================================================

namespace
{

constexpr char examplePluginStateMagic[] = { 'Y', 'U', 'P', 'S' };
constexpr int examplePluginStateVersion = 1;

const char* getPluginFormatName()
{
#if YUP_AUDIO_PLUGIN_ENABLE_AU
    return "au";
#elif YUP_AUDIO_PLUGIN_ENABLE_CLAP
    return "clap";
#elif YUP_AUDIO_PLUGIN_ENABLE_VST3
    return "vst3";
#elif YUP_AUDIO_PLUGIN_ENABLE_LV2
    return "lv2";
#elif YUP_AUDIO_PLUGIN_ENABLE_AAX
    return "aax";
#elif YUP_AUDIO_PLUGIN_ENABLE_STANDALONE
    return "standalone";
#else
    return "unknown";
#endif
}

class ExamplePluginLogger final
{
public:
    ExamplePluginLogger()
    {
        const auto logFileName = yup::String (YupPlugin_Name) + "_" + getPluginFormatName() + ".log";
        logger.reset (new yup::FileLogger (yup::FileLogger::getSystemLogFileFolder().getChildFile (logFileName),
                                           yup::String (YupPlugin_Name) + " " + getPluginFormatName() + " log"));

        yup::Logger::setCurrentLogger (logger.get());
        yup::Logger::writeToLog ("Logger initialised: " + logger->getLogFile().getFullPathName());
    }

    void setAsCurrentLogger()
    {
        yup::Logger::setCurrentLogger (logger.get());
    }

    ~ExamplePluginLogger()
    {
        if (yup::Logger::getCurrentLogger() == logger.get())
            yup::Logger::setCurrentLogger (nullptr);
    }

private:
    std::unique_ptr<yup::FileLogger> logger;
};

void initialiseExamplePluginLogger()
{
    static ExamplePluginLogger logger;
    logger.setAsCurrentLogger();
}

} // namespace

//==============================================================================

ExamplePlugin::ExamplePlugin()
    : yup::AudioProcessor ("MyPlugin",
                           yup::AudioBusLayout ({}, { yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
{
    initialiseExamplePluginLogger();

    addParameter (gainParameter = yup::AudioParameterBuilder()
                                      .withID ("volume")
                                      .withName ("Volume")
                                      .withRange (0.0f, 1.0f)
                                      .withDefault (0.5f)
                                      .withSmoothing (20.0f)
                                      .withModulatable (true)
                                      .build());
}

ExamplePlugin::~ExamplePlugin()
{
    voices.free();
}

//==============================================================================

void ExamplePlugin::prepareToPlay (const yup::AudioSpec& spec)
{
    this->sampleRate = spec.sampleRate;

    gainHandle = yup::AudioParameterHandle (*gainParameter, spec.sampleRate);
}

void ExamplePlugin::releaseResources()
{
    voices.free();
}

void ExamplePlugin::processBlock (yup::AudioProcessContext<float>& context)
{
    auto& audioBuffer = context.audio;
    auto& midiBuffer = context.midi;

    int numSamples = audioBuffer.getNumSamples();
    float* outputL = audioBuffer.getWritePointer (0);
    float* outputR = audioBuffer.getWritePointer (1);

    int nextEventSample = midiBuffer.getNumEvents() ? 0 : numSamples;
    auto midiIterator = midiBuffer.begin();

    gainHandle.prepareBlock (context.params, gainParameter->getIndexInContainer());

    for (int currentSample = 0; currentSample < numSamples;)
    {
        gainHandle.advanceToSample (currentSample);

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
                    getParameters()[controllerNumber]->setNormalizedValue (message.getControllerValue() / 127.0f);
                    // gainHandle.updateNextAudioBlock();
                }
            }

            // Per-voice (polyphonic) modulation is not yet supported; global modulation
            // via CLAP_EVENT_PARAM_MOD is handled by the plugin wrapper and already
            // reflected in the value returned by gainHandle.getNextValue().
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
    return currentPreset;
}

void ExamplePlugin::setCurrentPreset (int index) noexcept
{
    if (currentPreset == index)
        return;

    if (yup::isPositiveAndBelow (index, yup::numElementsInArray (presets)))
    {
        currentPreset = index;
        gainParameter->setValue (presets[index].gainValue);
    }
}

int ExamplePlugin::getNumPresets() const
{
    return yup::numElementsInArray (presets);
}

yup::String ExamplePlugin::getPresetName (int index) const
{
    if (yup::isPositiveAndBelow (index, yup::numElementsInArray (presets)))
        return presets[index].name;

    return "Invalid Preset";
}

void ExamplePlugin::setPresetName (int index, yup::StringRef newName)
{
    if (yup::isPositiveAndBelow (index, yup::numElementsInArray (presets)))
        presets[index].name = newName;
}

//==============================================================================

yup::Result ExamplePlugin::loadStateFromMemory (const yup::MemoryBlock& memoryBlock)
{
    constexpr size_t expectedSize = sizeof (examplePluginStateMagic) + (sizeof (int) * 2) + sizeof (float);

    if (memoryBlock.getSize() != expectedSize)
        return yup::Result::fail ("Invalid example plugin state size");

    yup::MemoryInputStream stream (memoryBlock, false);

    char magic[sizeof (examplePluginStateMagic)] {};
    if (stream.read (magic, sizeof (magic)) != static_cast<int> (sizeof (magic)))
        return yup::Result::fail ("Invalid example plugin state header");

    for (size_t i = 0; i < sizeof (examplePluginStateMagic); ++i)
        if (magic[i] != examplePluginStateMagic[i])
            return yup::Result::fail ("Invalid example plugin state header");

    const auto version = stream.readInt();
    if (version != examplePluginStateVersion)
        return yup::Result::fail ("Unsupported example plugin state version");

    const auto presetIndex = stream.readInt();
    if (! yup::isPositiveAndBelow (presetIndex, yup::numElementsInArray (presets)))
        return yup::Result::fail ("Invalid example plugin preset index");

    const auto gainValue = stream.readFloat();
    if (! (gainValue >= gainParameter->getMinimumValue() && gainValue <= gainParameter->getMaximumValue()))
        return yup::Result::fail ("Invalid example plugin gain value");

    currentPreset = presetIndex;
    gainParameter->setValue (gainValue);

    return yup::Result::ok();
}

yup::Result ExamplePlugin::saveStateIntoMemory (yup::MemoryBlock& memoryBlock)
{
    yup::MemoryOutputStream stream (memoryBlock, false);

    if (! stream.write (examplePluginStateMagic, sizeof (examplePluginStateMagic))
        || ! stream.writeInt (examplePluginStateVersion)
        || ! stream.writeInt (currentPreset)
        || ! stream.writeFloat (gainParameter->getValue()))
        return yup::Result::fail ("Failed to write example plugin state");

    stream.flush();
    return yup::Result::ok();
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
