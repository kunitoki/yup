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

#include "ParameterTextEntry.h"
#include "SynthTheme.h"

ParameterTextEntry::ParameterTextEntry (yup::Component& owningControl, ParameterGesture& gestureToUse)
    : gesture (gestureToUse)
{
    editor.setMultiLine (false);
    editor.setFont (SynthTheme::valueFont (14.0f));
    editor.setJustification (yup::Justification::center);
    editor.setSelectAllWhenFocused (true);
    editor.onFinished = [this] { finish(); };
    owningControl.addChildComponent (editor);
}

void ParameterTextEntry::show (yup::Rectangle<float> area)
{
    shownText = gesture.getParameter()->toString();
    editor.setText (shownText, yup::dontSendNotification);
    // Sections make their editors transparent; this one covers the control, so it hides it.
    editor.setColor (yup::TextEditor::Style::backgroundColorId, SynthTheme::wellColor (editor));
    editor.setBounds (area.withSizeKeepingCenter (area.getWidth(), yup::jmax (area.getHeight(), minimumHeight)));
    editor.setVisible (true);
    editor.toFront (false);
    editor.startEditing();
}

void ParameterTextEntry::finish()
{
    editor.setVisible (false);

    if (const auto text = editor.getText().trim(); text.isNotEmpty() && text != shownText)
    {
        const auto& parameter = gesture.getParameter();
        const auto value = yup::jlimit (parameter->getMinimumValue(), parameter->getMaximumValue(), parameter->convertFromString (text));
        gesture.setNormalized (parameter->convertToNormalizedValue (value));
    }

    if (onClosed != nullptr)
        onClosed();
}
