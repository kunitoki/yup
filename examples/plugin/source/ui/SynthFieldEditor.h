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

#pragma once

#include <yup_gui/yup_gui.h>

#include <functional>

//==============================================================================
/** A one line text field that takes the keyboard focus only while it is clicked to be
    edited, and on return or escape hands it to the nearest parent that takes it, so the
    computer keyboard plays the notes again. Return keeps the edit; escape first restores
    the text the field had when the edit started, notifying onTextChange.

    @see SynthPresetBar, SynthMacroPanel
*/
class SynthFieldEditor : public yup::TextEditor
{
public:
    SynthFieldEditor();

    /** Called when a click starts editing. */
    std::function<void()> onStarted;

    /** Called when editing stops, by return, escape or the focus moving away. It runs
        inside the focus change, so it must not move the focus itself. */
    std::function<void()> onFinished;

    void mouseDown (const yup::MouseEvent& event) override;
    void focusLost() override;
    bool keyDown (const yup::KeyPress& key, const yup::Point<float>& position) override;

private:
    yup::String textBeforeEdit;
};
