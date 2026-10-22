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

#include "SynthModulationPage.h"
#include "SynthTheme.h"

SynthModulationPage::SynthModulationPage (const SynthModulationParameters& parametersToEdit, const yup::Font& font)
{
    setupPanelTitle (titleLabel, "MODULATION MATRIX", font);
    addAndMakeVisible (titleLabel);

    for (std::size_t index = 0; index < rows.size(); ++index)
    {
        rows[index] = std::make_unique<SynthModulationRow> (parametersToEdit.slots[index], font);
        addAndMakeVisible (*rows[index]);
    }
}

void SynthModulationPage::syncFromParameters()
{
    for (auto& row : rows)
        row->syncFromParameters();
}

void SynthModulationPage::resized()
{
    auto bounds = getLocalBounds().reduced (panelInset);

    titleLabel.setBounds (bounds.removeFromTop (headerHeight));
    bounds.removeFromTop (spacing);

    const auto rowHeight = (bounds.getHeight() - spacing * static_cast<float> (rows.size() - 1)) / static_cast<float> (rows.size());

    for (auto& row : rows)
    {
        row->setBounds (bounds.removeFromTop (rowHeight));
        bounds.removeFromTop (spacing);
    }
}

void SynthModulationPage::paint (yup::Graphics& g)
{
    paintSynthPanel (g, getLocalBounds());
}
