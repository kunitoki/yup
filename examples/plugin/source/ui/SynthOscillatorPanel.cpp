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

#include "SynthOscillatorPanel.h"
#include "SynthTheme.h"

SynthOscillatorPanel::SynthOscillatorPanel (const yup::String& panelTitle,
                                            const SynthOscillatorParameters& parametersToEdit,
                                            SynthPartials& partialsToEdit,
                                            const SynthOscillatorResources& resources,
                                            std::shared_ptr<SynthWaveformShader> waveformShader,
                                            const yup::Font& font)
    : parameters (parametersToEdit)
    , partials (partialsToEdit)
    , editor (parametersToEdit, partialsToEdit, resources, std::move (waveformShader))
    , waveformChoice ("WAVEFORM", parametersToEdit.waveform, font)
    , syncModeChoice ("SYNC", parametersToEdit.syncMode, font)
    , levelKnob ("LEVEL", parametersToEdit.level, font)
    , octaveKnob ("OCTAVE", parametersToEdit.octave, font)
    , detuneKnob ("CENTS", parametersToEdit.detune, font)
    , ridgesKnob ("RIDGES", parametersToEdit.ridgeSpacing, font)
    , colorKnob ("COLOR", parametersToEdit.color, font)
    , dispersionKnob ("DISPERSION", parametersToEdit.dispersion, font)
    , squeezeKnob ("SQUEEZE", parametersToEdit.squeeze, font)
    , squashKnob ("SQUASH", parametersToEdit.squash, font)
    , tiltKnob ("TILT", parametersToEdit.tilt, font)
    , oddEvenKnob ("ODD/EVEN", parametersToEdit.oddEven, font)
    , formantKnob ("FORMANT", parametersToEdit.formant, font)
    , formantPositionKnob ("F.POS", parametersToEdit.formantPosition, font)
    , scatterKnob ("SCATTER", parametersToEdit.scatter, font)
    , syncRatioKnob ("SYNC RATIO", parametersToEdit.syncRatio, font)
    , unisonKnob ("UNISON", parametersToEdit.unisonVoices, font)
    , unisonDetuneKnob ("U.DETUNE", parametersToEdit.unisonDetune, font)
    , spreadKnob ("SPREAD", parametersToEdit.unisonSpread, font)
{
    setupPanelTitle (titleLabel, panelTitle, font);
    addAndMakeVisible (titleLabel);

    partialsButton.setButtonText ("PARTIALS");
    SynthTheme::styleToggle (partialsButton);
    partialsButton.onClick = [this] { editor.setEditingPartials (partialsButton.getToggleState()); };
    addAndMakeVisible (partialsButton);

    resetButton.setColor (yup::TextButton::Style::backgroundColorId, SynthTheme::displayBackground());
    resetButton.setColor (yup::TextButton::Style::textColorId, SynthTheme::textSecondary());
    resetButton.setColor (yup::TextButton::Style::outlineColorId, SynthTheme::panelBorder());
    resetButton.onClick = [this] { editor.revertToPreset(); };
    addAndMakeVisible (resetButton);

    addAndMakeVisible (editor);

    // Picking a preset drops any edited partials, otherwise the oscillator would keep
    // playing the edited shape while the combo box claims something else.
    waveformChoice.onChange = [this] { editor.revertToPreset(); };

    for (auto* choice : { &waveformChoice, &syncModeChoice })
        addAndMakeVisible (*choice);

    for (auto* knob : getKnobs())
        addAndMakeVisible (*knob);

    shownValues = parameters.read();
    partials.fillValues (shownValues);
    syncRatioKnob.setEnabled (shownValues.syncMode != yup::SyncMode::none);
}

void SynthOscillatorPanel::syncFromParameters()
{
    waveformChoice.syncFromParameter();
    syncModeChoice.syncFromParameter();

    for (auto* knob : getKnobs())
        knob->syncFromParameter();

    // One generation bump per frame, however many mouse events the drag produced.
    editor.commitPendingEdits();

    auto values = parameters.read();
    partials.fillValues (values);

    if (values == shownValues)
        return;

    shownValues = values;
    editor.refresh();

    // Greys out the ratio knob while no sync mode reads it.
    syncRatioKnob.setEnabled (values.syncMode != yup::SyncMode::none);
}

void SynthOscillatorPanel::resized()
{
    auto bounds = getLocalBounds().reduced (panelInset);

    auto header = bounds.removeFromTop (headerHeight);
    resetButton.setBounds (header.removeFromRight (buttonWidth));
    header.removeFromRight (spacing);
    partialsButton.setBounds (header.removeFromRight (buttonWidth));
    titleLabel.setBounds (header);

    bounds.removeFromTop (spacing);

    auto knobArea = bounds.removeFromBottom (knobRowHeight * 3.0f + spacing * 2.0f);
    bounds.removeFromBottom (spacing);

    auto choiceArea = bounds.removeFromBottom (choiceRowHeight);
    bounds.removeFromBottom (spacing);

    editor.setBounds (bounds);

    layoutControlsInRow (choiceArea, { &waveformChoice, &syncModeChoice });

    layoutControlsInRow (knobArea.removeFromTop (knobRowHeight),
                         { &levelKnob, &octaveKnob, &detuneKnob, &ridgesKnob, &colorKnob, &dispersionKnob });

    knobArea.removeFromTop (spacing);

    layoutControlsInRow (knobArea.removeFromTop (knobRowHeight),
                         { &squeezeKnob, &squashKnob, &tiltKnob, &oddEvenKnob, &formantKnob, &formantPositionKnob });

    knobArea.removeFromTop (spacing);

    layoutControlsInRow (knobArea,
                         { &scatterKnob, &syncRatioKnob, &unisonKnob, &unisonDetuneKnob, &spreadKnob });
}

void SynthOscillatorPanel::paint (yup::Graphics& g)
{
    paintSynthPanel (g, getLocalBounds());
}

std::vector<ParameterKnob*> SynthOscillatorPanel::getKnobs()
{
    return { &levelKnob, &octaveKnob, &detuneKnob, &ridgesKnob, &colorKnob, &dispersionKnob,
             &squeezeKnob, &squashKnob, &tiltKnob, &oddEvenKnob, &formantKnob,
             &formantPositionKnob, &scatterKnob, &syncRatioKnob,
             &unisonKnob, &unisonDetuneKnob, &spreadKnob };
}
