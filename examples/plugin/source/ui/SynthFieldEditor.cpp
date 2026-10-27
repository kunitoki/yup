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

#include "SynthFieldEditor.h"

SynthFieldEditor::SynthFieldEditor()
{
    setWantsKeyboardFocus (false);
}

void SynthFieldEditor::mouseDown (const yup::MouseEvent& event)
{
    if (! isReadOnly() && ! hasKeyboardFocus())
    {
        textBeforeEdit = getText();
        setWantsKeyboardFocus (true);

        if (onStarted != nullptr)
            onStarted();
    }

    yup::TextEditor::mouseDown (event);
}

void SynthFieldEditor::focusLost()
{
    yup::TextEditor::focusLost();
    setWantsKeyboardFocus (false);

    if (onFinished != nullptr)
        onFinished();
}

bool SynthFieldEditor::keyDown (const yup::KeyPress& key, const yup::Point<float>& position)
{
    if (key.getKey() == yup::KeyPress::enterKey || key.getKey() == yup::KeyPress::escapeKey)
    {
        if (key.getKey() == yup::KeyPress::escapeKey)
            setText (textBeforeEdit, yup::sendNotification);

        // The nearest parent that takes the focus gets it, so the keys go on reaching it.
        for (auto* parent = getParentComponent(); parent != nullptr; parent = parent->getParentComponent())
        {
            parent->takeKeyboardFocus();

            if (parent->hasKeyboardFocus())
                return true;
        }

        leaveKeyboardFocus();
        return true;
    }

    return yup::TextEditor::keyDown (key, position);
}
