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

#include "ParameterGesture.h"
#include "SynthFieldEditor.h"

#include <functional>

//==============================================================================
/** A text field shown over a control to type its parameter's value, on an opaque well.

    It opens filled with the parameter's value text, all selected. Return or the focus
    moving away sets the typed value, parsed by the parameter, as one undo step; escape,
    an empty text or the unchanged text leave the parameter as it was.

    @see ParameterKnob, SynthNumberField, SynthSliderField
*/
class ParameterTextEntry
{
public:
    /** The height the entry needs to be readable; it is never shown shorter. */
    static constexpr float minimumHeight = 22.0f;

    /** Creates the entry, hidden, as a child of a control moving a parameter with a gesture. */
    ParameterTextEntry (yup::Component& owningControl, ParameterGesture& gestureToUse);

    /** Shows the entry over an area of the control and starts editing. */
    void show (yup::Rectangle<float> area);

    /** True while the entry is shown. */
    bool isShowing() const { return editor.isVisible(); }

    /** Called when the entry closes, whether or not a value was set. */
    std::function<void()> onClosed;

private:
    void finish();

    ParameterGesture& gesture;
    SynthFieldEditor editor;
    yup::String shownText;
};
