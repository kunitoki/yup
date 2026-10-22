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

#pragma once

#include <yup_audio_processors/yup_audio_processors.h>

#include "dsp/HarmonicSynthEngine.h"
#include "dsp/MidiMessageFifo.h"
#include "dsp/SynthPatch.h"

#include <array>
#include <atomic>
#include <vector>

//==============================================================================
/** PRISM, a two oscillator spectral synthesiser, as an audio plugin.

    Every control is an AudioParameter with a stable host ID; the drawn partials of
    each oscillator are state saved alongside them. Session state, factory presets and
    user preset files share one DataTree format and one apply path, SynthPatch.

    Host modulation (CLAP parameter modulation) never reaches the parameters'
    own values, so the processor keeps a table of the values it actually plays. The
    table follows the parameters, keeps any modulation offset on top, and takes the
    block's parameter events at the start of each control chunk of the block.

    @see PrismEditor, SynthParameters, SynthPatch, HarmonicSynthEngine
*/
class PrismProcessor : public yup::AudioProcessor
{
public:
    PrismProcessor();
    ~PrismProcessor() override;

    //==============================================================================
    void prepareToPlay (const yup::AudioSpec& spec) override;
    void releaseResources() override;
    void processBlock (yup::AudioProcessContext<float>& context) override;
    void flush() override;

    int getNumVoices() const override { return SynthExample::voiceCount; }

    //==============================================================================
    int getCurrentPreset() const noexcept override;
    void setCurrentPreset (int index) noexcept override;
    int getNumPresets() const override;
    yup::String getPresetName (int index) const override;
    void setPresetName (int index, yup::StringRef newName) override;

    //==============================================================================
    bool supportsDataTreeState() const noexcept override { return true; }
    yup::Result loadStateFromDataTree (const yup::DataTree& state) override;
    yup::Result saveStateIntoDataTree (yup::DataTree& state) override;

    //==============================================================================
    bool hasEditor() const override { return true; }
    yup::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    /** Returns the parameters, grouped by section, for the editor to bind to. */
    const SynthParameters& getSynthParameters() const noexcept { return synthParameters; }

    /** Returns the drawn partials of one oscillator, which the waveform editor edits. */
    SynthPartials& getPartials (int oscillatorIndex) noexcept { return partials[static_cast<std::size_t> (oscillatorIndex)]; }

    /** Returns the waveform presets the oscillators play. */
    const SynthOscillatorResources& getResources() const noexcept { return engine.getResources(); }

    /** Replaces the patch with a random one, keeping volume, voice mode and glide. Message thread. */
    void randomize();

    /** Writes the current patch to a user preset file, in the session state format. Message thread. */
    yup::Result savePresetFile (const yup::File& file);

    /** Loads a user preset file written by savePresetFile(). Message thread. */
    yup::Result loadPresetFile (const yup::File& file);

    /** Releases every sounding note at the start of the next block. */
    void requestAllNotesOff() noexcept { engine.requestAllNotesOff(); }

    //==============================================================================
    /** Returns the keys and wheels the host and the on-screen keyboard hold.

        Every block feeds the host's MIDI into it and plays the notes the editor's keyboard
        added, so any number of editors can show and play it at once.
    */
    yup::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }

    /** Queues a pitch bend or controller from the editor's wheels for the next block. Message thread. */
    bool sendMidiFromEditor (const yup::MidiMessage& message) noexcept { return editorMidi.push (message); }

    //==============================================================================
    /** Returns the phase of an LFO in the most recently started voice. */
    float getLFOPhase (int lfoIndex) const noexcept { return engine.getLFOPhase (lfoIndex); }

    /** Returns how many voices are sounding. */
    int getNumActiveVoices() const noexcept { return engine.getNumActiveVoices(); }

    /** Returns the share of the block's time the last blocks took, in percent. */
    double getLoadPercentage() const { return loadMeasurer.getLoadAsPercentage(); }

    /** Returns how many blocks took longer than their duration. */
    int getXRunCount() const { return loadMeasurer.getXRunCount(); }

    /** Copies the latest rendered block for the oscilloscope. Returns false when there is none new. */
    bool readScope (std::vector<float>& destination);

private:
    //==============================================================================
    yup::Result applyState (const yup::DataTree& state, ChangeDetails details);
    void followParameters (const yup::ParameterChangeBuffer& changes) noexcept;
    void applyParameterChange (const yup::ParameterChange& change) noexcept;

    //==============================================================================
    SynthParameters synthParameters;
    SynthPatch::Partials partials;
    HarmonicSynthEngine engine;

    std::vector<float> playedValues;     // what the engine plays, host modulation included
    std::vector<float> parameterValues;  // the parameter values playedValues last followed
    std::vector<char> changedByEvents;
    int volumeIndex = 0;
    yup::SmoothedValue<float> outputGain;

    yup::MidiKeyboardState keyboardState;
    MidiMessageFifo editorMidi { 1024 };

    yup::AudioProcessLoadMeasurer loadMeasurer;
    std::array<float, SynthExample::maxBlockSize> scopeSamples {};
    std::atomic<bool> scopeReady { false };
    int scopeCount = 0;

    std::vector<yup::String> presetNames;
    std::atomic<int> currentPreset { 0 };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrismProcessor)
};
