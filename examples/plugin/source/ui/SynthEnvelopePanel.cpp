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

#include "SynthEnvelopePanel.h"
#include "SynthTheme.h"

SynthEnvelopePanel::SynthEnvelopePanel (const yup::String& panelTitle, const SynthEnvelopeParameters& parametersToEdit, const yup::Font& font)
    : parameters (parametersToEdit)
    , shownValues (parametersToEdit.read())
    , delayKnob ("DELAY", parametersToEdit.delay, font)
    , attackKnob ("ATTACK", parametersToEdit.attack, font)
    , holdKnob ("HOLD", parametersToEdit.hold, font)
    , decayKnob ("DECAY", parametersToEdit.decay, font)
    , sustainKnob ("SUSTAIN", parametersToEdit.sustain, font)
    , releaseKnob ("RELEASE", parametersToEdit.release, font)
{
    setupPanelTitle (titleLabel, panelTitle, font);
    addAndMakeVisible (titleLabel);

    display.setValues (shownValues);
    addAndMakeVisible (display);

    for (auto* knob : { &delayKnob, &attackKnob, &holdKnob, &decayKnob, &sustainKnob, &releaseKnob })
        addAndMakeVisible (*knob);
}

void SynthEnvelopePanel::syncFromParameters()
{
    for (auto* knob : { &delayKnob, &attackKnob, &holdKnob, &decayKnob, &sustainKnob, &releaseKnob })
        knob->syncFromParameter();

    if (const auto values = parameters.read(); values != shownValues)
    {
        shownValues = values;
        display.setValues (values);
    }
}

void SynthEnvelopePanel::resized()
{
    auto bounds = getLocalBounds().reduced (panelInset);

    titleLabel.setBounds (bounds.removeFromTop (headerHeight));
    bounds.removeFromTop (spacing);

    auto knobArea = bounds.removeFromBottom (knobRowHeight);
    bounds.removeFromBottom (spacing);

    display.setBounds (bounds);

    layoutControlsInRow (knobArea, { &delayKnob, &attackKnob, &holdKnob, &decayKnob, &sustainKnob, &releaseKnob });
}

void SynthEnvelopePanel::paint (yup::Graphics& g)
{
    paintSynthPanel (g, getLocalBounds());
}
