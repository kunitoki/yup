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

#include <yup_audio_gui/yup_audio_gui.h>

#include "PrismProcessor.h"
#include "ui/Oscilloscope.h"
#include "ui/SynthEnvelopePanel.h"
#include "ui/SynthFilterPanel.h"
#include "ui/SynthLFOPanel.h"
#include "ui/SynthModulationPage.h"
#include "ui/SynthOscillatorPanel.h"
#include "ui/SynthPage.h"
#include "ui/SynthPresetBar.h"

#include <array>
#include <memory>

//==============================================================================
/** The PRISM instrument: oscillators, filter, envelopes, LFOs, modulation matrix and keyboard.

    Every control is bound to a parameter of the processor. A timer shows whatever the
    host, a preset or a state load changed, publishes the waveform editors' pending
    edits once per frame. The keyboard and wheels show and play the processor's keyboard
    state, so the host's notes light the keys.

    @see PrismProcessor
*/
class PrismEditor
    : public yup::AudioProcessorEditor
    , private yup::Timer
{
public:
    explicit PrismEditor (PrismProcessor& processorToEdit);
    ~PrismEditor() override;

    //==============================================================================
    bool isResizable() const override { return true; }
    yup::Size<int> getPreferredSize() const override { return { 1280, 900 }; }

    //==============================================================================
    void resized() override;
    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    //==============================================================================
    void timerCallback() override;

    void layoutMainPage();
    void showModulationPage (bool show);
    void syncFromProcessor();

    //==============================================================================
    static constexpr float outerInset = 10.0f;
    static constexpr float headerHeight = 60.0f;
    static constexpr float spacing = 8.0f;
    static constexpr float buttonInset = 12.0f;
    static constexpr float pageButtonWidth = 76.0f;
    static constexpr float actionButtonWidth = 120.0f;
    static constexpr float logoAspect = 236.0f / 122.0f;
    static constexpr float wheelWidth = 28.0f;
    static constexpr float keyboardHeight = 72.0f;
    static constexpr float lfoRowHeight = 104.0f;
    static constexpr float shapingRowHeight = 150.0f;

    //==============================================================================
    PrismProcessor& processor;

    yup::MidiKeyboardComponent keyboardComponent;
    yup::PitchWheelComponent pitchWheelComponent;
    yup::ModWheelComponent modWheelComponent;

    yup::Drawable logo;
    yup::Rectangle<float> logoArea;
    yup::Label titleLabel;
    yup::Label subtitleLabel;
    yup::Label voiceLabel;
    yup::Label loadLabel;

    SynthPage mainPage;
    std::shared_ptr<SynthWaveformShader> waveformShader = std::make_shared<SynthWaveformShader>();
    std::array<std::unique_ptr<SynthOscillatorPanel>, SynthExample::oscillatorCount> oscillatorPanels;
    std::unique_ptr<SynthFilterPanel> filterPanel;
    std::array<std::unique_ptr<SynthEnvelopePanel>, SynthExample::envelopeCount> envelopePanels;
    std::array<std::unique_ptr<SynthLFOPanel>, SynthExample::lfoCount> lfoPanels;
    std::unique_ptr<SynthModulationPage> modulationPage;
    yup::ToggleButton mainPageButton;
    yup::ToggleButton modulationPageButton;

    yup::TextButton randomizeButton { "RANDOMIZE" };
    yup::TextButton clearButton { "ALL NOTES OFF" };
    std::unique_ptr<ParameterKnob> volumeKnob;
    std::unique_ptr<ParameterChoice> modeChoice;
    std::unique_ptr<ParameterKnob> glideKnob;
    std::unique_ptr<SynthPresetBar> presetBar;
    Oscilloscope oscilloscope;
};
