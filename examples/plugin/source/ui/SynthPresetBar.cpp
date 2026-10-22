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

#include "SynthPresetBar.h"
#include "SynthTheme.h"

namespace
{
constexpr const char* presetExtension = ".prism";
} // namespace

SynthPresetBar::SynthPresetBar (PrismProcessor& processorToControl)
    : processor (processorToControl)
{
    setOpaque (false);

    for (int index = 0; index < processor.getNumPresets(); ++index)
        presetCombo.addItem (processor.getPresetName (index), index + 1);

    presetCombo.setTextWhenNothingSelected ("-");
    SynthTheme::styleComboBox (presetCombo);
    presetCombo.onSelectedItemChanged = [this]
    {
        const auto index = presetCombo.getSelectedId() - 1;

        if (index >= 0 && index != processor.getCurrentPreset())
            processor.setCurrentPreset (index);
    };
    addAndMakeVisible (presetCombo);

    for (auto* button : { &previousButton, &nextButton, &saveButton, &loadButton })
    {
        SynthTheme::styleButton (*button, SynthTheme::panelBackground());
        addAndMakeVisible (*button);
    }

    previousButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    saveButton.onClick = [this] { savePreset(); };
    loadButton.onClick = [this] { loadPreset(); };

    syncFromProcessor();
}

void SynthPresetBar::syncFromProcessor()
{
    const auto id = processor.getCurrentPreset() + 1;

    if (presetCombo.getSelectedId() != yup::jmax (0, id))
        presetCombo.setSelectedId (yup::jmax (0, id), yup::dontSendNotification);
}

void SynthPresetBar::setCustomPatchName (const yup::String& name)
{
    presetCombo.setTextWhenNothingSelected (name);
}

void SynthPresetBar::resized()
{
    auto bounds = getLocalBounds();

    loadButton.setBounds (bounds.removeFromRight (fileButtonWidth));
    bounds.removeFromRight (spacing);
    saveButton.setBounds (bounds.removeFromRight (fileButtonWidth));
    bounds.removeFromRight (spacing * 2.0f);
    nextButton.setBounds (bounds.removeFromRight (buttonWidth));
    bounds.removeFromRight (spacing);
    previousButton.setBounds (bounds.removeFromRight (buttonWidth));
    bounds.removeFromRight (spacing);
    presetCombo.setBounds (bounds);
}

void SynthPresetBar::stepPreset (int delta)
{
    const auto count = processor.getNumPresets();

    if (count <= 0)
        return;

    const auto current = processor.getCurrentPreset();
    const auto next = current < 0 ? 0 : (current + delta + count) % count;

    processor.setCurrentPreset (next);
    syncFromProcessor();
}

void SynthPresetBar::savePreset()
{
    auto chooser = yup::FileChooser::create ("Save Prism preset", getPresetFolder(), yup::String ("*") + presetExtension);

    chooser->browseForFileToSave ([safeThis = yup::WeakReference<yup::Component> (this)] (bool success, const yup::Array<yup::File>& results)
    {
        auto* bar = dynamic_cast<SynthPresetBar*> (safeThis.get());

        if (bar == nullptr || ! success || results.isEmpty())
            return;

        const auto file = results[0].withFileExtension (presetExtension);

        if (const auto result = bar->processor.savePresetFile (file); result.failed())
        {
            yup::Logger::writeToLog ("Prism: " + result.getErrorMessage());
            return;
        }

        bar->setCustomPatchName (file.getFileNameWithoutExtension());
    }, true);
}

void SynthPresetBar::loadPreset()
{
    auto chooser = yup::FileChooser::create ("Load Prism preset", getPresetFolder(), yup::String ("*") + presetExtension);

    chooser->browseForFileToOpen ([safeThis = yup::WeakReference<yup::Component> (this)] (bool success, const yup::Array<yup::File>& results)
    {
        auto* bar = dynamic_cast<SynthPresetBar*> (safeThis.get());

        if (bar == nullptr || ! success || results.isEmpty())
            return;

        if (const auto result = bar->processor.loadPresetFile (results[0]); result.failed())
        {
            yup::Logger::writeToLog ("Prism: " + result.getErrorMessage());
            return;
        }

        bar->setCustomPatchName (results[0].getFileNameWithoutExtension());
        bar->syncFromProcessor();
    });
}

yup::File SynthPresetBar::getPresetFolder()
{
    auto folder = yup::File::getSpecialLocation (yup::File::userApplicationDataDirectory).getChildFile ("YUP/Prism/Presets");
    folder.createDirectory();
    return folder;
}
