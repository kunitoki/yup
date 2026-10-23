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

SynthFilterPanel::SynthFilterPanel (const SynthFilterParameters& parametersToEdit)
    : typeChoice ({}, parametersToEdit.type, SynthTheme::valueFont (13.5f))
    , cutoffField ("Cut", parametersToEdit.cutoff)
    , resonanceField ("Q", parametersToEdit.resonance)
    , response (parametersToEdit)
    , driveKnob ("DRIVE", parametersToEdit.drive, SynthTheme::captionFont (12.0f))
    , keytrackKnob ("KEYTRACK", parametersToEdit.keytrack, SynthTheme::captionFont (12.0f))
    , mixKnob ("MIX", parametersToEdit.mix, SynthTheme::captionFont (12.0f))
{
    setOpaque (false);

    for (auto* control : std::initializer_list<yup::Component*> { &typeChoice, &cutoffField, &resonanceField, &response, &driveKnob, &keytrackKnob, &mixKnob })
        addAndMakeVisible (*control);
}

void SynthFilterPanel::syncFromParameters()
{
    typeChoice.syncFromParameter();
    cutoffField.syncFromParameter();
    resonanceField.syncFromParameter();
    response.syncFromParameters();

    for (auto* knob : { &driveKnob, &keytrackKnob, &mixKnob })
        knob->syncFromParameter();
}

void SynthFilterPanel::resized()
{
    const auto gap = SynthTheme::controlGap;
    auto bounds = getLocalBounds();

    auto line = bounds.removeFromTop (lineHeight);
    typeChoice.setBounds (line.removeFromLeft (yup::jmin (140.0f, line.getWidth() * 0.4f)));
    resonanceField.setBounds (line.removeFromRight (70.0f));
    line.removeFromRight (gap);
    cutoffField.setBounds (line.removeFromRight (104.0f));
    bounds.removeFromTop (gap);

    auto knobs = bounds.removeFromBottom (knobRowHeight);
    bounds.removeFromBottom (gap);
    response.setBounds (bounds);

    const auto width = knobs.getWidth() / 3.0f;

    for (auto* knob : { &driveKnob, &keytrackKnob, &mixKnob })
        knob->setBounds (knobs.removeFromLeft (width));
}
