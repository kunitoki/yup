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

#include "ExamplePlugin.h"
#include "ExampleEditor.h"

//==============================================================================

namespace
{
constexpr float millisecondsToSeconds = 0.001f;

yup::AudioParameter::Ptr addExampleParameter (ExamplePlugin& processor,
                                              const yup::String& id,
                                              const yup::String& name,
                                              yup::uint32 hostID,
                                              yup::NormalisableRange<float> range,
                                              float defaultValue,
                                              yup::AudioParameter::ParameterUnit unit = yup::AudioParameter::ParameterUnit::Generic,
                                              yup::StringArray enumNames = {})
{
    auto metadata = yup::AudioParameter::Metadata {};
    metadata.name = name;
    metadata.hostParameterID = hostID;
    metadata.valueRange = range;
    metadata.defaultValue = defaultValue;
    metadata.modulePath = "Main";
    metadata.unit = unit;

    yup::AudioParameter::ValueToString valueToString;
    yup::AudioParameter::StringToValue stringToValue;

    if (! enumNames.isEmpty())
    {
        metadata.setStepped (true);
        metadata.setEnum (true);

        valueToString = [enumNames] (float value) -> yup::String
        {
            return enumNames[yup::jlimit (0, enumNames.size() - 1, yup::roundToInt (value))];
        };

        stringToValue = [enumNames] (const yup::String& text) -> float
        {
            return static_cast<float> (yup::jmax (0, enumNames.indexOf (text)));
        };
    }
    else
    {
        const auto isPercent = unit == yup::AudioParameter::ParameterUnit::Percent;
        const yup::String suffix (yup::AudioParameter::getParameterUnitShortName (unit));

        valueToString = [isPercent, suffix] (float value) -> yup::String
        {
            const auto displayValue = isPercent ? value * 100.0f : value;
            auto text = yup::String (displayValue, isPercent ? 0 : 2);

            if (suffix.isNotEmpty())
                text << " " << suffix;

            return text;
        };

        stringToValue = [isPercent] (const yup::String& text) -> float
        {
            const auto parsed = text.getFloatValue();
            return isPercent ? parsed * 0.01f : parsed;
        };
    }

    yup::AudioParameter::Ptr parameter = new yup::AudioParameter (id, metadata, valueToString, stringToValue);
    processor.addParameter (parameter);
    return parameter;
}
} // namespace

//==============================================================================
yup::AudioBusLayout ExamplePlugin::makeBusLayout()
{
    return yup::AudioBusLayout (
        {},
        { yup::AudioBus ("Output", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Output, 2) });
}

//==============================================================================
ExamplePlugin::ExamplePlugin()
    : yup::AudioProcessor (YupPlugin_Name, makeBusLayout())
{
    gain = addExampleParameter (*this, "gain", "Gain", gainParam, { 0.0f, 1.0f }, 0.8f, yup::AudioParameter::ParameterUnit::Percent);
    waveform = addExampleParameter (*this, "waveform", "Waveform", waveformParam, { 0.0f, 2.0f, 1.0f }, 0.0f, yup::AudioParameter::ParameterUnit::Generic, { "Sine", "Saw", "Square" });
    attack = addExampleParameter (*this, "attack", "Attack", attackParam, { 1.0f, 2000.0f }, 5.0f, yup::AudioParameter::ParameterUnit::Milliseconds);
    release = addExampleParameter (*this, "release", "Release", releaseParam, { 1.0f, 2000.0f }, 150.0f, yup::AudioParameter::ParameterUnit::Milliseconds);
}

//==============================================================================
void ExamplePlugin::prepareToPlay (const yup::AudioSpec& spec)
{
    synth.prepare (spec.sampleRate);
    gainSmoothed.reset (spec.sampleRate, 0.02);
    gainSmoothed.setCurrentAndTargetValue (gain->getValue());

    updateSynthParameters();
}

void ExamplePlugin::releaseResources()
{
    synth.reset();
}

//==============================================================================
void ExamplePlugin::updateSynthParameters() noexcept
{
    synth.setEnvelope (attack->getValue() * millisecondsToSeconds, release->getValue() * millisecondsToSeconds);
    synth.setWaveform (static_cast<ExampleWaveform> (static_cast<int> (waveform->getValue())));
}

//==============================================================================
void ExamplePlugin::processBlock (yup::AudioProcessContext<float>& context)
{
    auto& audio = context.audio;
    const auto numSamples = audio.getNumSamples();
    const auto numChannels = audio.getNumChannels();

    updateSynthParameters();

    keyboardState.processNextMidiBuffer (context.midi, 0, numSamples, true);

    gainSmoothed.setTargetValue (gain->getValue());

    auto eventIterator = context.midi.begin();
    const auto eventEnd = context.midi.end();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        while (eventIterator != eventEnd && (*eventIterator).samplePosition <= sample)
        {
            const auto message = (*eventIterator).getMessage();

            if (message.isNoteOn())
                synth.noteOn (message.getNoteNumber(), message.getFloatVelocity());
            else if (message.isNoteOff())
                synth.noteOff (message.getNoteNumber());
            else if (message.isAllNotesOff() || message.isAllSoundOff())
                synth.allNotesOff();

            ++eventIterator;
        }

        const auto value = synth.renderSample() * gainSmoothed.getNextValue();

        for (int channel = 0; channel < numChannels; ++channel)
            audio.setSample (channel, sample, value);
    }
}

//==============================================================================
yup::Result ExamplePlugin::loadStateFromDataTree (const yup::DataTree& state)
{
    for (const auto& parameter : getParameters())
    {
        const auto id = parameter->getID();
        if (state.hasProperty (id))
            parameter->setValue (static_cast<float> (state.getProperty (id)));
    }

    updateSynthParameters();
    return yup::Result::ok();
}

yup::Result ExamplePlugin::saveStateIntoDataTree (yup::DataTree& state)
{
    state = yup::DataTree ("PluginState");
    auto transaction = state.beginTransaction();

    for (const auto& parameter : getParameters())
        transaction.setProperty (parameter->getID(), parameter->getValue());

    return yup::Result::ok();
}

//==============================================================================
yup::AudioProcessorEditor* ExamplePlugin::createEditor()
{
    return new ExampleEditor (*this);
}

//==============================================================================
extern "C" yup::AudioProcessor* createPluginProcessor()
{
    return new ExamplePlugin();
}
