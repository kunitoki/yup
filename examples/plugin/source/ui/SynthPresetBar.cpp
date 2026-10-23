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
constexpr const char* fieldCaptions[] = { "BANK:", "AUTHOR:", "TAGS:" };
} // namespace

SynthPresetBar::FieldEditor::FieldEditor()
{
    setWantsKeyboardFocus (false);
}

void SynthPresetBar::FieldEditor::mouseDown (const yup::MouseEvent& event)
{
    if (! isReadOnly())
        setWantsKeyboardFocus (true);

    yup::TextEditor::mouseDown (event);
}

void SynthPresetBar::FieldEditor::focusLost()
{
    yup::TextEditor::focusLost();
    setWantsKeyboardFocus (false);

    if (onFinished != nullptr)
        onFinished();
}

void SynthPresetBar::FieldEditor::keyDown (const yup::KeyPress& key, const yup::Point<float>& position)
{
    if (key.getKey() == yup::KeyPress::enterKey || key.getKey() == yup::KeyPress::escapeKey)
    {
        leaveKeyboardFocus();
        return;
    }

    yup::TextEditor::keyDown (key, position);
}

SynthPresetBar::SynthPresetBar (PrismProcessor& processorToControl)
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
        editor.setColor (yup::TextEditor::Style::backgroundColorId, yup::Colors::transparentBlack);
        editor.setColor (yup::TextEditor::Style::outlineColorId, yup::Colors::transparentBlack);
        editor.setColor (yup::TextEditor::Style::focusedOutlineColorId, SynthTheme::accent());
        editor.setColor (yup::TextEditor::Style::textColorId, SynthTheme::textPrimary());
        editor.setColor (yup::TextEditor::Style::caretColorId, SynthTheme::accent());
        editor.onTextChange = [this, field] { writeField (static_cast<Field> (field), editors[static_cast<std::size_t> (field)].getText()); };
        editor.onFinished = [this] { finishEditingWhenIdle(); };
        addAndMakeVisible (editor);
    }

    syncFromProcessor();
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

        if (! editor.hasKeyboardFocus() && editor.getText() != values[field])
            editor.setText (values[field], yup::dontSendNotification);
    }

    if (info == shownInfo)
        return;

    shownInfo = info;
    repaint();
}

yup::Rectangle<float> SynthPresetBar::getTrailingArea() const
{
    auto line = getLocalBounds().withTrimmedTop (getHeight() * 0.5f);
    return line.removeFromRight (trailingWidth).reduced (8.0f, 6.0f);
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

    second.removeFromRight (trailingWidth - padding);
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
    SynthTheme::paintTile (g, bounds);

    const auto middle = bounds.getY() + bounds.getHeight() * 0.5f;
    SynthTheme::paintSeam (g, { bounds.getX() + 1.0f, middle }, { bounds.getRight() - 1.0f, middle });

    g.setFillColor (SynthTheme::accent());
    g.fillFittedText (shownInfo.name.isNotEmpty() ? shownInfo.name : yup::String ("Untitled"), SynthTheme::valueFont (18.0f), nameArea, yup::Justification::centerLeft);

    const auto captionFont = SynthTheme::captionFont (11.5f);
    g.setFillColor (SynthTheme::textSecondary());

    for (std::size_t field = 0; field < captionAreas.size(); ++field)
        g.fillFittedText (fieldCaptions[field], captionFont, captionAreas[field], yup::Justification::centerLeft);
}

void SynthPresetBar::finishEditingWhenIdle()
{
    // Deferred, so moving from one field to another does not hand the focus away.
    yup::MessageManager::callAsync ([safeThis = yup::WeakReference<yup::Component> (this)]
    {
        auto* bar = dynamic_cast<SynthPresetBar*> (safeThis.get());

        if (bar == nullptr || bar->onEditingFinished == nullptr)
            return;

        for (const auto& editor : bar->editors)
            if (editor.hasKeyboardFocus())
                return;

        bar->onEditingFinished();
    });
}

void SynthPresetBar::writeField (Field field, const yup::String& text)
{
    if (processor.getCurrentPreset() >= 0)
        return;

    auto info = processor.getPresetInfo();
    (field == bank ? info.bank : field == author ? info.author : info.tags) = text;
    processor.setPresetInfo (std::move (info));
}

void SynthPresetBar::showPresetMenu()
{
    auto menu = yup::PopupMenu::create (yup::PopupMenu::Options {}
                                            .withParentComponent (getPopupParentComponent())
                                            .withMinimumWidth (static_cast<int> (nameArea.getWidth()))
                                            .withRelativePosition (this));

    for (int index = 0; index < processor.getNumPresets(); ++index)
        menu->addItem (processor.getPresetName (index), index + 1, true, index == processor.getCurrentPreset());

    menu->show ([safeThis = yup::WeakReference<yup::Component> (this)] (int selectedId)
    {
        auto* bar = dynamic_cast<SynthPresetBar*> (safeThis.get());

        if (bar == nullptr || selectedId <= 0)
            return;

        bar->processor.setCurrentPreset (selectedId - 1);
        bar->syncFromProcessor();
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
    auto chooser = yup::FileChooser::create ("Save Prism preset", getPresetFolder(), yup::String ("*") + presetExtension);

    chooser->browseForFileToSave ([safeThis = yup::WeakReference<yup::Component> (this)] (bool success, const yup::Array<yup::File>& results)
    {
        auto* bar = dynamic_cast<SynthPresetBar*> (safeThis.get());

        if (bar == nullptr || ! success || results.isEmpty())
            return;

        if (const auto result = bar->processor.savePresetFile (results[0].withFileExtension (presetExtension)); result.failed())
            yup::Logger::writeToLog ("Prism: " + result.getErrorMessage());

        bar->syncFromProcessor();
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
            yup::Logger::writeToLog ("Prism: " + result.getErrorMessage());

        bar->syncFromProcessor();
    });
}

yup::File SynthPresetBar::getPresetFolder()
{
    auto folder = yup::File::getSpecialLocation (yup::File::userApplicationDataDirectory).getChildFile ("YUP/Prism/Presets");
    folder.createDirectory();
    return folder;
}
