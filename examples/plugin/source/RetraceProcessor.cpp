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

#include "RetraceProcessor.h"
#include "RetraceEditor.h"
#include "PluginLogger.h"
#include "ui/SynthThemes.h"
#include "ui/SynthPresetLibrary.h"
#include "browser/PresetBrowseState.h"

namespace
{
const yup::Identifier themeProperty ("theme");
const yup::Identifier presetFileProperty ("presetFile");
} // namespace

RetraceProcessor::RetraceProcessor()
    : yup::AudioProcessor ("Retrace",
                           yup::AudioBusLayout ({}, { yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    , engine (synthParameters, partials, routes, curves)
{
    PluginLogger::initialise();

    synthParameters.addTo (*this);
    synthParameters.quality->addListener (this);

    for (const auto& parameter : getParameters())
        parameterValues.push_back (parameter->getValue());

    playedValues = parameterValues;
    globalValues = parameterValues;
    changedByEvents.assign (parameterValues.size(), 0);
    volumeIndex = synthParameters.volume->getIndexInContainer();

    for (int index = 0; index < SynthPatch::getNumFactoryPresets(); ++index)
        presetNames.push_back (SynthPatch::getFactoryPresetName (index));

    themeName = SynthThemes::getThemes()[static_cast<std::size_t> (SynthThemes::loadSelectedTheme())].name;
}

RetraceProcessor::~RetraceProcessor()
{
    synthParameters.quality->removeListener (this);

    if (presetLibrary != nullptr)
        presetLibrary->removeFileListener (this);
}

//==============================================================================
void RetraceProcessor::prepareToPlay (const yup::AudioSpec& spec)
{
    preparedSampleRate = spec.sampleRate;
    preparedBlockSize = yup::jmax (spec.maxBlockSize, SynthExample::maxBlockSize);
    prepareEngine (synthParameters.getQuality());

    effects.prepare (spec.sampleRate, SynthExample::controlChunk);

    for (auto& clipper : masterClippers)
        clipper.prepare (spec.sampleRate, spec.maxBlockSize);

    const auto parameters = getParameters();
    for (std::size_t index = 0; index < parameters.size(); ++index)
        parameterValues[index] = playedValues[index] = parameters[index]->getValue();

    outputGain.reset (spec.sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (playedValues[static_cast<std::size_t> (volumeIndex)]);

    loadMeasurer.reset (spec.sampleRate, spec.maxBlockSize);
}

void RetraceProcessor::prepareEngine (SynthQuality quality)
{
    preparedQuality = quality;
    engine.prepare (preparedSampleRate, preparedBlockSize, quality, getParameters());
    setLatencySamples (engine.getLatencyInSamples());
}

void RetraceProcessor::parameterValueChanged (const yup::AudioParameter::Ptr&, int)
{
    const auto quality = synthParameters.getQuality();

    if (preparedSampleRate <= 0.0 || quality == preparedQuality)
        return;

    // The engine reallocates for the new rate, so the audio callback must be out of the way.
    {
        const ScopedProcessSuspension suspension (*this);
        const yup::CriticalSection::ScopedLockType lock (getProcessLock());
        prepareEngine (quality);
    }

    updateHostDisplay (ChangeDetails {}.withLatencyChanged (true));
}

void RetraceProcessor::releaseResources()
{
    engine.allNotesOff (0, false);
    effects.reset();
}

void RetraceProcessor::flush()
{
    engine.allNotesOff (0, false);
    effects.reset();
}

void RetraceProcessor::processBlock (yup::AudioProcessContext<float>& context)
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

    double bpm = 0.0;

    if (context.playHead != nullptr)
        if (const auto position = context.playHead->getPosition())
        {
            bpm = position->getBpm().value_or (0.0);

            if (const auto signature = position->getTimeSignature(); signature.has_value() && signature->numerator > 0 && signature->denominator > 0)
            {
                hostNumerator.store (signature->numerator, std::memory_order_relaxed);
                hostDenominator.store (signature->denominator, std::memory_order_relaxed);
            }
        }

    hostBpm.store (bpm, std::memory_order_relaxed);

    for (int start = 0; start < numSamples; start += SynthExample::controlChunk)
    {
        const auto count = yup::jmin (SynthExample::controlChunk, numSamples - start);

        for (; change != context.params.end() && change->sampleOffset <= start; ++change)
            applyParameterChange (*change);

        engine.renderRange (audio, context.midi, start, count, playedValues, bpm);

        if (audio.getNumChannels() >= 2)
        {
            const auto values = synthParameters.effects.read ({ engine.getGlobalValues (globalValues) });
            effects.process (audio.getWritePointer (0, start), audio.getWritePointer (1, start), count, values, bpm);
        }

        outputGain.setTargetValue (playedValues[static_cast<std::size_t> (volumeIndex)]);

        for (int sample = start; sample < start + count; ++sample)
        {
            const auto gain = outputGain.getNextValue();

            for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            {
                auto& value = audio.getWritePointer (channel)[sample];
                value = masterClippers[static_cast<std::size_t> (yup::jmin (channel, 1))].processSample (value * gain);
            }
        }
    }

    for (; change != context.params.end(); ++change)
        applyParameterChange (*change);

    for (int channel = 0; channel < static_cast<int> (outputPeaks.size()); ++channel)
    {
        const auto source = yup::jmin (channel, audio.getNumChannels() - 1);
        const auto peak = source >= 0 && numSamples > 0 ? audio.getMagnitude (source, 0, numSamples) : 0.0f;
        // Kept as a running maximum until the meter takes it, so no block's peak is missed.
        auto& stored = outputPeaks[static_cast<std::size_t> (channel)];
        auto previous = stored.load (std::memory_order_relaxed);

        while (peak > previous && ! stored.compare_exchange_weak (previous, peak, std::memory_order_relaxed))
        {
        }
    }

    // Whatever is left in the buffer goes back to the host as output events.
    context.midi.clear();
}

void RetraceProcessor::followParameters (const yup::ParameterChangeBuffer& changes) noexcept
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

void RetraceProcessor::applyParameterChange (const yup::ParameterChange& change) noexcept
{
    const auto parameters = getParameters();

    if (! yup::isPositiveAndBelow (change.parameterIndex, static_cast<int> (parameters.size())))
        return;

    const auto index = static_cast<std::size_t> (change.parameterIndex);
    playedValues[index] = parameters[index]->convertToDenormalizedValue (change.normalizedValue);
}

//==============================================================================
int RetraceProcessor::getCurrentPreset() const noexcept
{
    return currentPreset.load();
}

void RetraceProcessor::setCurrentPreset (int index) noexcept
{
    const auto state = SynthPatch::createFactoryState (index);

    if (state.isValid())
        applyState (state, ChangeDetails {}.withProgramChanged (true));
}

int RetraceProcessor::getNumPresets() const
{
    return static_cast<int> (presetNames.size());
}

yup::String RetraceProcessor::getPresetName (int index) const
{
    if (yup::isPositiveAndBelow (index, getNumPresets()))
        return presetNames[static_cast<std::size_t> (index)];

    return {};
}

void RetraceProcessor::setPresetName (int index, yup::StringRef newName)
{
    if (yup::isPositiveAndBelow (index, getNumPresets()))
        presetNames[static_cast<std::size_t> (index)] = newName;
}

SynthTransport RetraceProcessor::getTransport() const noexcept
{
    return { hostBpm.load (std::memory_order_relaxed), hostNumerator.load (std::memory_order_relaxed), hostDenominator.load (std::memory_order_relaxed) };
}

void RetraceProcessor::randomize()
{
    applyState (SynthPatch::createRandomState (synthParameters, yup::Random::getSystemRandom(), getPresetInfo().macroNames), {});
}

void RetraceProcessor::randomizeSlightly()
{
    auto info = getPresetInfo();

    // A varied factory preset is no longer the factory's.
    if (currentPreset.load() >= 0)
        info.bank = info.author = {};

    auto state = SynthPatch::createState (getParameters(), getPatchState(), -1, info);
    SynthPatch::varyState (state, synthParameters, yup::Random::getSystemRandom());
    applyState (state, {});
}

//==============================================================================
yup::Result RetraceProcessor::loadStateFromDataTree (const yup::DataTree& state)
{
    if (auto result = applyState (state, {}); result.failed())
        return result;

    if (const auto name = state.getProperty (themeProperty, {}).toString(); name.isNotEmpty())
    {
        const yup::SpinLock::ScopedLockType lock (themeNameLock);
        themeName = name;
    }

    // Only remembered, never read: a file that is gone simply is not listed by the browser.
    if (const auto path = state.getProperty (presetFileProperty, {}).toString(); yup::File::isAbsolutePath (path))
    {
        const yup::SpinLock::ScopedLockType lock (presetInfoLock);
        currentPresetFile = yup::File (path);
    }

    return yup::Result::ok();
}

yup::Result RetraceProcessor::saveStateIntoDataTree (yup::DataTree& state)
{
    state = SynthPatch::createState (getParameters(), getPatchState(), currentPreset.load(), getPresetInfo());
    auto transaction = state.beginTransaction();
    transaction.setProperty (themeProperty, getThemeName());

    if (const auto file = getCurrentPresetFile(); file != yup::File())
        transaction.setProperty (presetFileProperty, file.getFullPathName());

    return yup::Result::ok();
}

/** A file keeps the id it is listed with, so its favorite and rating stay with it, and the
    current file keeps the id it was saved with before the library lists it; anything else,
    a factory preset included, gets a new id. */
yup::Result RetraceProcessor::savePresetFile (const yup::File& file)
{
    auto info = getPresetInfo();
    info.name = file.getFileNameWithoutExtension();

    const auto isFactory = currentPreset.load() >= 0;

    // A factory preset saved as a file becomes the user's own.
    if (isFactory)
        info.bank = info.author = {};

    const PresetEntry* listed = nullptr;

    if (presetLibrary != nullptr)
    {
        for (const auto& entry : presetLibrary->getLibrary().getEntries())
        {
            if (! entry.isFactory() && entry.file == file)
                listed = &entry;
        }
    }

    const auto previousKey = listed != nullptr ? listed->key : yup::String();

    if (listed != nullptr && listed->id.isNotEmpty())
        info.id = listed->id;
    else if (listed != nullptr || isFactory || file != getCurrentPresetFile() || info.id.isEmpty())
        info.id = yup::Uuid().toDashedString();

    const auto xml = SynthPatch::createState (getParameters(), getPatchState(), -1, info).createXml();

    if (xml == nullptr || ! xml->writeTo (file))
        return yup::Result::fail ("Cannot write " + file.getFullPathName());

    if (previousKey.isNotEmpty() && previousKey != info.id)
    {
        auto& userData = presetLibrary->getUserData();
        userData.copyKey (previousKey, info.id);
        userData.forgetKey (previousKey);
    }

    currentPreset.store (-1);

    const yup::SpinLock::ScopedLockType lock (presetInfoLock);
    presetInfo = std::move (info);
    currentPresetFile = file;
    return yup::Result::ok();
}

yup::Result RetraceProcessor::loadPresetFile (const yup::File& file)
{
    const auto xml = yup::parseXML (file);

    if (xml == nullptr)
        return yup::Result::fail ("Cannot read " + file.getFullPathName());

    if (auto result = applyState (yup::DataTree::fromXml (*xml), ChangeDetails {}.withProgramChanged (true)); result.failed())
        return result;

    const yup::SpinLock::ScopedLockType lock (presetInfoLock);
    currentPresetFile = file;

    if (presetInfo.name.isEmpty())
        presetInfo.name = file.getFileNameWithoutExtension();

    return yup::Result::ok();
}

SynthPresetLibrary& RetraceProcessor::getPresetLibrary()
{
    if (presetLibrary == nullptr)
    {
        presetLibrary = SynthPresetLibrary::acquire();
        presetLibrary->addFileListener (this);
    }

    return *presetLibrary;
}

PresetBrowseState& RetraceProcessor::getPresetBrowser()
{
    if (presetBrowser == nullptr)
        presetBrowser = std::make_unique<PresetBrowseState> (getPresetLibrary().getLibrary(), getPresetLibrary().getUserData());

    return *presetBrowser;
}

yup::Result RetraceProcessor::loadPresetEntry (const PresetEntry& entry)
{
    if (! entry.isFactory())
        return loadPresetFile (entry.file);

    if (! yup::isPositiveAndBelow (entry.factoryIndex, SynthPatch::getNumFactoryPresets()))
        return yup::Result::fail ("No factory preset " + yup::String (entry.factoryIndex));

    setCurrentPreset (entry.factoryIndex);
    return yup::Result::ok();
}

yup::File RetraceProcessor::getCurrentPresetFile() const
{
    const yup::SpinLock::ScopedLockType lock (presetInfoLock);
    return currentPresetFile;
}

yup::String RetraceProcessor::getCurrentPresetKey() const
{
    if (const auto file = getCurrentPresetFile(); file != yup::File())
    {
        // Keyed as the library keys it: by its id, or by its path when it has none or
        // another file holds the same id.
        const auto id = getPresetInfo().id;
        const auto* listed = id.isNotEmpty() && presetLibrary != nullptr ? presetLibrary->getLibrary().findEntry (id) : nullptr;
        return id.isEmpty() || (listed != nullptr && listed->file != file) ? file.getFullPathName() : id;
    }

    if (const auto index = currentPreset.load(); index >= 0)
        return SynthPatch::getFactoryPresetId (index);

    return {};
}

/** Follows a preset file the browser changed when it is the current one, in whichever
    instance the browser ran: the current file moves with it and the description takes its
    fields, keeping the macro names; a deleted file is only forgotten. */
void RetraceProcessor::presetFileChanged (const yup::File& previousFile, const PresetEntry& entry)
{
    const yup::SpinLock::ScopedLockType lock (presetInfoLock);

    if (previousFile == yup::File() || previousFile != currentPresetFile)
        return;

    currentPresetFile = entry.file;

    if (entry.file == yup::File())
        return;

    presetInfo.name = entry.name;
    presetInfo.category = entry.category;
    presetInfo.author = entry.author;
    presetInfo.bank = entry.bank;
    presetInfo.description = entry.description;
    presetInfo.notes = entry.notes;
    presetInfo.tags = PresetEntry::joinTags (entry.tags);
    presetInfo.id = entry.id;
}

yup::String RetraceProcessor::getThemeName() const
{
    const yup::SpinLock::ScopedLockType lock (themeNameLock);
    return themeName;
}

void RetraceProcessor::setThemeName (const yup::String& newName)
{
    {
        const yup::SpinLock::ScopedLockType lock (themeNameLock);

        if (themeName == newName)
            return;

        themeName = newName;
    }

    updateHostDisplay (ChangeDetails {}.withNonParameterStateChanged (true));
}

SynthPatch::PresetInfo RetraceProcessor::getPresetInfo() const
{
    const yup::SpinLock::ScopedLockType lock (presetInfoLock);
    return presetInfo;
}

void RetraceProcessor::setPresetInfo (SynthPatch::PresetInfo newInfo)
{
    const yup::SpinLock::ScopedLockType lock (presetInfoLock);
    presetInfo = std::move (newInfo);
}

yup::Result RetraceProcessor::applyState (const yup::DataTree& state, ChangeDetails details)
{
    if (auto result = SynthPatch::loadState (state, getParameters(), getPatchState()); result.failed())
        return result;

    currentPreset.store (SynthPatch::getPresetIndex (state));

    {
        const yup::SpinLock::ScopedLockType lock (presetInfoLock);
        presetInfo = SynthPatch::readInfo (state);
        currentPresetFile = yup::File();
    }

    // A patch replaced by a preset, a file or the host leaves nothing to undo into.
    undoManager.clear();

    updateHostDisplay (details.withParameterValuesChanged (true).withNonParameterStateChanged (true));
    return yup::Result::ok();
}

//==============================================================================
yup::AudioProcessorEditor* RetraceProcessor::createEditor()
{
    return new RetraceEditor (*this);
}

//==============================================================================
extern "C" yup::AudioProcessor* createPluginProcessor()
{
    return new RetraceProcessor();
}
