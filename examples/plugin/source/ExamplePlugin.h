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

#include "ExampleSynth.h"

//==============================================================================
/** A small polyphonic instrument plugin.

    It turns incoming MIDI notes into sound with a pool of voices, shaped by four
    automatable parameters: output gain, oscillator waveform, and the attack and
    release of the amplitude envelope. The parameters are saved and restored as a
    DataTree, so the state is readable and forward compatible.

    @see ExampleSynth, ExampleEditor
*/
class ExamplePlugin final : public yup::AudioProcessor
{
public:
    //==============================================================================
    /** Stable host automation IDs, kept forever once shipped. */
    enum ParameterID : yup::uint32
    {
        gainParam = 100,
        waveformParam = 200,
        attackParam = 300,
        releaseParam = 400
    };

    //==============================================================================
    ExamplePlugin();
    ~ExamplePlugin() override = default;

    //==============================================================================
    void prepareToPlay (const yup::AudioSpec& spec) override;
    void releaseResources() override;
    void processBlock (yup::AudioProcessContext<float>& context) override;

    //==============================================================================
    int getNumVoices() const override { return ExampleSynth::numVoices; }

    //==============================================================================
    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    yup::String getPresetName (int) const override { return {}; }

    void setPresetName (int, yup::StringRef) override {}

    //==============================================================================
    bool supportsDataTreeState() const noexcept override { return true; }

    yup::Result loadStateFromDataTree (const yup::DataTree& state) override;
    yup::Result saveStateIntoDataTree (yup::DataTree& state) override;

    //==============================================================================
    bool hasEditor() const override { return true; }

    yup::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    /** Returns the shared keyboard state, played by the host and by the editor's keyboard. */
    yup::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }

private:
    //==============================================================================
    static yup::AudioBusLayout makeBusLayout();
    void updateSynthParameters() noexcept;

    ExampleSynth synth;
    yup::MidiKeyboardState keyboardState;

    yup::AudioParameter::Ptr gain;
    yup::AudioParameter::Ptr waveform;
    yup::AudioParameter::Ptr attack;
    yup::AudioParameter::Ptr release;

    yup::SmoothedValue<float> gainSmoothed;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExamplePlugin)
};
