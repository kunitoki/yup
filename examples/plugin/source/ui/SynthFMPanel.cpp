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

SynthFMPanel::SynthFMPanel (const SynthFMParameters& parametersToEdit)
{
    setOpaque (false);

    for (int carrier = 0; carrier < SynthExample::oscillatorCount; ++carrier)
    {
        for (int modulator = 0; modulator < SynthExample::oscillatorCount; ++modulator)
        {
            auto& knob = knobs[static_cast<std::size_t> (carrier)][static_cast<std::size_t> (modulator)];
            knob = std::make_unique<ParameterKnob> (yup::String(), parametersToEdit.get (carrier, modulator), SynthTheme::captionFont (11.0f));
            knob->getKnob().setShowsValueInWell (true);
            knob->getKnob().getValueText = [parameter = parametersToEdit.get (carrier, modulator)]
            {
                return yup::String (yup::roundToInt (parameter->getNormalizedValue() * 100.0f));
            };

            if (carrier == modulator)
                knob->getKnob().setAccent (SynthTheme::lfoAccent());

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
    auto bounds = getLocalBounds();
    bounds.removeFromBottom (hintHeight);
    bounds.removeFromTop (labelSize);
    bounds.removeFromLeft (labelSize);
    grid = bounds;

    const auto count = static_cast<float> (SynthExample::oscillatorCount);
    const auto cellWidth = grid.getWidth() / count;
    const auto cellHeight = grid.getHeight() / count;
    const auto size = yup::jmin (cellWidth, cellHeight) - SynthTheme::controlGap;

    for (int carrier = 0; carrier < SynthExample::oscillatorCount; ++carrier)
    {
        for (int modulator = 0; modulator < SynthExample::oscillatorCount; ++modulator)
        {
            const yup::Rectangle<float> cell { grid.getX() + cellWidth * static_cast<float> (modulator),
                                               grid.getY() + cellHeight * static_cast<float> (carrier),
                                               cellWidth,
                                               cellHeight };

            knobs[static_cast<std::size_t> (carrier)][static_cast<std::size_t> (modulator)]->setBounds (cell.withSizeKeepingCenter (size, size));
        }
    }
}

void SynthFMPanel::paint (yup::Graphics& g)
{
    const auto count = static_cast<float> (SynthExample::oscillatorCount);
    const auto cellWidth = grid.getWidth() / count;
    const auto cellHeight = grid.getHeight() / count;
    const auto font = SynthTheme::captionFont (11.5f);

    g.setFillColor (SynthTheme::textSecondary());

    for (int index = 0; index < SynthExample::oscillatorCount; ++index)
    {
        const auto offset = static_cast<float> (index);
        g.fillFittedText ("OP " + yup::String (index + 1), font, { grid.getX() + cellWidth * offset, 0.0f, cellWidth, labelSize }, yup::Justification::center);
        g.fillFittedText (yup::String (index + 1), font, { 0.0f, grid.getY() + cellHeight * offset, labelSize, cellHeight }, yup::Justification::centerLeft);
    }

    const yup::Rectangle<float> hint { 0.0f, getHeight() - hintHeight, getWidth(), hintHeight };
    g.fillFittedText ("Each cell is how much the column operator modulates the row. Diagonal = feedback.",
                      SynthTheme::valueFont (12.0f), hint, yup::Justification::centerLeft);
}
