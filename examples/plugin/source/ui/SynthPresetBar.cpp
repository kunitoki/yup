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
#include "SynthPresetLibrary.h"
#include "SynthTheme.h"
#include "../browser/PresetBrowseState.h"

namespace
{
constexpr const char* presetExtension = ".retrace";
constexpr const char* fieldCaptions[] = { "BANK:", "AUTHOR:", "TAGS:" };
} // namespace

SynthPresetBar::SynthPresetBar (RetraceProcessor& processorToControl)
    : processor (processorToControl)
{
    setOpaque (false);

    listButton.onClick = [this] { toggleBrowser(); };
    menuButton.onClick = [this] { toggleBrowser(); };
    previousButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };

    for (auto* button : { &listButton, &menuButton, &previousButton, &nextButton })
        addAndMakeVisible (*button);

    processor.getPresetBrowser();
    syncFromProcessor();
}

void SynthPresetBar::syncFromProcessor()
{
    const auto info = processor.getPresetInfo();

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

    for (std::size_t field = 0; field < valueAreas.size(); ++field)
    {
        auto area = second.removeFromLeft (fieldWidth);
        captionAreas[field] = area.removeFromLeft (SynthTheme::textWidth (fieldCaptions[field], captionFont) + 4.0f);
        valueAreas[field] = area.reduced (4.0f, 6.0f);
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

    const std::array<yup::String, 3> values { shownInfo.bank, shownInfo.author, shownInfo.tags };
    const auto valueFont = SynthTheme::valueFont (13.5f);
    g.setFillColor (SynthTheme::textPrimary (*this));

    for (std::size_t field = 0; field < valueAreas.size(); ++field)
        g.fillFittedText (values[field], valueFont, valueAreas[field], yup::Justification::centerLeft);
}

void SynthPresetBar::stepPreset (int delta)
{
    const auto currentKey = processor.getCurrentPresetKey();
    const auto key = processor.getPresetBrowser().getSteppedKey (currentKey, delta);
    const auto* entry = processor.getPresetLibrary().getLibrary().findEntry (key);

    if (entry == nullptr || key == currentKey)
        return;

    if (const auto result = processor.loadPresetEntry (*entry); result.failed())
        yup::Logger::writeToLog ("Retrace: " + result.getErrorMessage());

    syncFromProcessor();
}

void SynthPresetBar::setBrowserOpen (bool isOpen)
{
    listButton.setHighlighted (isOpen);
    menuButton.setHighlighted (isOpen);
}

void SynthPresetBar::toggleBrowser()
{
    if (onBrowserToggled != nullptr)
        onBrowserToggled();
}

void SynthPresetBar::mouseMove (const yup::MouseEvent& event)
{
    setMouseCursor (nameArea.contains (event.getPosition()) ? yup::MouseCursor::Hand : yup::MouseCursor::Default);
}

void SynthPresetBar::mouseDown (const yup::MouseEvent& event)
{
    if (nameArea.contains (event.getPosition()))
        toggleBrowser();
}

void SynthPresetBar::savePreset()
{
    auto chooser = yup::FileChooser::create ("Save Retrace preset", SynthPresetLibrary::getUserFolder(), yup::String ("*") + presetExtension);

    chooser->browseForFileToSave ([safeThis = yup::WeakReference<yup::Component> (this)] (bool success, const yup::Array<yup::File>& results)
    {
        auto* bar = dynamic_cast<SynthPresetBar*> (safeThis.get());

        if (bar == nullptr || ! success || results.isEmpty())
            return;

        if (const auto result = bar->processor.savePresetFile (results[0].withFileExtension (presetExtension)); result.failed())
            yup::Logger::writeToLog ("Retrace: " + result.getErrorMessage());
        else
            bar->processor.getPresetLibrary().getLibrary().rescan();

        bar->syncFromProcessor();
    }, true);
}

void SynthPresetBar::loadPreset()
{
    auto chooser = yup::FileChooser::create ("Load Retrace preset", SynthPresetLibrary::getUserFolder(), yup::String ("*") + presetExtension);

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
