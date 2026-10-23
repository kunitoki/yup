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

#include <functional>

//==============================================================================
/** A combo box bound to a stepped AudioParameter, with its caption above it.

    The items are the parameter's own value texts, one per step. An empty caption
    leaves the whole height to the box. A pick is one host gesture.

    @see ParameterKnob, ParameterSegmented
*/
class ParameterChoice : public yup::Component
{
public:
    ParameterChoice (const yup::String& caption, yup::AudioParameter::Ptr parameterToControl, const yup::Font& font);

    /** Called after the user picked an item, once the parameter holds it. */
    std::function<void()> onChange;

    /** Shows the parameter's current value.

        @returns true when the shown value changed.
    */
    bool syncFromParameter();

    void resized() override;

private:
    static constexpr float captionHeight = 13.0f;

    int getParameterItemId() const;

    yup::AudioParameter::Ptr parameter;
    yup::Label label;
    yup::ComboBox comboBox;
};
