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

#include "SynthPresetInfoPanel.h"
#include "SynthPresetLibrary.h"
#include "SynthTheme.h"
#include "../browser/PresetFileOperations.h"
#include "../browser/PresetQuery.h"

namespace
{
constexpr const char* captions[] = { "NAME", "CATEGORY", "AUTHOR", "BANK", "DESCRIPTION", "NOTES", "TAGS" };
} // namespace

SynthPresetInfoPanel::SynthPresetInfoPanel (RetraceProcessor& processorToControl)
    : processor (processorToControl)
{
    setOpaque (false);

    for (int field = 0; field < fieldCount; ++field)
    {
        auto& editor = editors[static_cast<std::size_t> (field)];
        const auto isText = field == description || field == notes;
        editor.setMultiLine (isText);
        editor.setFont (SynthTheme::valueFont (isText ? 12.0f : 13.0f));
        editor.onFinished = [this, field] { finishField (static_cast<Field> (field)); };
        addAndMakeVisible (editor);
    }

    categoryButton.onClick = [this] { showCategoryMenu(); };
    addAndMakeVisible (categoryButton);

    showEntry (nullptr);
    syncFromProcessor();
}

void SynthPresetInfoPanel::syncFromProcessor()
{
    const auto& library = processor.getPresetLibrary().getLibrary();
    const auto key = processor.getCurrentPresetKey();
    const auto* entry = library.findEntry (key);

    // A renamed preset is only listed again once the rescan it started is done.
    if (entry == nullptr && hasEntry && library.isScanning())
        return;

    shownKey = key;
    showEntry (entry);
}

yup::String SynthPresetInfoPanel::getValue (const PresetEntry& entry, Field field)
{
    switch (field)
    {
        case name:
            return entry.name;
        case category:
            return entry.category;
        case author:
            return entry.author;
        case bank:
            return entry.bank;
        case description:
            return entry.description;
        case notes:
            return entry.notes;
        case tags:
        case fieldCount:
            break;
    }

    return PresetEntry::joinTags (entry.tags);
}

/** Fields being typed in keep their text; the others follow the entry. */
void SynthPresetInfoPanel::showEntry (const PresetEntry* entry)
{
    hasEntry = entry != nullptr;
    const auto readOnly = entry == nullptr || entry->isFactory();

    for (int field = 0; field < fieldCount; ++field)
    {
        auto& editor = editors[static_cast<std::size_t> (field)];
        const auto value = entry != nullptr ? getValue (*entry, static_cast<Field> (field)) : yup::String();

        shownValues[static_cast<std::size_t> (field)] = value;
        editor.setReadOnly (readOnly);

        if (! editor.hasKeyboardFocus() && editor.getText() != value)
            editor.setText (value, yup::dontSendNotification);
    }

    categoryButton.setEnabled (! readOnly);
}

/** Writes a changed field into the preset file, or a new name as a rename, then lets the
    processor and the library follow the file. */
void SynthPresetInfoPanel::finishField (Field field)
{
    const auto index = static_cast<std::size_t> (field);
    const auto text = editors[index].getText();

    if (text == shownValues[index])
        return;

    auto& presetLibrary = processor.getPresetLibrary();
    auto& library = presetLibrary.getLibrary();
    const auto* entry = library.findEntry (shownKey);

    if (entry == nullptr || entry->isFactory())
    {
        editors[index].setText (shownValues[index], yup::dontSendNotification);
        return;
    }

    const auto previousFile = entry->file;
    auto edited = *entry;
    auto result = yup::Result::ok();

    if (field == name)
    {
        if (const auto renamed = PresetFileOperations::rename (*entry, text, library.getFormat(), presetLibrary.getUserData()); renamed.failed())
        {
            result = yup::Result::fail (renamed.getErrorMessage());
        }
        else
        {
            edited.file = renamed.getValue();
            edited.key = edited.file.getFullPathName();
            edited.name = edited.file.getFileNameWithoutExtension();
        }
    }
    else
    {
        if (field == tags)
            edited.tags = PresetEntry::parseTags (text);
        else if (field == category)
            edited.category = text;
        else if (field == author)
            edited.author = text;
        else if (field == bank)
            edited.bank = text;
        else if (field == description)
            edited.description = text;
        else
            edited.notes = text;

        result = PresetFileOperations::writeMetadata (edited, library.getFormat());
    }

    if (result.failed())
    {
        yup::Logger::writeToLog ("Retrace: " + result.getErrorMessage());
        editors[index].setText (shownValues[index], yup::dontSendNotification);
        return;
    }

    shownValues[index] = getValue (edited, field);
    processor.presetFileChanged (previousFile, edited);
    library.rescan();
}

void SynthPresetInfoPanel::showCategoryMenu()
{
    const auto categories = PresetQuery::collectCategories (processor.getPresetLibrary().getLibrary().getEntries());
    const auto current = editors[category].getText();

    if (categories.isEmpty())
        return;

    auto menu = yup::PopupMenu::create (yup::PopupMenu::Options {}
                                            .withParentComponent (getPopupParentComponent())
                                            .withMinimumWidth (static_cast<int> (wellAreas[category].getWidth()))
                                            .withRelativePosition (&editors[category]));

    for (int index = 0; index < categories.size(); ++index)
        menu->addItem (categories[index], index + 1, true, categories[index].equalsIgnoreCase (current));

    menu->show ([safeThis = yup::WeakReference<yup::Component> (this), categories] (int selectedId)
    {
        auto* panel = dynamic_cast<SynthPresetInfoPanel*> (safeThis.get());

        if (panel == nullptr || ! yup::isPositiveAndBelow (selectedId - 1, categories.size()))
            return;

        panel->editors[category].setText (categories[selectedId - 1], yup::dontSendNotification);
        panel->finishField (category);
    });
}

void SynthPresetInfoPanel::resized()
{
    auto bounds = getLocalBounds();

    for (int field = 0; field < fieldCount; ++field)
    {
        const auto index = static_cast<std::size_t> (field);
        const auto isText = field == description || field == notes;

        captionAreas[index] = bounds.removeFromTop (captionHeight);
        wellAreas[index] = bounds.removeFromTop (isText ? textHeight : fieldHeight);
        bounds.removeFromTop (SynthTheme::controlGap);

        auto editorArea = wellAreas[index].reduced (4.0f, 2.0f);

        if (field == category)
            categoryButton.setBounds (editorArea.removeFromRight (18.0f));

        editors[index].setBounds (editorArea);
    }
}

void SynthPresetInfoPanel::paint (yup::Graphics& g)
{
    const auto captionFont = SynthTheme::captionFont (9.5f);
    g.setFillColor (SynthTheme::textSecondary (*this));

    for (std::size_t index = 0; index < captionAreas.size(); ++index)
        g.fillFittedText (captions[index], captionFont, captionAreas[index], yup::Justification::bottomLeft);

    for (const auto& area : wellAreas)
        SynthTheme::paintWell (g, *this, area, SynthTheme::displayCorner);
}

void SynthPresetInfoPanel::paintOverChildren (yup::Graphics& g)
{
    for (const auto& area : wellAreas)
        SynthTheme::paintDisplayBorder (g, *this, area);
}
