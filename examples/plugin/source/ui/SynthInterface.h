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

#include "../RetraceProcessor.h"
#include "ParameterSegmented.h"
#include "SynthEnvelopePanel.h"
#include "SynthFMPanel.h"
#include "SynthFilterPanel.h"
#include "SynthLFOPanel.h"
#include "SynthModulationPage.h"
#include "SynthOscillatorPanel.h"
#include "SynthPage.h"
#include "SynthHeader.h"
#include "SynthPresetBrowser.h"
#include "SynthFieldEditor.h"
#include "SynthMacroPanel.h"
#include "SynthModulationHandle.h"
#include "SynthSection.h"
#include "SynthUndoHistory.h"
#include "SynthVoicingPanel.h"

#include <array>
#include <memory>
#include <optional>
#include <vector>

//==============================================================================
/** The whole RETRACE interface: operators, filters, envelopes, LFOs, FM matrix and keyboard.

    Each section shows one of its panels at a time, picked with a row of tabs. The
    interface takes its colors from the theme when it is built, so RetraceEditor rebuilds
    it when the theme changes.

    Every control is bound to a parameter of the processor. A timer shows whatever the
    host, a preset or a state load changed, publishes the waveform editors' pending
    edits once per frame. The keyboard and wheels show and play the processor's keyboard
    state, so the host's notes light the keys.

    An envelope's or LFO's modulation handle dragged over the interface lights every
    control it can be routed into, and the one under the mouse more brightly; dropped
    there, it takes a free route slot at +25 % depth, as one undoable step.

    @see RetraceProcessor
*/
class SynthInterface
    : public yup::Component
    , public UndoHistoryOwner
    , public yup::DragAndDropTarget
    , private yup::Timer
{
public:
    /** Builds the interface in a theme, see SynthThemes::getThemes(). */
    SynthInterface (RetraceProcessor& processorToEdit, int themeToWear);
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
    /** Shows the preset browser over the instrument's panels, or hides it again. The
        header, the wheels, the keyboard and the voicing stay visible and playable.
    */
    void setBrowserOpen (bool shouldBeOpen);

    /** True while the preset browser is shown. */
    bool isBrowserOpen() const noexcept { return presetBrowser->isVisible(); }

    //==============================================================================
    /** Returns the processor's undo history, which every control in the interface records into. */
    yup::UndoManager& getUndoHistory() override { return processor.getUndoManager(); }

    //==============================================================================
    void resized() override;
    void paint (yup::Graphics& g) override;
    void paintOverChildren (yup::Graphics& g) override;
    bool keyDown (const yup::KeyPress& key, const yup::Point<float>& position) override;
    bool keyUp (const yup::KeyPress& key, const yup::Point<float>& position) override;
    void attachedToNative() override;

    //==============================================================================
    bool isInterestedInDragSource (const yup::DragAndDropSourceDetails& details) override;
    void itemDragEnter (const yup::DragAndDropSourceDetails& details) override;
    void itemDragMove (const yup::DragAndDropSourceDetails& details) override;
    void itemDragExit (const yup::DragAndDropSourceDetails& details) override;
    bool itemDropped (const yup::DragAndDropSourceDetails& details) override;

private:
    void timerCallback() override;

    void showSettingsMenu (yup::Point<float> position);
    void applyFrameRate();
    void applyUndoSetting();

    void layoutMainPage();
    void syncFromProcessor();
    void updateModulation();

    std::optional<SynthModulationSource> getDraggedSource() const;
    bool canRoute (SynthModulationSource source, const SynthModulationTarget& target) const;
    SynthModulationTarget* findTargetAt (yup::Point<float> position) const;
    std::vector<std::pair<SynthModulationTarget*, yup::Rectangle<float>>> getDropAreas() const;
    void hoverDrag (yup::Point<float> position);
    void clearDrag();

    //==============================================================================
    static constexpr float wheelWidth = 30.0f;
    static constexpr float wheelsTileWidth = 100.0f;
    static constexpr float wheelCaptionHeight = 16.0f;
    static constexpr float firstRowShare = 0.6f;
    static constexpr float filterShare = 384.0f / 790.0f;
    static constexpr float macroColumnWidth = 128.0f;
    static constexpr float initialDepth = 0.25f;

    //==============================================================================
    RetraceProcessor& processor;
    const int theme;

    std::unique_ptr<SynthHeader> header;

    yup::MidiKeyboardComponent keyboardComponent;
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
    std::unique_ptr<SynthMacroPanel> macroPanel;
    std::unique_ptr<ParameterSegmented> routingSelector;
    std::unique_ptr<yup::Label> matrixCaption;
    std::unique_ptr<yup::Label> ampCaption;
    std::vector<std::unique_ptr<ParameterLed>> lfoSyncLeds;
    std::unique_ptr<SynthSection> oscillatorSection;
    std::unique_ptr<SynthSection> matrixSection;
    std::unique_ptr<SynthSection> macroSection;
    std::unique_ptr<SynthSection> filterSection;
    std::unique_ptr<SynthSection> envelopeSection;
    std::unique_ptr<SynthSection> lfoSection;

    std::unique_ptr<SynthVoicingPanel> voicingPanel;
    std::unique_ptr<SynthPresetBrowser> presetBrowser;

    std::vector<SynthModulationTarget*> modulationTargets;
    std::optional<SynthModulationSource> dragSource;
    SynthModulationTarget* dropTarget = nullptr;
    std::vector<std::vector<SynthKnob::ModulationArc>> arcsByParameter;
    std::vector<float> modulatedValues;
    bool modulationLive = false;
    int appliedFrameRate = -1;
    yup::Rectangle<float> wheelsTile;
    yup::Rectangle<float> keyboardTile;
};
