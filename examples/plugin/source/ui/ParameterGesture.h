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

#include <yup_audio_processors/yup_audio_processors.h>
#include <yup_gui/yup_gui.h>

//==============================================================================
/** Moves an AudioParameter in host gestures on behalf of a control.

    A drag is one gesture from begin() to end(); a change made outside a drag, such as a
    click or a reset, is wrapped in a gesture of its own. Values are normalized.

    Each gesture that changed the value is one step of the undo history the owning
    control sits in; undoing it moves the parameter back in a gesture of its own, as if
    the user had.

    @see SynthUndoHistory, SynthNumberField, SynthSliderField, ParameterSegmented, ParameterLed, SynthSpinner
*/
class ParameterGesture
{
public:
    ParameterGesture (yup::Component& owningControl, yup::AudioParameter::Ptr parameterToMove);
    ~ParameterGesture();

    /** Binds another parameter, ending any gesture on the current one. */
    void setParameter (yup::AudioParameter::Ptr newParameter);

    /** Returns the parameter. */
    const yup::AudioParameter::Ptr& getParameter() const noexcept { return parameter; }

    /** Starts a drag and remembers the normalized value it started from. */
    void begin();

    /** Ends the drag, if one is running.

        @param joinsLastStep True to undo it together with the step recorded just before,
                             for a drag that moves many parameters
    */
    void end (bool joinsLastStep = false);

    /** True while a drag runs. */
    bool isActive() const noexcept { return active; }

    /** Returns the normalized value the running drag started from. */
    float getStartValue() const noexcept { return startValue; }

    /** Sets a normalized value, inside the running drag or in a gesture of its own.

        @param normalizedValue The value to set
        @param joinsLastStep   Outside a drag, true to undo it together with the step
                               recorded just before, for a change made of many parameters
    */
    void setNormalized (float normalizedValue, bool joinsLastStep = false);

    /** Returns the parameter to its default, in a gesture of its own. */
    void resetToDefault();

private:
    void record (float valueBefore, bool joinsLastStep = false);

    yup::Component& owner;
    yup::AudioParameter::Ptr parameter;
    float startValue = 0.0f;
    bool active = false;
};
