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

#include "ExampleEditor.h"

namespace
{
constexpr float referenceWidth = 520.0f;
constexpr float referenceHeight = 320.0f;
constexpr float titleHeight = 52.0f;
constexpr float keyboardHeight = 92.0f;
constexpr float captionHeight = 22.0f;
constexpr float knobPadding = 12.0f;
constexpr float titleFontHeight = 26.0f;
constexpr float captionFontHeight = 14.0f;

constexpr int keyboardLowestNote = 48;  // C3
constexpr int keyboardHighestNote = 72; // C5
} // namespace

//==============================================================================
ExampleEditor::ExampleEditor (ExamplePlugin& processorToEdit)
    : processor (processorToEdit)
    , keyboard (processorToEdit.getKeyboardState(), yup::MidiKeyboardComponent::horizontalKeyboard)
    , font (yup::ApplicationTheme::getGlobalTheme()->getDefaultFont())
{
    title.setText (YupPlugin_Name, yup::dontSendNotification);
    title.setJustification (yup::Justification::center);
    title.setColor (yup::Label::Style::textFillColorId, yup::Colors::white);
    addAndMakeVisible (title);

    bindKnob (knobs[0], "gain", "Gain");
    bindKnob (knobs[1], "waveform", "Waveform");
    bindKnob (knobs[2], "attack", "Attack");
    bindKnob (knobs[3], "release", "Release");

    keyboard.setAvailableRange (0, 127);
    keyboard.setVisibleRange (keyboardLowestNote, keyboardHighestNote);
    addAndMakeVisible (keyboard);

    setSize (getPreferredSize().to<float>());
}

//==============================================================================
float ExampleEditor::getScaleFactor() const
{
    const auto bounds = getLocalBounds();
    return yup::jmin (bounds.getWidth() / referenceWidth, bounds.getHeight() / referenceHeight);
}

ExampleEditor::~ExampleEditor() = default;

//==============================================================================
void ExampleEditor::bindKnob (Knob& knob, const yup::String& parameterID, const yup::String& caption)
{
    knob.parameter = processor.getParameterByID (parameterID);
    knob.name = caption;

    knob.caption.setJustification (yup::Justification::center);
    knob.caption.setColor (yup::Label::Style::textFillColorId, yup::Color (0xffb9bec9));
    addAndMakeVisible (knob.caption);

    if (knob.parameter == nullptr)
        return;

    const auto step = knob.parameter->isStepped() ? 1.0 : 0.0;
    knob.slider.setRange (knob.parameter->getMinimumValue(), knob.parameter->getMaximumValue(), step);
    knob.slider.setValue (knob.parameter->getValue(), yup::dontSendNotification);
    updateCaption (knob);

    auto* parameter = knob.parameter.get();
    knob.slider.onValueChanged = [this, knobPtr = &knob, parameter] (double value)
    {
        if (updatingFromParameter)
            return;

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (static_cast<float> (value));
        parameter->endChangeGesture();
        updateCaption (*knobPtr);
    };

    knob.attachment = std::make_unique<yup::AudioParameterAttachment> (
        *this,
        knob.parameter,
        [this, knobPtr = &knob]
        {
        updatingFromParameter = true;
        knobPtr->slider.setValue (knobPtr->parameter->getValue(), yup::dontSendNotification);
        updatingFromParameter = false;
        updateCaption (*knobPtr);
        },
        true);

    addAndMakeVisible (knob.slider);
}

//==============================================================================
void ExampleEditor::updateCaption (Knob& knob)
{
    if (knob.parameter == nullptr)
        return;

    knob.caption.setText (knob.name + ": " + knob.parameter->toString(), yup::dontSendNotification);
}

//==============================================================================
void ExampleEditor::resized()
{
    const auto scale = getScaleFactor();
    auto area = getLocalBounds();

    keyboard.setBounds (area.removeFromBottom (keyboardHeight * scale));

    title.setFont (font.withHeight (titleFontHeight * scale));
    title.setBounds (area.removeFromTop (titleHeight * scale));

    const auto columnWidth = area.getWidth() / static_cast<float> (knobs.size());
    const auto captionH = captionHeight * scale;

    for (auto& knob : knobs)
    {
        auto column = area.removeFromLeft (columnWidth);

        const auto side = yup::jmin (column.getWidth(), column.getHeight() - captionH) - knobPadding * scale * 2.0f;
        const auto groupHeight = side + captionH;
        const auto groupTop = column.getY() + (column.getHeight() - groupHeight) * 0.5f;
        const auto centerX = column.getCenterX();

        knob.slider.setBounds (yup::Rectangle<float> (centerX - side * 0.5f, groupTop, side, side));

        knob.caption.setFont (font.withHeight (captionFontHeight * scale));
        knob.caption.setBounds (yup::Rectangle<float> (column.getX(), groupTop + side, column.getWidth(), captionH));
    }
}

//==============================================================================
void ExampleEditor::paint (yup::Graphics& g)
{
    g.setFillColor (0xff1d1f24);
    g.fillAll();
}
