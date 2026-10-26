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

#include "SynthMacroPanel.h"
#include "SynthTheme.h"
#include "SynthUndoHistory.h"

SynthMacroPanel::SynthMacroPanel (RetraceProcessor& processorToControl)
    : processor (processorToControl)
{
    setOpaque (false);

    const auto& macros = processor.getSynthParameters().macros;

    for (int macro = 0; macro < SynthExample::macroCount; ++macro)
    {
        const auto index = static_cast<std::size_t> (macro);

        knobs[index] = std::make_unique<ParameterKnob> (yup::String(), macros[index], SynthTheme::captionFont (11.0f));
        addAndMakeVisible (*knobs[index]);

        auto& name = names[index];
        name.setMultiLine (false);
        name.setFont (SynthTheme::captionFont (11.5f));
        name.onStarted = [this, macro] { namesBeforeEdit[static_cast<std::size_t> (macro)] = processor.getPresetInfo().macroNames[static_cast<std::size_t> (macro)]; };
        name.onTextChange = [this, macro] { writeName (macro, names[static_cast<std::size_t> (macro)].getText()); };
        name.onFinished = [this, macro] { finishName (macro); };
        addAndMakeVisible (name);
    }

    syncFromProcessor();
}

void SynthMacroPanel::syncFromProcessor()
{
    for (auto& knob : knobs)
        knob->syncFromParameter();

    const auto info = processor.getPresetInfo();

    for (int macro = 0; macro < SynthExample::macroCount; ++macro)
    {
        const auto index = static_cast<std::size_t> (macro);
        auto& name = names[index];
        const auto shown = info.getMacroName (macro);

        if (! name.hasKeyboardFocus())
        {
            if (name.getText() != shown)
                name.setText (shown, yup::dontSendNotification);
        }
        else if (info.macroNames[index] != toStoredName (macro, name.getText()))
        {
            namesBeforeEdit[index] = info.macroNames[index];
            name.setText (shown, yup::dontSendNotification);
        }
    }
}

void SynthMacroPanel::writeName (int macro, const yup::String& text)
{
    auto info = processor.getPresetInfo();
    info.macroNames[static_cast<std::size_t> (macro)] = toStoredName (macro, text);
    processor.setPresetInfo (std::move (info));
}

/** Returns a name as the patch keeps it; an empty one, or the default, leaves the macro unnamed. */
yup::String SynthMacroPanel::toStoredName (int macro, const yup::String& text)
{
    const auto trimmed = text.trim();
    const auto isDefault = trimmed.isEmpty() || trimmed == SynthPatch::PresetInfo().getMacroName (macro);

    return isDefault ? yup::String() : trimmed;
}

/** Keeps a finished rename as one undo step. */
void SynthMacroPanel::finishName (int macro)
{
    const auto index = static_cast<std::size_t> (macro);
    const auto before = namesBeforeEdit[index];
    const auto after = processor.getPresetInfo().macroNames[index];

    if (after != before)
    {
        SynthUndoHistory::record<yup::String> (*this, before, after, [&processor = processor, index] (const yup::String& name)
        {
            auto info = processor.getPresetInfo();
            info.macroNames[index] = name;
            processor.setPresetInfo (std::move (info));
        });
    }

    namesBeforeEdit[index] = after;
    syncFromProcessor();
}

void SynthMacroPanel::resized()
{
    const auto bounds = getLocalBounds();
    const auto rows = SynthExample::macroCount / columns;
    const auto cellWidth = bounds.getWidth() / static_cast<float> (columns);
    const auto cellHeight = bounds.getHeight() / static_cast<float> (rows);
    const auto knobSize = yup::jmax (0.0f, yup::jmin (cellWidth, cellHeight - nameHeight) - SynthTheme::controlGap);

    for (int macro = 0; macro < SynthExample::macroCount; ++macro)
    {
        const yup::Rectangle<float> cell { bounds.getX() + cellWidth * static_cast<float> (macro % columns),
                                           bounds.getY() + cellHeight * static_cast<float> (macro / columns),
                                           cellWidth,
                                           cellHeight };

        auto area = cell.withSizeKeepingCenter (cellWidth, knobSize + nameHeight);
        knobs[static_cast<std::size_t> (macro)]->setBounds (area.removeFromTop (knobSize).withSizeKeepingCenter (knobSize, knobSize));
        names[static_cast<std::size_t> (macro)].setBounds (area.reduced (2.0f, 0.0f));
    }
}
