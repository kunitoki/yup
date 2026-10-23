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

#include "ParameterKnob.h"
#include "SynthTheme.h"

ParameterKnob::ParameterKnob (const yup::String& caption, yup::AudioParameter::Ptr parameterToControl, const yup::Font& font)
    : parameter (std::move (parameterToControl))
{
    setOpaque (false); // the knob and its caption paint themselves, the row draws nothing

    bindSlider();
    slider.getValueText = [this] { return parameter->toString(); };
    slider.onValueChanged = [this] (double value) { setParameterValue (value); };
    slider.onDragStart = [this] (const yup::MouseEvent&)
    {
        parameter->beginChangeGesture();
        dragging = true;
        slider.setShowingValue (true);
    };
    slider.onDragEnd = [this] (const yup::MouseEvent&) { endGesture(); };
    addAndMakeVisible (slider);

    captionText = caption;
    label.setText (caption, yup::dontSendNotification);
    label.setFont (font);
    label.setJustification (yup::Justification::center);
    addAndMakeVisible (label);
}

ParameterKnob::~ParameterKnob()
{
    endGesture();
}

void ParameterKnob::bindSlider()
{
    const auto steps = parameter->getNumSteps();
    slider.setRange (0.0, 1.0, steps > 0 ? 1.0 / steps : 0.0);
    slider.setDefaultValue (parameter->convertToNormalizedValue (parameter->getDefaultValue()));
    slider.setValue (parameter->getNormalizedValue(), yup::dontSendNotification);
}

void ParameterKnob::setParameter (yup::AudioParameter::Ptr newParameter)
{
    if (newParameter == parameter)
        return;

    endGesture();
    parameter = std::move (newParameter);
    bindSlider();

    if (captionShowsValue)
        label.setText (parameter->toString(), yup::dontSendNotification);
}

void ParameterKnob::setCaptionShowsValue (bool shouldShowValue)
{
    captionShowsValue = shouldShowValue;
    label.setText (captionShowsValue ? parameter->toString() : captionText, yup::dontSendNotification);
    resized();
}

float ParameterKnob::getCaptionHeight() const noexcept
{
    return captionText.isEmpty() && ! captionShowsValue ? 0.0f : captionHeight;
}

bool ParameterKnob::syncFromParameter()
{
    const auto normalized = static_cast<double> (parameter->getNormalizedValue());

    if (captionShowsValue && label.getText() != parameter->toString())
        label.setText (parameter->toString(), yup::dontSendNotification);
    if (dragging || std::abs (slider.getValue() - normalized) < 1.0e-6)
        return false;

    slider.setValue (normalized, yup::dontSendNotification);
    return true;
}

void ParameterKnob::resized()
{
    const auto bounds = getLocalBounds();
    const auto caption = getCaptionHeight();
    const auto size = yup::jmax (0.0f, yup::jmin (bounds.getWidth(), bounds.getHeight() - caption));

    // The knob and its caption are centered as one block, so the caption sits right under the knob.
    auto block = bounds.withSizeKeepingCenter (bounds.getWidth(), size + caption);
    slider.setBounds (block.removeFromTop (size).withSizeKeepingCenter (size, size));
    label.setBounds (block);
}

void ParameterKnob::setParameterValue (double normalizedValue)
{
    const auto value = parameter->convertToDenormalizedValue (static_cast<float> (normalizedValue));

    if (! dragging)
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (value);
        parameter->endChangeGesture();
        return;
    }

    parameter->setValueNotifyingHost (value);
}

void ParameterKnob::endGesture()
{
    if (! dragging)
        return;

    dragging = false;
    parameter->endChangeGesture();
    slider.setShowingValue (false);
}
