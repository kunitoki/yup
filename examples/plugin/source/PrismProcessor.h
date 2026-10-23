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
/** PRISM, a four operator spectral FM synthesiser, as an audio plugin.

    Every control is an AudioParameter with a stable host ID; the drawn partials of
    each oscillator, the envelope and LFO curves and the modulation routes are state
    saved alongside them. Session state, factory presets and user preset files share
    one DataTree format and one apply path, SynthPatch.

    The operator network runs oversampled, which delays the output by a constant that
    depends on the quality setting and is reported to the host. Changing the quality
    re-prepares the engine with processing suspended and reports the new latency.

    Host modulation (CLAP parameter modulation) never reaches the parameters'
    own values, so the processor keeps a table of the values it actually plays. The
    table follows the parameters, keeps any modulation offset on top, and takes the
    block's parameter events at the start of each control chunk of the block.

    @see PrismEditor, SynthParameters, SynthPatch, HarmonicSynthEngine
*/
class PrismProcessor
    : public yup::AudioProcessor
    , private yup::AudioParameter::Listener
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

    /** Returns the envelope and LFO curves. */
    SynthCurveState& getCurves() noexcept { return curves; }

    /** Returns the modulation routes. */
    SynthRoutes& getRoutes() noexcept { return routes; }

    /** Returns the waveform presets the oscillators play. */
    const SynthOscillatorResources& getResources() const noexcept { return engine.getResources(); }

    /** Replaces the patch with a random one, keeping volume, voice mode and glide. Message thread. */
    void randomize();

    /** Writes the current patch to a user preset file, in the session state format. Message thread. */
    yup::Result savePresetFile (const yup::File& file);

    /** Loads a user preset file written by savePresetFile(). Message thread. */
    yup::Result loadPresetFile (const yup::File& file);

    /** Returns the description of the current patch: its name, bank, author and tags. */
    SynthPatch::PresetInfo getPresetInfo() const;

    /** Replaces the description of the current patch, saved with it from now on. Message thread. */
    void setPresetInfo (SynthPatch::PresetInfo newInfo);

    /** Returns the history of the edits made in the editor, kept here so it outlives the
        editor. Replacing the patch clears it. Message thread.
    */
    yup::UndoManager& getUndoManager() noexcept { return undoManager; }

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
    /** Returns the phase of an LFO in the most recently started voice, or -1 when no note plays. */
    float getLFOPhase (int lfoIndex) const noexcept { return engine.getLFOPhase (lfoIndex); }

    /** Returns how far a route slot moves its destination in the most recent note, in the
        destination's normalized range, or 0 when no note plays. */
    float getRouteAmount (int slot) const noexcept { return engine.getRouteAmount (slot); }

    /** Returns where an envelope of the most recent note is along its points (see yup::CurveEnvelope::getPointPosition()), or -1. */
    float getEnvelopePosition (int envelopeIndex) const noexcept { return engine.getEnvelopePosition (envelopeIndex); }

    /** Returns how many voices are sounding. */
    int getNumActiveVoices() const noexcept { return engine.getNumActiveVoices(); }

    /** Returns how many sounding voices run their operators oversampled. */
    int getNumOversamplingVoices() const noexcept { return engine.getNumOversamplingVoices(); }

    /** Returns the oversampling factor the engine was prepared with. */
    int getOversamplingFactor() const noexcept { return ::getOversamplingFactor (preparedQuality); }

    /** Returns the share of the block's time the last blocks took, in percent. */
    double getLoadPercentage() const { return loadMeasurer.getLoadAsPercentage(); }

    /** Returns how many blocks took longer than their duration. */
    int getXRunCount() const { return loadMeasurer.getXRunCount(); }

    /** Returns the absolute peak of the last block on an output channel, 0 or 1, linear. */
    float getOutputPeak (int channel) const noexcept
    {
        return yup::isPositiveAndBelow (channel, static_cast<int> (outputPeaks.size())) ? outputPeaks[static_cast<std::size_t> (channel)].load (std::memory_order_relaxed) : 0.0f;
    }


private:
    //==============================================================================
    yup::Result applyState (const yup::DataTree& state, ChangeDetails details);
    SynthPatch::State getPatchState() noexcept { return { partials, curves, routes }; }
    void prepareEngine (SynthQuality quality);

    void parameterValueChanged (const yup::AudioParameter::Ptr& parameter, int indexInContainer) override;
    void parameterGestureBegin (const yup::AudioParameter::Ptr&, int) override {}
    void parameterGestureEnd (const yup::AudioParameter::Ptr&, int) override {}
    void followParameters (const yup::ParameterChangeBuffer& changes) noexcept;
    void applyParameterChange (const yup::ParameterChange& change) noexcept;

    //==============================================================================
    SynthParameters synthParameters;
    SynthPatch::Partials partials;
    SynthCurveState curves;
    SynthRoutes routes;
    HarmonicSynthEngine engine;
    std::array<yup::SoftClipper<float>, 2> masterClippers;
    SynthQuality preparedQuality = SynthQuality::x4;
    double preparedSampleRate = 0.0;
    int preparedBlockSize = 0;

    std::vector<float> playedValues;     // what the engine plays, host modulation included
    std::vector<float> parameterValues;  // the parameter values playedValues last followed
    std::vector<char> changedByEvents;
    int volumeIndex = 0;
    yup::SmoothedValue<float> outputGain;

    yup::MidiKeyboardState keyboardState;
    MidiMessageFifo editorMidi { 1024 };

    yup::AudioProcessLoadMeasurer loadMeasurer;
    std::array<std::atomic<float>, 2> outputPeaks {};

    std::vector<yup::String> presetNames;
    std::atomic<int> currentPreset { 0 };
    SynthPatch::PresetInfo presetInfo;
    mutable yup::SpinLock presetInfoLock;

    // Grouped by gesture rather than by time: every edit is its own step.
    yup::UndoManager undoManager { yup::RelativeTime() };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrismProcessor)
};
