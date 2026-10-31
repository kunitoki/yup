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

#include "ParameterChoice.h"
#include "SynthTheme.h"

ParameterChoice::ParameterChoice (const yup::String& caption, yup::AudioParameter::Ptr parameterToControl, const yup::Font& font)
    : parameter (std::move (parameterToControl))
    , gesture (*this, parameter)
{
    setOpaque (false); // the caption and combo box paint themselves, the row draws nothing

    label.setText (caption, yup::dontSendNotification);
    label.setFont (font);

    if (caption.isNotEmpty())
        addAndMakeVisible (label);

    for (int step = 0; step <= parameter->getNumSteps(); ++step)
        comboBox.addItem (parameter->convertToString (parameter->getMinimumValue() + static_cast<float> (step)), step + 1);

    comboBox.setTextWhenNothingSelected ("-");
    comboBox.setSelectedId (getParameterItemId(), yup::dontSendNotification);
    comboBox.onSelectedItemChanged = [this]
    {
        const auto id = comboBox.getSelectedId();

        if (id <= 0 || id == getParameterItemId())
            return;

        gesture.setNormalized (parameter->convertToNormalizedValue (parameter->getMinimumValue() + static_cast<float> (id - 1)));

        if (onChange != nullptr)
            onChange();
    };
    addAndMakeVisible (comboBox);
}

bool ParameterChoice::syncFromParameter()
{
    const auto id = getParameterItemId();

    if (comboBox.getSelectedId() == id)
        return false;

    comboBox.setSelectedId (id, yup::dontSendNotification);
    return true;
}

void ParameterChoice::resized()
{
    auto bounds = getLocalBounds();

    if (label.getText().isNotEmpty())
        label.setBounds (bounds.removeFromTop (captionHeight));

    comboBox.setBounds (bounds);
}

int ParameterChoice::getParameterItemId() const
{
    return yup::roundToInt (parameter->getValue() - parameter->getMinimumValue()) + 1;
}
