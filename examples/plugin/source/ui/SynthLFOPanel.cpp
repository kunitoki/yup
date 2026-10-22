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

SynthLFOPanel::SynthLFOPanel (const yup::String& panelTitle, const SynthLFOParameters& parametersToEdit, const yup::Font& font)
    : parameters (parametersToEdit)
    , shownValues (parametersToEdit.read())
    , retriggerButton ("RETRIG", parametersToEdit.retrigger)
    , shapeChoice ("SHAPE", parametersToEdit.shape, font)
    , rateKnob ("RATE", parametersToEdit.rate, font)
    , phaseKnob ("PHASE", parametersToEdit.phase, font)
{
    setupPanelTitle (titleLabel, panelTitle, font);
    addAndMakeVisible (titleLabel);

    addAndMakeVisible (retriggerButton);

    display.setValues (shownValues);
    addAndMakeVisible (display);

    addAndMakeVisible (shapeChoice);
    addAndMakeVisible (rateKnob);
    addAndMakeVisible (phaseKnob);
}

void SynthLFOPanel::syncFromParameters()
{
    retriggerButton.syncFromParameter();
    shapeChoice.syncFromParameter();
    rateKnob.syncFromParameter();
    phaseKnob.syncFromParameter();

    if (const auto values = parameters.read(); values != shownValues)
    {
        shownValues = values;
        display.setValues (values);
    }
}

void SynthLFOPanel::resized()
{
    auto bounds = getLocalBounds().reduced (panelInset);

    auto header = bounds.removeFromTop (headerHeight);
    retriggerButton.setBounds (header.removeFromRight (buttonWidth));
    titleLabel.setBounds (header);
    bounds.removeFromTop (spacing);

    // Everything sits in one row so the panel stays short: display, shape, rate, phase.
    auto knobs = bounds.removeFromRight (knobWidth * 2.0f);
    bounds.removeFromRight (spacing);
    auto choice = bounds.removeFromRight (choiceWidth);
    bounds.removeFromRight (spacing);
    display.setBounds (bounds);

    shapeChoice.setBounds (choice.withSizeKeepingCenter (choice.getWidth(), yup::jmin (choice.getHeight(), choiceHeight)));
    layoutControlsInRow (knobs, { &rateKnob, &phaseKnob });
}

void SynthLFOPanel::paint (yup::Graphics& g)
{
    paintSynthPanel (g, getLocalBounds());
}
