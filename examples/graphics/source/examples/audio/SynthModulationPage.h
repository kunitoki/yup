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

#pragma once

#include "SynthPanels.h"

#include <array>
#include <memory>

//==============================================================================
/** One routing: which source drives which destination, and how much. */
class SynthModulationRow : public yup::Component
{
public:
    SynthModulationRow (SynthModulationSettings::Slot& slotToEdit, const yup::Font& font)
        : slot (slotToEdit)
        , sourceChoice ("SOURCE", getSynthModulationSourceNames(), font)
        , destinationChoice ("DESTINATION", getSynthModulationDestinationNames(), font)
        , depthKnob ("DEPTH", -1.0, 1.0, 0.01, 0.0, font)
    {
        addAndMakeVisible (sourceChoice);
        addAndMakeVisible (destinationChoice);
        addAndMakeVisible (depthKnob);

        depthKnob.formatValue = SynthFormat::bipolar;

        sourceChoice.onChange = [this] (int id) { slot.source = id - 1; };
        destinationChoice.onChange = [this] (int id) { slot.destination = id - 1; };
        depthKnob.onChange = [this] (double value) { slot.depth = static_cast<float> (value); };

        refresh();
    }

    /** Reads the slot back into the widgets. */
    void refresh()
    {
        sourceChoice.getComboBox().setSelectedId (slot.source.load() + 1, yup::dontSendNotification);
        destinationChoice.getComboBox().setSelectedId (slot.destination.load() + 1, yup::dontSendNotification);
        depthKnob.getSlider().setValue (slot.depth.load(), yup::dontSendNotification);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        depthKnob.setBounds (bounds.removeFromRight (knobWidth));
        bounds.removeFromRight (spacing);
        sourceChoice.setBounds (bounds.removeFromLeft (bounds.getWidth() * 0.35f).reduced (0.0f, 8.0f));
        bounds.removeFromLeft (spacing);
        destinationChoice.setBounds (bounds.reduced (0.0f, 8.0f));
    }

private:
    static constexpr float knobWidth = 58.0f;
    static constexpr float spacing = 6.0f;

    SynthModulationSettings::Slot& slot;

    ChoiceControl sourceChoice;
    ChoiceControl destinationChoice;
    KnobControl depthKnob;
};

//==============================================================================
/** The modulation matrix: eight routings in a panel.

    @see SynthModulationSettings
*/
class SynthModulationPage : public yup::Component
{
public:
    SynthModulationPage (SynthModulationSettings& settingsToEdit, const yup::Font& font)
    {
        titleLabel.setText ("MODULATION MATRIX", yup::dontSendNotification);
        titleLabel.setFont (font.withHeight (12.0f));
        titleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textPrimary);
        addAndMakeVisible (titleLabel);

        for (std::size_t index = 0; index < rows.size(); ++index)
        {
            rows[index] = std::make_unique<SynthModulationRow> (settingsToEdit.slots[index], font);
            addAndMakeVisible (*rows[index]);
        }
    }

    /** Reads every slot back into its row. */
    void refresh()
    {
        for (auto& row : rows)
            row->refresh();
    }

    void resized() override
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

    void paint (yup::Graphics& g) override
    {
        paintSynthPanel (g, getLocalBounds());
    }

private:
    static constexpr float panelInset = 8.0f;
    static constexpr float headerHeight = 18.0f;
    static constexpr float spacing = 6.0f;

    yup::Label titleLabel;
    std::array<std::unique_ptr<SynthModulationRow>, SynthExample::modulationSlots> rows;
};
