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
#include "SynthUndoHistory.h"

#include <algorithm>

namespace
{
constexpr const char* presetExtension = ".retrace";
constexpr const char* fieldCaptions[] = { "BANK:", "AUTHOR:", "TAGS:" };
} // namespace

SynthPresetBar::SynthPresetBar (RetraceProcessor& processorToControl)
    : processor (processorToControl)
{
    setOpaque (false);

    listButton.onClick = [this] { showPresetMenu(); };
    menuButton.onClick = [this] { showPresetMenu(); };
    previousButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };

    for (auto* button : { &listButton, &menuButton, &previousButton, &nextButton })
        addAndMakeVisible (*button);

    for (int field = bank; field <= tags; ++field)
    {
        auto& editor = editors[static_cast<std::size_t> (field)];
        editor.setMultiLine (false);
        editor.setFont (SynthTheme::valueFont (13.5f));
        editor.onTextChange = [this, field] { writeField (static_cast<Field> (field), editors[static_cast<std::size_t> (field)].getText()); };
        editor.onStarted = [this, field]
        {
            auto info = processor.getPresetInfo();
            fieldsBeforeEdit[static_cast<std::size_t> (field)] = getField (info, static_cast<Field> (field));
        };
        editor.onFinished = [this, field] { finishField (static_cast<Field> (field)); };
        addAndMakeVisible (editor);
    }

    syncFromProcessor();
    scanUserPresets();
}

void SynthPresetBar::syncFromProcessor()
{
    const auto info = processor.getPresetInfo();
    const auto isFactory = processor.getCurrentPreset() >= 0;
    const std::array<yup::String, 3> values { info.bank, info.author, info.tags };

    for (std::size_t field = 0; field < editors.size(); ++field)
    {
        auto& editor = editors[field];
        editor.setReadOnly (isFactory);

        if (editor.getText() == values[field])
            continue;

        if (editor.hasKeyboardFocus())
            fieldsBeforeEdit[field] = values[field];

        editor.setText (values[field], yup::dontSendNotification);
    }

    if (info == shownInfo)
        return;

    shownInfo = info;
    repaint();
}

void SynthPresetBar::resized()
{
    auto bounds = getLocalBounds();
    auto first = bounds.removeFromTop (bounds.getHeight() * 0.5f).reduced (padding - 4.0f, 0.0f);
    auto second = bounds.reduced (padding, 0.0f);

    listButton.setBounds (first.removeFromLeft (arrowWidth));
    first.removeFromLeft (4.0f);
    nextButton.setBounds (first.removeFromRight (arrowWidth));
    previousButton.setBounds (first.removeFromRight (arrowWidth));
    first.removeFromRight (8.0f);
    menuButton.setBounds (first.removeFromRight (arrowWidth));
    nameArea = first;

    const auto fieldWidth = second.getWidth() / 3.0f;
    const auto captionFont = SynthTheme::captionFont (11.5f);

    for (std::size_t field = 0; field < editors.size(); ++field)
    {
        auto area = second.removeFromLeft (fieldWidth);
        captionAreas[field] = area.removeFromLeft (SynthTheme::textWidth (fieldCaptions[field], captionFont) + 4.0f);
        editors[field].setBounds (area.reduced (2.0f, 6.0f));
    }
}

void SynthPresetBar::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    SynthTheme::paintTile (g, *this, bounds);

    const auto middle = bounds.getY() + bounds.getHeight() * 0.5f;
    SynthTheme::paintSeam (g, *this, { bounds.getX() + 1.0f, middle }, { bounds.getRight() - 1.0f, middle });

    g.setFillColor (SynthTheme::accent (*this));
    g.fillFittedText (shownInfo.name.isNotEmpty() ? shownInfo.name : yup::String ("Untitled"), SynthTheme::valueFont (18.0f), nameArea, yup::Justification::centerLeft);

    const auto captionFont = SynthTheme::captionFont (11.5f);
    g.setFillColor (SynthTheme::textSecondary (*this));

    for (std::size_t field = 0; field < captionAreas.size(); ++field)
        g.fillFittedText (fieldCaptions[field], captionFont, captionAreas[field], yup::Justification::centerLeft);
}

void SynthPresetBar::writeField (Field field, const yup::String& text)
{
    if (processor.getCurrentPreset() >= 0)
        return;

    auto info = processor.getPresetInfo();
    getField (info, field) = text;
    processor.setPresetInfo (std::move (info));
}

/** Keeps a finished edit as one undo step. */
void SynthPresetBar::finishField (Field field)
{
    const auto index = static_cast<std::size_t> (field);
    auto info = processor.getPresetInfo();
    const auto before = fieldsBeforeEdit[index];
    const auto after = getField (info, field);

    if (after != before)
    {
        SynthUndoHistory::record<yup::String> (*this, before, after, [&processor = processor, field] (const yup::String& text)
        {
            auto restored = processor.getPresetInfo();
            getField (restored, field) = text;
            processor.setPresetInfo (std::move (restored));
        });
    }

    fieldsBeforeEdit[index] = after;
}

yup::String& SynthPresetBar::getField (SynthPatch::PresetInfo& info, Field field)
{
    return field == bank ? info.bank : field == author ? info.author : info.tags;
}

void SynthPresetBar::showPresetMenu()
{
    auto menu = yup::PopupMenu::create (yup::PopupMenu::Options {}
                                            .withParentComponent (getPopupParentComponent())
                                            .withMinimumWidth (static_cast<int> (nameArea.getWidth()))
                                            .withRelativePosition (this));

    // Factory preset IDs are 1 based; user preset IDs are userBase plus their index.
    constexpr int userBase = 10000;

    for (int index = 0; index < processor.getNumPresets(); ++index)
        menu->addItem (processor.getPresetName (index), index + 1, true, index == processor.getCurrentPreset());

    if (! userPresets.empty())
    {
        menu->addSeparator();
        menu->addItem ("User", -1, false);

        for (std::size_t index = 0; index < userPresets.size(); ++index)
        {
            const auto name = userPresets[index].getFileNameWithoutExtension();
            menu->addItem (name, userBase + static_cast<int> (index), true, processor.getCurrentPreset() < 0 && name == shownInfo.name);
        }
    }

    menu->show ([safeThis = yup::WeakReference<yup::Component> (this)] (int selectedId)
    {
        auto* bar = dynamic_cast<SynthPresetBar*> (safeThis.get());

        if (bar == nullptr || selectedId <= 0)
            return;

        if (selectedId < userBase)
        {
            bar->processor.setCurrentPreset (selectedId - 1);
        }
        else if (const auto index = static_cast<std::size_t> (selectedId - userBase); index < bar->userPresets.size())
        {
            if (const auto result = bar->processor.loadPresetFile (bar->userPresets[index]); result.failed())
                yup::Logger::writeToLog ("Retrace: " + result.getErrorMessage());
        }

        bar->syncFromProcessor();
    });
}

/** Gathers the preset files of the preset folder on a background thread, so a large
    folder never stalls the interface, and hands the list to the bar when done. */
void SynthPresetBar::scanUserPresets()
{
    yup::Thread::launch ([folder = getPresetFolder(), safeThis = yup::WeakReference<yup::Component> (this)]
    {
        std::vector<yup::File> found;

        for (const auto& file : folder.findChildFiles (yup::File::findFiles, true, yup::String ("*") + presetExtension, yup::File::FollowSymlinks::noCycles))
            found.push_back (file);

        std::sort (found.begin(), found.end(), [] (const yup::File& a, const yup::File& b)
        {
            return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()) < 0;
        });

        yup::MessageManager::callAsync ([safeThis, found = std::move (found)]() mutable
        {
            if (auto* bar = dynamic_cast<SynthPresetBar*> (safeThis.get()))
                bar->userPresets = std::move (found);
        });
    });
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
    auto chooser = yup::FileChooser::create ("Save Retrace preset", getPresetFolder(), yup::String ("*") + presetExtension);

    chooser->browseForFileToSave ([safeThis = yup::WeakReference<yup::Component> (this)] (bool success, const yup::Array<yup::File>& results)
    {
        auto* bar = dynamic_cast<SynthPresetBar*> (safeThis.get());

        if (bar == nullptr || ! success || results.isEmpty())
            return;

        if (const auto result = bar->processor.savePresetFile (results[0].withFileExtension (presetExtension)); result.failed())
            yup::Logger::writeToLog ("Retrace: " + result.getErrorMessage());

        bar->syncFromProcessor();
        bar->scanUserPresets();
    }, true);
}

void SynthPresetBar::loadPreset()
{
    auto chooser = yup::FileChooser::create ("Load Retrace preset", getPresetFolder(), yup::String ("*") + presetExtension);

    chooser->browseForFileToOpen ([safeThis = yup::WeakReference<yup::Component> (this)] (bool success, const yup::Array<yup::File>& results)
    {
        auto* bar = dynamic_cast<SynthPresetBar*> (safeThis.get());

        if (bar == nullptr || ! success || results.isEmpty())
            return;

        if (const auto result = bar->processor.loadPresetFile (results[0]); result.failed())
            yup::Logger::writeToLog ("Retrace: " + result.getErrorMessage());

        bar->syncFromProcessor();
    });
}

yup::File SynthPresetBar::getPresetFolder()
{
    auto folder = yup::File::getSpecialLocation (yup::File::userAudioPresetsDirectory)
                      .getChildFile (yup::File::createLegalFileName (YupPlugin_Vendor))
                      .getChildFile (yup::File::createLegalFileName (YupPlugin_Name));
    folder.createDirectory();
    return folder;
}
