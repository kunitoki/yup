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

#include "SynthFMPanel.h"
#include "SynthTheme.h"

SynthFMPanel::SynthFMPanel (const SynthFMParameters& parametersToEdit, const yup::Font& font)
{
    setupPanelTitle (titleLabel, "FM MATRIX  /  ROW MODULATED BY COLUMN", font);
    addAndMakeVisible (titleLabel);

    for (int carrier = 0; carrier < SynthExample::oscillatorCount; ++carrier)
    {
        for (int modulator = 0; modulator < SynthExample::oscillatorCount; ++modulator)
        {
            const auto caption = carrier == modulator ? "FB " + yup::String (carrier + 1)
                                                      : yup::String (modulator + 1) + " > " + yup::String (carrier + 1);

            auto& knob = knobs[static_cast<std::size_t> (carrier)][static_cast<std::size_t> (modulator)];
            knob = std::make_unique<ParameterKnob> (caption, parametersToEdit.get (carrier, modulator), font);
            addAndMakeVisible (*knob);
        }
    }
}

void SynthFMPanel::syncFromParameters()
{
    for (auto& row : knobs)
        for (auto& knob : row)
            knob->syncFromParameter();
}

void SynthFMPanel::resized()
{
    auto bounds = getLocalBounds().reduced (panelInset);

    titleLabel.setBounds (bounds.removeFromTop (headerHeight));
    bounds.removeFromTop (spacing);

    const auto rowHeight = bounds.getHeight() / static_cast<float> (SynthExample::oscillatorCount);

    for (auto& row : knobs)
    {
        std::vector<yup::Component*> controls;

        for (auto& knob : row)
            controls.push_back (knob.get());

        layoutControlsInRow (bounds.removeFromTop (rowHeight), controls);
    }
}

void SynthFMPanel::paint (yup::Graphics& g)
{
    paintSynthPanel (g, getLocalBounds());
}
