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

#include "SynthModulationRow.h"

SynthModulationRow::SynthModulationRow (const SynthModulationParameters::Slot& slotToEdit, const yup::Font& font)
    : sourceChoice ({}, slotToEdit.source, font)
    , destinationChoice ({}, slotToEdit.destination, font)
    , depthKnob ("DEPTH", slotToEdit.depth, font)
{
    setOpaque (false);

    addAndMakeVisible (sourceChoice);
    addAndMakeVisible (destinationChoice);
    addAndMakeVisible (depthKnob);
}

void SynthModulationRow::syncFromParameters()
{
    sourceChoice.syncFromParameter();
    destinationChoice.syncFromParameter();
    depthKnob.syncFromParameter();
}

void SynthModulationRow::resized()
{
    auto bounds = getLocalBounds();

    depthKnob.setBounds (bounds.removeFromRight (knobWidth));
    bounds.removeFromRight (spacing);
    sourceChoice.setBounds (bounds.removeFromLeft (bounds.getWidth() * 0.35f).reduced (0.0f, 4.0f));
    bounds.removeFromLeft (spacing);
    destinationChoice.setBounds (bounds.reduced (0.0f, 4.0f));
}
