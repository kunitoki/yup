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

#include "SynthPresetBrowser.h"
#include "SynthPresetLibrary.h"
#include "SynthPresetPrompt.h"
#include "SynthTheme.h"
#include "../browser/PresetBrowseState.h"
#include "../browser/PresetFileOperations.h"

namespace
{
enum MenuItem
{
    renameId = 1,
    duplicateId,
    deleteId,
    revealId,
    saveHereId
};
} // namespace

SynthPresetBrowser::SynthPresetBrowser (RetraceProcessor& processorToControl)
    : processor (processorToControl)
    , sources (processorToControl, processorToControl.getPresetLibrary())
    , filters (processorToControl.getPresetBrowser())
    , list (processorToControl)
    , info (processorToControl)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);

    // The lists, the tree and the fields sit in wells painted like the waveform display, so their own boxes go.
    for (const auto& colorId : { yup::ListBox::Style::backgroundColorId, yup::ListBox::Style::outlineColorId, yup::TextEditor::Style::backgroundColorId,
                                 yup::TextEditor::Style::outlineColorId, yup::TextEditor::Style::focusedOutlineColorId })
        setColor (colorId, yup::Colors::transparentBlack);

    for (auto* column : std::initializer_list<yup::Component*> { &sources, &filters, &list, &info })
        addAndMakeVisible (*column);

    list.onMenuRequested = [this] (int row, yup::Point<float> position) { showRowMenu (row, getLocalPoint (&list, position)); };
    list.onPresetChosen = [this]
    {
        if (onPresetChosen != nullptr)
            onPresetChosen();
    };
    sources.onSaveIntoFolder = [this] (const yup::File& folder) { saveIntoFolder (folder); };
}

void SynthPresetBrowser::syncFromProcessor()
{
    list.syncFromProcessor();
    info.syncFromProcessor();
}

void SynthPresetBrowser::focusList()
{
    list.focusList();
}

void SynthPresetBrowser::resized()
{
    auto bounds = getLocalBounds().reduced (SynthTheme::panelPadding);
    auto left = bounds.removeFromLeft (leftColumnWidth);
    bounds.removeFromLeft (SynthTheme::panelPadding);
    info.setBounds (bounds.removeFromRight (rightColumnWidth));
    bounds.removeFromRight (SynthTheme::panelPadding);
    list.setBounds (bounds);

    sources.setBounds (left.removeFromTop (std::round (left.getHeight() * sourcesShare)));
    left.removeFromTop (SynthTheme::panelPadding);
    filters.setBounds (left);
}

/** The browser holds the focus after a click on its filters or a finished search, so the
    list's navigation keys reach the list from here too. */
bool SynthPresetBrowser::keyDown (const yup::KeyPress& key, const yup::Point<float>&)
{
    for (const auto navigationKey : { yup::KeyPress::upKey, yup::KeyPress::downKey, yup::KeyPress::pageUpKey,
                                      yup::KeyPress::pageDownKey, yup::KeyPress::homeKey, yup::KeyPress::endKey })
    {
        if (key.getKey() == navigationKey)
            return list.navigate (key);
    }

    return false;
}

void SynthPresetBrowser::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();

    g.setFillColor (SynthTheme::frameColor (*this));
    g.fillRect (bounds);
    SynthTheme::paintTile (g, *this, bounds);
}

/** A row's menu works on a copy of its entry, since a rescan may replace the listed one
    before an item is picked. */
void SynthPresetBrowser::showRowMenu (int row, yup::Point<float> position)
{
    const auto* listed = processor.getPresetBrowser().getResult (row);
    const auto entry = listed != nullptr ? *listed : PresetEntry {};
    auto* parent = getPopupParentComponent();
    auto menu = yup::PopupMenu::create (yup::PopupMenu::Options {}
                                            .withParentComponent (parent)
                                            .withPosition (parent != nullptr ? parent->getLocalPoint (this, position) : position));

    if (listed != nullptr)
    {
        const auto isFile = ! entry.isFactory();
        menu->addItem ("Rename...", renameId, isFile);
        menu->addItem ("Duplicate", duplicateId, isFile);
        menu->addItem ("Delete...", deleteId, isFile);
        menu->addSeparator();
        menu->addItem (SynthPresetSources::getRevealCommandName(), revealId, isFile);
    }
    else
    {
        menu->addItem ("Save current patch here...", saveHereId);
    }

    menu->show ([safeThis = yup::WeakReference<yup::Component> (this), entry] (int selectedId)
    {
        auto* browser = dynamic_cast<SynthPresetBrowser*> (safeThis.get());

        if (browser == nullptr)
            return;

        switch (selectedId)
        {
            case renameId:
                browser->renamePreset (entry);
                break;

            case duplicateId:
                browser->duplicatePreset (entry);
                break;

            case deleteId:
                browser->deletePreset (entry);
                break;

            case revealId:
                entry.file.revealToUser();
                break;

            case saveHereId:
            {
                const auto& query = browser->processor.getPresetBrowser().getQuery();
                browser->saveIntoFolder (query.source == PresetQuery::Source::folder ? query.folder : SynthPresetLibrary::getUserFolder());
                break;
            }

            default:
                break;
        }
    });
}

void SynthPresetBrowser::renamePreset (const PresetEntry& entry)
{
    SynthPresetPrompt::askForName (*this, "Rename preset", entry.name, [this, entry] (const yup::String& name) -> yup::String
    {
        auto& presetLibrary = processor.getPresetLibrary();
        const auto renamed = PresetFileOperations::rename (entry, name, presetLibrary.getLibrary().getFormat(), presetLibrary.getUserData());

        if (renamed.failed())
            return renamed.getErrorMessage();

        presetLibrary.presetFileChanged (entry.file, renamed.getReference());
        presetLibrary.getLibrary().rescan();
        return {};
    });
}

void SynthPresetBrowser::duplicatePreset (const PresetEntry& entry)
{
    auto& presetLibrary = processor.getPresetLibrary();

    if (const auto copied = PresetFileOperations::duplicate (entry, presetLibrary.getLibrary().getFormat(), presetLibrary.getUserData()); copied.failed())
        yup::Logger::writeToLog ("Retrace: " + copied.getErrorMessage());
    else
        presetLibrary.getLibrary().rescan();
}

void SynthPresetBrowser::deletePreset (const PresetEntry& entry)
{
    SynthPresetPrompt::askToConfirm (*this, "Delete preset", "Delete '" + entry.name + "'?", "Delete", [this, entry]
    {
        auto& presetLibrary = processor.getPresetLibrary();

        if (const auto result = PresetFileOperations::remove (entry, presetLibrary.getUserData()); result.failed())
        {
            yup::Logger::writeToLog ("Retrace: " + result.getErrorMessage());
            return;
        }

        // The patch stays loaded, but no longer comes from a file.
        presetLibrary.presetFileChanged (entry.file, PresetEntry {});
        presetLibrary.getLibrary().rescan();
    });
}

void SynthPresetBrowser::saveIntoFolder (const yup::File& folder)
{
    SynthPresetPrompt::askForName (*this, "Save preset", processor.getPresetInfo().name, [this, folder] (const yup::String& name) -> yup::String
    {
        auto& library = processor.getPresetLibrary().getLibrary();
        const auto file = PresetFileOperations::getFileFor (folder, name, library.getFormat());

        if (name.isEmpty())
            return "A preset needs a name";

        if (file == yup::File())
            return "The name has no characters a file name can use";

        if (file.exists())
            return "A preset called '" + file.getFileNameWithoutExtension() + "' already exists";

        if (const auto result = processor.savePresetFile (file); result.failed())
            return result.getErrorMessage();

#if YUP_WASM
        file.startAsProcess();
#endif

        library.rescan();
        return {};
    });
}
