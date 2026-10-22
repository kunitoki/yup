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

#include "PrismProcessor.h"
#include "PrismEditor.h"
#include "PluginLogger.h"

PrismProcessor::PrismProcessor()
    : yup::AudioProcessor ("Prism",
                           yup::AudioBusLayout ({}, { yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    , engine (synthParameters, partials)
{
    PluginLogger::initialise();

    synthParameters.addTo (*this);

    for (const auto& parameter : getParameters())
        parameterValues.push_back (parameter->getValue());

    playedValues = parameterValues;
    changedByEvents.assign (parameterValues.size(), 0);
    volumeIndex = synthParameters.volume->getIndexInContainer();

    for (int index = 0; index < SynthPatch::getNumFactoryPresets(); ++index)
        presetNames.push_back (SynthPatch::getFactoryPresetName (index));
}

PrismProcessor::~PrismProcessor() = default;

//==============================================================================
void PrismProcessor::prepareToPlay (const yup::AudioSpec& spec)
{
    engine.prepare (spec.sampleRate, yup::jmax (spec.maxBlockSize, SynthExample::maxBlockSize));

    const auto parameters = getParameters();
    for (std::size_t index = 0; index < parameters.size(); ++index)
        parameterValues[index] = playedValues[index] = parameters[index]->getValue();

    outputGain.reset (spec.sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (playedValues[static_cast<std::size_t> (volumeIndex)]);

    loadMeasurer.reset (spec.sampleRate, spec.maxBlockSize);
}

void PrismProcessor::releaseResources()
{
    engine.allNotesOff (0, false);
}

void PrismProcessor::flush()
{
    engine.allNotesOff (0, false);
}

void PrismProcessor::processBlock (yup::AudioProcessContext<float>& context)
{
    const yup::ScopedNoDenormals noDenormals;

    auto& audio = context.audio;
    const auto numSamples = audio.getNumSamples();
    const yup::AudioProcessLoadMeasurer::ScopedTimer renderTimer (loadMeasurer, numSamples);

    audio.clear();

    followParameters (context.params);

    editorMidi.drainInto (context.midi, 0);

    if (numSamples > 0)
        keyboardState.processNextMidiBuffer (context.midi, 0, numSamples, true);

    // Parameter events land at the start of the control chunk they fall in, where the
    // engine applies its own modulation and its smoothers ramp to the new value.
    auto change = context.params.begin();

    for (int start = 0; start < numSamples; start += SynthExample::controlChunk)
    {
        const auto count = yup::jmin (SynthExample::controlChunk, numSamples - start);

        for (; change != context.params.end() && change->sampleOffset <= start; ++change)
            applyParameterChange (*change);

        engine.renderRange (audio, context.midi, start, count, playedValues);

        outputGain.setTargetValue (playedValues[static_cast<std::size_t> (volumeIndex)]);

        for (int sample = start; sample < start + count; ++sample)
        {
            const auto gain = outputGain.getNextValue();

            for (int channel = 0; channel < audio.getNumChannels(); ++channel)
                audio.getWritePointer (channel)[sample] *= gain;
        }
    }

    for (; change != context.params.end(); ++change)
        applyParameterChange (*change);

    // Whatever is left in the buffer goes back to the host as output events.
    context.midi.clear();

    if (numSamples > 0 && audio.getNumChannels() > 0 && ! scopeReady.load (std::memory_order_acquire))
    {
        scopeCount = yup::jmin (numSamples, SynthExample::maxBlockSize);
        std::copy_n (audio.getReadPointer (0), scopeCount, scopeSamples.begin());
        scopeReady.store (true, std::memory_order_release);
    }
}

void PrismProcessor::followParameters (const yup::ParameterChangeBuffer& changes) noexcept
{
    std::fill (changedByEvents.begin(), changedByEvents.end(), 0);

    for (const auto& change : changes)
        if (yup::isPositiveAndBelow (change.parameterIndex, static_cast<int> (changedByEvents.size())))
            changedByEvents[static_cast<std::size_t> (change.parameterIndex)] = 1;

    const auto parameters = getParameters();

    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        const auto& parameter = parameters[index];
        const auto value = parameter->getValue();

        if (value == parameterValues[index])
            continue;

        // A parameter moved by the editor, a preset or a state load shifts what is played
        // by the same amount, so a host modulation offset on top of it survives. One the
        // host moved with events this block follows those events instead.
        if (changedByEvents[index] == 0)
            playedValues[index] = yup::jlimit (parameter->getMinimumValue(),
                                               parameter->getMaximumValue(),
                                               playedValues[index] + value - parameterValues[index]);

        parameterValues[index] = value;
    }
}

void PrismProcessor::applyParameterChange (const yup::ParameterChange& change) noexcept
{
    const auto parameters = getParameters();

    if (! yup::isPositiveAndBelow (change.parameterIndex, static_cast<int> (parameters.size())))
        return;

    const auto index = static_cast<std::size_t> (change.parameterIndex);
    playedValues[index] = parameters[index]->convertToDenormalizedValue (change.normalizedValue);
}

bool PrismProcessor::readScope (std::vector<float>& destination)
{
    if (! scopeReady.load (std::memory_order_acquire))
        return false;

    destination.assign (scopeSamples.begin(), scopeSamples.begin() + scopeCount);
    scopeReady.store (false, std::memory_order_release);
    return true;
}

//==============================================================================
int PrismProcessor::getCurrentPreset() const noexcept
{
    return currentPreset.load();
}

void PrismProcessor::setCurrentPreset (int index) noexcept
{
    const auto state = SynthPatch::createFactoryState (index);

    if (state.isValid())
        applyState (state, ChangeDetails {}.withProgramChanged (true));
}

int PrismProcessor::getNumPresets() const
{
    return static_cast<int> (presetNames.size());
}

yup::String PrismProcessor::getPresetName (int index) const
{
    if (yup::isPositiveAndBelow (index, getNumPresets()))
        return presetNames[static_cast<std::size_t> (index)];

    return {};
}

void PrismProcessor::setPresetName (int index, yup::StringRef newName)
{
    if (yup::isPositiveAndBelow (index, getNumPresets()))
        presetNames[static_cast<std::size_t> (index)] = newName;
}

void PrismProcessor::randomize()
{
    applyState (SynthPatch::createRandomState (synthParameters, yup::Random::getSystemRandom()), {});
}

//==============================================================================
yup::Result PrismProcessor::loadStateFromDataTree (const yup::DataTree& state)
{
    return applyState (state, {});
}

yup::Result PrismProcessor::saveStateIntoDataTree (yup::DataTree& state)
{
    state = SynthPatch::createState (getParameters(), partials, currentPreset.load());
    return yup::Result::ok();
}

yup::Result PrismProcessor::savePresetFile (const yup::File& file)
{
    const auto xml = SynthPatch::createState (getParameters(), partials, -1).createXml();

    if (xml == nullptr || ! xml->writeTo (file))
        return yup::Result::fail ("Cannot write " + file.getFullPathName());

    return yup::Result::ok();
}

yup::Result PrismProcessor::loadPresetFile (const yup::File& file)
{
    const auto xml = yup::parseXML (file);

    if (xml == nullptr)
        return yup::Result::fail ("Cannot read " + file.getFullPathName());

    return applyState (yup::DataTree::fromXml (*xml), ChangeDetails {}.withProgramChanged (true));
}

yup::Result PrismProcessor::applyState (const yup::DataTree& state, ChangeDetails details)
{
    if (auto result = SynthPatch::loadState (state, getParameters(), partials); result.failed())
        return result;

    currentPreset.store (SynthPatch::getPresetIndex (state));

    updateHostDisplay (details.withParameterValuesChanged (true).withNonParameterStateChanged (true));
    return yup::Result::ok();
}

//==============================================================================
yup::AudioProcessorEditor* PrismProcessor::createEditor()
{
    return new PrismEditor (*this);
}

//==============================================================================
extern "C" yup::AudioProcessor* createPluginProcessor()
{
    return new PrismProcessor();
}
