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
#include "ParameterSegmented.h"
#include "SynthEnvelopePanel.h"
#include "SynthFMPanel.h"
#include "SynthFilterPanel.h"
#include "SynthLFOPanel.h"
#include "SynthModulationPage.h"
#include "SynthOscillatorPanel.h"
#include "SynthPage.h"
#include "SynthHeader.h"
#include "SynthKeyboard.h"
#include "SynthSection.h"
#include "SynthUndoHistory.h"
#include "SynthVoicingPanel.h"

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
    , public UndoHistoryOwner
    , private yup::Timer
{
public:
    explicit SynthInterface (PrismProcessor& processorToEdit);
    ~SynthInterface() override;

    //==============================================================================
    /** Called with the index of a theme picked in the settings menu. */
    std::function<void (int)> onThemeChosen;

    //==============================================================================
    /** Called when the post effect settings change; createPostEffect() builds the new one. */
    std::function<void()> onPostEffectChanged;

    /** Returns the post effect the settings ask for, or nullptr when it is off.

        The effect belongs on the component the interface fills, not on the interface, so
        it renders at the size the interface is shown at whatever its scale.
    */
    static yup::ComponentEffect::Ptr createPostEffect();

    //==============================================================================
    /** Returns the processor's undo history, which every control in the interface records into. */
    yup::UndoManager& getUndoHistory() override { return processor.getUndoManager(); }

    //==============================================================================
    void resized() override;
    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    void timerCallback() override;

    void showSettingsMenu();
    void applyFrameRate();
    void applyUndoSetting();

    void layoutMainPage();
    void syncFromProcessor();
    void updateModulation();

    //==============================================================================
    static constexpr float wheelWidth = 30.0f;
    static constexpr float wheelsTileWidth = 100.0f;
    static constexpr float wheelCaptionHeight = 16.0f;
    static constexpr float firstRowShare = 0.6f;
    static constexpr float filterShare = 384.0f / 790.0f;

    //==============================================================================
    PrismProcessor& processor;

    std::unique_ptr<SynthHeader> header;

    SynthKeyboard keyboardComponent;
    yup::PitchWheelComponent pitchWheelComponent;
    yup::ModWheelComponent modWheelComponent;


    SynthPage mainPage;
    std::shared_ptr<SynthWaveformShader> waveformShader = std::make_shared<SynthWaveformShader>();
    std::array<std::unique_ptr<SynthOscillatorPanel>, SynthExample::oscillatorCount> oscillatorPanels;
    std::array<std::unique_ptr<SynthFilterPanel>, SynthExample::filterCount> filterPanels;
    std::array<std::unique_ptr<SynthEnvelopePanel>, SynthExample::envelopeCount> envelopePanels;
    std::array<std::unique_ptr<SynthLFOPanel>, SynthExample::lfoCount> lfoPanels;
    std::unique_ptr<SynthFMPanel> fmPanel;
    std::unique_ptr<SynthModulationPage> modulationPage;
    std::unique_ptr<ParameterSegmented> routingSelector;
    std::unique_ptr<yup::Label> matrixCaption;
    std::unique_ptr<yup::Label> ampCaption;
    std::vector<std::unique_ptr<ParameterLed>> lfoSyncLeds;
    std::unique_ptr<SynthSection> oscillatorSection;
    std::unique_ptr<SynthSection> matrixSection;
    std::unique_ptr<SynthSection> filterSection;
    std::unique_ptr<SynthSection> envelopeSection;
    std::unique_ptr<SynthSection> lfoSection;

    std::unique_ptr<SynthVoicingPanel> voicingPanel;

    std::vector<SynthModulationTarget*> modulationTargets;
    std::vector<std::vector<SynthKnob::ModulationArc>> arcsByParameter;
    std::vector<float> modulatedValues;
    bool modulationLive = false;
    int appliedFrameRate = -1;
    yup::Rectangle<float> wheelsTile;
    yup::Rectangle<float> keyboardTile;
};
