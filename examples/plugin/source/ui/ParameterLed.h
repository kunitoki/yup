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

#include <yup_gui/yup_gui.h>

#include <optional>

//==============================================================================
/** A small square light showing an on/off parameter, which a click toggles, with an
    optional caption to its left.

    Without a parameter the light is locked on: it is drawn lit and ignores the mouse,
    for a module that is always enabled.

    @see ParameterSegmented
*/
class ParameterLed : public yup::Component
{
public:
    explicit ParameterLed (yup::AudioParameter::Ptr toggleParameter, const yup::String& caption = {});

    /** Lights the LED in this color rather than the theme accent. */
    void setAccent (yup::Color newAccent);

    /** True when the parameter is on, or when the LED is locked on. */
    bool isOn() const;

    /** Shows the parameter's current state.
        @returns true when the shown state changed.
    */
    bool syncFromParameter();

    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    static constexpr float ledSize = 9.0f;

    ParameterGesture gesture;
    yup::String caption;
    std::optional<yup::Color> accentOverride;
    bool shownOn = false;
};
