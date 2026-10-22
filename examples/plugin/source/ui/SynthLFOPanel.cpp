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

#include "SynthLFOPanel.h"
#include "SynthTheme.h"

SynthLFOPanel::SynthLFOPanel (const yup::String& panelTitle,
                              const SynthLFOParameters& parametersToEdit,
                              const SynthCurveState& curvesToShow,
                              int lfoIndex,
                              const yup::Font& font)
    : parameters (parametersToEdit)
    , curves (curvesToShow)
    , index (lfoIndex)
    , shownValues (parametersToEdit.read())
    , syncButton ("SYNC", parametersToEdit.tempoSync)
    , shapeChoice ("SHAPE", parametersToEdit.shape, font)
    , divisionChoice ("DIVISION", parametersToEdit.division, font)
    , retriggerChoice ("TRIGGER", parametersToEdit.retrigger, font)
    , rateKnob ("RATE", parametersToEdit.rate, font)
    , phaseKnob ("PHASE", parametersToEdit.phase, font)
    , smoothKnob ("SMOOTH", parametersToEdit.smoothing, font)
    , depthKnob ("DEPTH", parametersToEdit.depth, font)
{
    setupPanelTitle (titleLabel, panelTitle, font);
    addAndMakeVisible (titleLabel);

    addAndMakeVisible (syncButton);

    display.setValues (shownValues);
    addAndMakeVisible (display);

    for (auto* choice : { &shapeChoice, &divisionChoice, &retriggerChoice })
        addAndMakeVisible (*choice);

    for (auto* knob : { &rateKnob, &phaseKnob, &smoothKnob, &depthKnob })
        addAndMakeVisible (*knob);

    syncFromParameters();
}

void SynthLFOPanel::syncFromParameters()
{
    syncButton.syncFromParameter();

    for (auto* choice : { &shapeChoice, &divisionChoice, &retriggerChoice })
        choice->syncFromParameter();

    for (auto* knob : { &rateKnob, &phaseKnob, &smoothKnob, &depthKnob })
        knob->syncFromParameter();

    if (const auto values = parameters.read(); values != shownValues)
    {
        shownValues = values;
        display.setValues (values);
    }

    if (const auto generation = curves.getGeneration(); generation != shownGeneration)
    {
        shownGeneration = generation;
        display.setCurve (curves.getLFO (index));
    }

    rateKnob.setEnabled (! shownValues.tempoSync);
    divisionChoice.setEnabled (shownValues.tempoSync);
}

void SynthLFOPanel::resized()
{
    auto bounds = getLocalBounds().reduced (panelInset);

    auto header = bounds.removeFromTop (headerHeight);
    syncButton.setBounds (header.removeFromRight (buttonWidth));
    titleLabel.setBounds (header);
    bounds.removeFromTop (spacing);

    auto knobs = bounds.removeFromBottom (yup::jmin (58.0f, bounds.getHeight() * 0.5f));
    bounds.removeFromBottom (spacing);

    auto choices = bounds.removeFromRight (choiceWidth);
    bounds.removeFromRight (spacing);
    display.setBounds (bounds);

    const auto choiceRow = choices.getHeight() / 3.0f;

    for (auto* choice : { &shapeChoice, &divisionChoice, &retriggerChoice })
        choice->setBounds (choices.removeFromTop (choiceRow));

    layoutControlsInRow (knobs, { &rateKnob, &phaseKnob, &smoothKnob, &depthKnob });
}

void SynthLFOPanel::paint (yup::Graphics& g)
{
    paintSynthPanel (g, getLocalBounds());
}
