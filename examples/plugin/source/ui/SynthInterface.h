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

#include "../PrismProcessor.h"
#include "SynthEnvelopePanel.h"
#include "SynthFMPanel.h"
#include "SynthFilterPanel.h"
#include "SynthLFOPanel.h"
#include "SynthModulationPage.h"
#include "SynthOscillatorPanel.h"
#include "SynthPage.h"
#include "SynthPresetBar.h"

#include <array>
#include <memory>
#include <vector>

//==============================================================================
/** The whole PRISM interface: operators, filters, envelopes, LFOs, FM matrix and keyboard.

    Each section shows one of its panels at a time, picked with a row of tabs. The
    interface takes its colors from the theme when it is built, so PrismEditor rebuilds
    it when the theme changes.

    Every control is bound to a parameter of the processor. A timer shows whatever the
    host, a preset or a state load changed, publishes the waveform editors' pending
    edits once per frame. The keyboard and wheels show and play the processor's keyboard
    state, so the host's notes light the keys.

    @see PrismProcessor
*/
class SynthInterface
    : public yup::Component
    , private yup::Timer
{
public:
    explicit SynthInterface (PrismProcessor& processorToEdit);
    ~SynthInterface() override;

    /** Called with the index of a theme picked in the header. */
    std::function<void (int)> onThemeChosen;

    //==============================================================================

    //==============================================================================
    void resized() override;
    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    //==============================================================================
    /** A row of tab buttons showing one of a section's panels. */
    struct TabGroup
    {
        std::vector<std::unique_ptr<yup::ToggleButton>> buttons;
        std::vector<yup::Component*> panels;
        int selected = 0;
    };

    void setupTabs (TabGroup& group, const yup::StringArray& names, std::vector<yup::Component*> panels);
    void selectTab (TabGroup& group, int index);
    void layoutTabbedSection (TabGroup& group, yup::Rectangle<float> area);

    void timerCallback() override;

    void layoutMainPage();
    void syncFromProcessor();

    //==============================================================================
    static constexpr float outerInset = 10.0f;
    static constexpr float headerHeight = 60.0f;
    static constexpr float spacing = 8.0f;
    static constexpr float buttonInset = 12.0f;
    static constexpr float actionButtonWidth = 120.0f;
    static constexpr float logoAspect = 236.0f / 122.0f;
    static constexpr float wheelWidth = 28.0f;
    static constexpr float keyboardHeight = 72.0f;
    static constexpr float shapingRowHeight = 170.0f;
    static constexpr float tabHeight = 22.0f;

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
    std::array<std::unique_ptr<SynthFilterPanel>, SynthExample::filterCount> filterPanels;
    std::array<std::unique_ptr<SynthEnvelopePanel>, SynthExample::envelopeCount> envelopePanels;
    std::array<std::unique_ptr<SynthLFOPanel>, SynthExample::lfoCount> lfoPanels;
    std::unique_ptr<SynthFMPanel> fmPanel;
    std::unique_ptr<SynthModulationPage> modulationPage;
    TabGroup matrixTabs;
    TabGroup oscillatorTabs;
    TabGroup filterTabs;
    TabGroup envelopeTabs;
    TabGroup lfoTabs;

    yup::TextButton randomizeButton { "RANDOMIZE" };
    yup::TextButton clearButton { "ALL NOTES OFF" };
    std::unique_ptr<ParameterKnob> volumeKnob;
    std::unique_ptr<ParameterChoice> modeChoice;
    std::unique_ptr<ParameterKnob> glideKnob;
    std::unique_ptr<ParameterChoice> qualityChoice;
    std::unique_ptr<ParameterChoice> routingChoice;
    std::unique_ptr<ParameterKnob> unisonKnob;
    std::unique_ptr<ParameterKnob> unisonDetuneKnob;
    std::unique_ptr<ParameterKnob> unisonSpreadKnob;
    std::unique_ptr<SynthPresetBar> presetBar;
    yup::ComboBox themeBox;
};
