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
#include <vector>

//==============================================================================
/** The instrument's rotary knob: a dark well, an accent value arc and, while the knob is
    hovered or dragged, a pill with the value as text.

    It also carries the modulation arcs drawn around the value, one per route reaching
    the control, in the route source's color.

    @see ParameterKnob
*/
class SynthKnob : public yup::Slider
{
public:
    /** One route drawn around the knob. */
    struct ModulationArc
    {
        float depth = 0.0f; /**< Normalized, -1 to 1, one full knob sweep per unit. */
        yup::Color color;
    };

    SynthKnob();

    /** Returns the text of the value pill. */
    std::function<yup::String()> getValueText;

    /** Keeps the value pill shown, for instance while a drag continues outside the knob. */
    void setShowingValue (bool shouldShowValue);

    /** True while the value pill is shown. */
    bool isShowingValue() const noexcept { return showingValue || isMouseOver(); }

    /** Replaces the modulation arcs. */
    void setModulationArcs (std::vector<ModulationArc> newArcs);

    /** Returns the modulation arcs. */
    const std::vector<ModulationArc>& getModulationArcs() const noexcept { return modulationArcs; }

private:
    static void paintKnob (yup::Graphics& g, const yup::ApplicationTheme& theme, const yup::Slider& slider);

    std::vector<ModulationArc> modulationArcs;
    bool showingValue = false;
};
