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

#include "SynthFilterPanel.h"
#include "SynthTheme.h"

SynthFilterPanel::SynthFilterPanel (const SynthFilterParameters& parametersToEdit, const yup::Font& font)
    : typeChoice ("TYPE", parametersToEdit.type, font)
    , cutoffKnob ("CUTOFF", parametersToEdit.cutoff, font)
    , resonanceKnob ("RESO", parametersToEdit.resonance, font)
    , driveKnob ("DRIVE", parametersToEdit.drive, font)
    , keytrackKnob ("KEYTRACK", parametersToEdit.keytrack, font)
{
    setupPanelTitle (titleLabel, "FILTER", font);
    addAndMakeVisible (titleLabel);

    addAndMakeVisible (typeChoice);

    for (auto* knob : { &cutoffKnob, &resonanceKnob, &driveKnob, &keytrackKnob })
        addAndMakeVisible (*knob);
}

void SynthFilterPanel::syncFromParameters()
{
    typeChoice.syncFromParameter();

    for (auto* knob : { &cutoffKnob, &resonanceKnob, &driveKnob, &keytrackKnob })
        knob->syncFromParameter();
}

void SynthFilterPanel::resized()
{
    auto bounds = getLocalBounds().reduced (panelInset);

    titleLabel.setBounds (bounds.removeFromTop (headerHeight));
    bounds.removeFromTop (spacing);

    typeChoice.setBounds (bounds.removeFromTop (choiceHeight));
    bounds.removeFromTop (spacing);

    layoutControlsInRow (bounds, { &cutoffKnob, &resonanceKnob, &driveKnob, &keytrackKnob });
}

void SynthFilterPanel::paint (yup::Graphics& g)
{
    paintSynthPanel (g, getLocalBounds());
}
