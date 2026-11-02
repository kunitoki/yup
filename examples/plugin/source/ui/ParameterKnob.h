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
#include "ParameterTextEntry.h"
#include "SynthKnob.h"
#include "SynthModulationTarget.h"
#include <yup_audio_processors/yup_audio_processors.h>
#include <yup_gui/yup_gui.h>

#include <functional>

//==============================================================================
/** A rotary knob bound to an AudioParameter, with its caption underneath.

    The knob works in the parameter's normalized range, so any skew lives only in the
    parameter. While the knob is hovered or dragged, the caption shows the value text in
    place of the caption text. A double-click opens a field over the knob to type the value
    in, while the caption keeps the name. A drag is one host gesture and one undo step; any other change, such as a
    typed value, a reset click or the mouse wheel, is wrapped in a gesture of its own.

    @see ParameterChoice, SynthNumberField
*/
class ParameterKnob
    : public yup::Component
    , public SynthModulationTarget
{
public:
    ParameterKnob (const yup::String& caption, yup::AudioParameter::Ptr parameterToControl, const yup::Font& font);
    ~ParameterKnob() override;

    /** Shows the parameter's current value unless the knob is being dragged.

        @returns true when the shown value changed.
    */
    bool syncFromParameter();

    /** Binds another parameter, for a knob that follows a mode such as an LFO's sync. */
    void setParameter (yup::AudioParameter::Ptr newParameter);

    /** True while the knob is hovered or dragged and its value is shown in place of a name. */
    bool isShowingValue() const;

    /** Returns the value text the caption shows; the parameter's own text by default. */
    std::function<yup::String()> getValueText;

    yup::AudioParameter::Ptr getModulationParameter() const override { return parameter; }
    void showModulation (std::vector<SynthKnob::ModulationArc> arcs, std::optional<float> modulatedValue) override;

    /** Returns the knob, for its look options. */
    SynthKnob& getKnob() noexcept { return slider; }

    void resized() override;

private:
    static constexpr float captionHeight = 13.0f;
    static constexpr float textEntryHeight = 22.0f;

    void endGesture();
    void updateCaption();
    void showTextEntry();

    void bindSlider();
    float getCaptionHeight() const noexcept;

    yup::AudioParameter::Ptr parameter;
    ParameterGesture gesture;
    SynthKnob slider;
    yup::Label label;
    ParameterTextEntry textEntry { *this, gesture };
    yup::String captionText;
    std::optional<float> captionValue;
};
