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

#include "SynthKeyboardToolbar.h"
#include "SynthTheme.h"

SynthKeyboardToolbar::SynthKeyboardToolbar (SynthTuning& tuningToEdit, yup::MidiKeyboardComponent& keyboardToPlay)
    : tuning (tuningToEdit)
    , keyboard (keyboardToPlay)
{
    captionFont = SynthTheme::captionFont (10.5f);

    setOpaque (false);

    modeSelector.onSelect = [this] (int index)
    {
        tuning.setBendMode (index == 1 ? SynthBendMode::steps : SynthBendMode::semitones);
    };

    tuningBox.onSelectedItemChanged = [this]
    {
        const auto index = tuningBox.getSelectedId() - 1;

        if (yup::isPositiveAndBelow (index, SynthTuning::getNumPresets()) && index != tuning.getPresetIndex())
            tuning.setPreset (index);
    };

    previousButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    loadButton.onClick = [this] { loadTuningFile(); };

    bendDownBox.setTitle ("Pitch bend down");
    bendUpBox.setTitle ("Pitch bend up");
    octaveBox.setTitle ("Computer keyboard base octave");
    octaveBox.setValue (keyboard.getKeyPressBaseOctave());
    loadButton.setTitle ("Load a tuning file");

    for (auto* child : std::initializer_list<yup::Component*> { &bendDownBox, &bendUpBox, &modeSelector, &octaveBox, &tuningBox, &previousButton, &nextButton, &loadButton })
        addAndMakeVisible (*child);

    tuning.addChangeListener (this);
    syncFromTuning();
}

SynthKeyboardToolbar::~SynthKeyboardToolbar()
{
    tuning.removeChangeListener (this);
}

//==============================================================================
SynthKeyboardToolbar::ValueBox::ValueBox (int minimumValue, int maximumValue, int defaultValueToUse, std::function<yup::String (int)> formatValue, std::function<void (int)> onChangeCallback)
    : minimum (minimumValue)
    , maximum (maximumValue)
    , defaultValue (defaultValueToUse)
    , format (std::move (formatValue))
    , onChange (std::move (onChangeCallback))
{
    valueFont = SynthTheme::valueFont (13.5f);

    setOpaque (false);
    setMouseCursor (yup::MouseCursor::Hand);
}

void SynthKeyboardToolbar::ValueBox::setValue (int newValue)
{
    if (newValue == value)
        return;

    value = newValue;
    repaint();
}

void SynthKeyboardToolbar::ValueBox::changeTo (int newValue)
{
    newValue = yup::jlimit (minimum, maximum, newValue);

    if (newValue == value)
        return;

    setValue (newValue);
    onChange (value);
}

void SynthKeyboardToolbar::ValueBox::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    SynthTheme::paintWell (g, *this, bounds);

    g.setFillColor (isEnabled() ? SynthTheme::accent (*this) : SynthTheme::textSecondary (*this));
    g.fillFittedText (format (value), valueFont.withHeight (yup::jmin (13.5f, bounds.getHeight() * 0.65f)), bounds.reduced (4.0f, 0.0f), yup::Justification::center);
}

void SynthKeyboardToolbar::ValueBox::mouseDown (const yup::MouseEvent&)
{
    dragStartValue = value;
}

void SynthKeyboardToolbar::ValueBox::mouseDrag (const yup::MouseEvent& event)
{
    const auto travel = event.getLastMouseDownPosition().getY() - event.getPosition().getY();
    const auto scale = event.getModifiers().isShiftDown() ? 0.25f : 1.0f;

    changeTo (dragStartValue + static_cast<int> (std::trunc (travel * scale / pixelsPerStep)));
}

void SynthKeyboardToolbar::ValueBox::mouseDoubleClick (const yup::MouseEvent&)
{
    changeTo (defaultValue);
}

//==============================================================================
void SynthKeyboardToolbar::loadTuningFile()
{
    auto chooser = yup::FileChooser::create ("Load a tuning", yup::File(), "*.scl;*.kbm;*.tun;*.msf");

    chooser->browseForFileToOpen ([safeThis = yup::WeakReference<yup::Component> (this)] (bool success, const yup::Array<yup::File>& results)
    {
        auto* toolbar = dynamic_cast<SynthKeyboardToolbar*> (safeThis.get());

        if (toolbar == nullptr || ! success || results.isEmpty())
            return;

        if (const auto result = toolbar->tuning.loadFile (results[0]); result.failed())
            yup::Logger::writeToLog ("Retrace: " + result.getErrorMessage());
    });
}

void SynthKeyboardToolbar::stepPreset (int delta)
{
    const auto count = SynthTuning::getNumPresets();
    const auto current = tuning.getPresetIndex();

    if (current < 0)
        tuning.setPreset (delta > 0 ? 0 : count - 1);
    else
        tuning.setPreset ((current + delta + count) % count);
}

void SynthKeyboardToolbar::changeListenerCallback (yup::ChangeBroadcaster*)
{
    syncFromTuning();
}

void SynthKeyboardToolbar::syncFromTuning()
{
    bendDownBox.setValue (tuning.getBendDown());
    bendUpBox.setValue (tuning.getBendUp());
    modeSelector.setSelectedIndex (tuning.getBendMode() == SynthBendMode::steps ? 1 : 0);

    tuningBox.clear();

    for (int index = 0; index < SynthTuning::getNumPresets(); ++index)
        tuningBox.addItem (SynthTuning::getPresetName (index), index + 1);

    const auto preset = tuning.getPresetIndex();

    if (preset < 0)
    {
        tuningBox.addSeparator();
        tuningBox.addItem (tuning.getName(), customTuningId);
    }

    tuningBox.setSelectedId (preset < 0 ? customTuningId : preset + 1, yup::dontSendNotification);
}

//==============================================================================
void SynthKeyboardToolbar::resized()
{
    auto bounds = getLocalBounds();

    bendCaption = bounds.removeFromLeft (captionWidth);
    bendDownBox.setBounds (bounds.removeFromLeft (rangeWidth));
    bounds.removeFromLeft (gap * 0.5f);
    bendUpBox.setBounds (bounds.removeFromLeft (rangeWidth));
    bounds.removeFromLeft (gap);
    modeSelector.setBounds (bounds.removeFromLeft (modeWidth));
    bounds.removeFromLeft (gap);

    keysCaption = bounds.removeFromLeft (keysCaptionWidth);
    octaveBox.setBounds (bounds.removeFromLeft (rangeWidth));
    bounds.removeFromLeft (gap);

    loadButton.setBounds (bounds.removeFromRight (arrowWidth));
    bounds.removeFromRight (gap * 0.5f);
    nextButton.setBounds (bounds.removeFromRight (arrowWidth));

    const auto tuningWidth = yup::jmax (0.0f, bounds.getWidth() - captionWidth - arrowWidth);
    tuningBox.setBounds (bounds.removeFromRight (yup::jmin (tuningWidth, getTuningBoxWidth())));
    previousButton.setBounds (bounds.removeFromRight (arrowWidth));
    tuningCaption = bounds.removeFromRight (captionWidth);
}

float SynthKeyboardToolbar::getTuningBoxWidth() const
{
    const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();
    const auto textFont = font.withHeight (yup::jmin (font.getHeight(), getHeight() * 0.6f));
    auto widest = 0.0f;

    for (int index = 0; index < SynthTuning::getNumPresets(); ++index)
        widest = yup::jmax (widest, SynthTheme::textWidth (SynthTuning::getPresetName (index), textFont));

    return widest + comboBoxChrome;
}

void SynthKeyboardToolbar::paint (yup::Graphics& g)
{
    g.setFillColor (SynthTheme::textSecondary (*this));
    g.fillFittedText ("BEND", captionFont, bendCaption, yup::Justification::centerLeft);
    g.fillFittedText ("KEYS", captionFont, keysCaption, yup::Justification::centerLeft);
    g.fillFittedText ("TUNING", captionFont, tuningCaption.withTrimmedRight (gap), yup::Justification::centerRight);
}
