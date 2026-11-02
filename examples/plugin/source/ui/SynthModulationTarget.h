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

#include "../dsp/SynthTypes.h"
#include "SynthKnob.h"
#include "SynthTheme.h"

#include <yup_audio_processors/yup_audio_processors.h>

#include <optional>
#include <vector>

//==============================================================================
/** A control that shows the modulation reaching its parameter: one range per route, in
    the color of the route's source, and the value the modulation plays at the moment.

    The interface finds every target under it and feeds them once per frame.

    @see ParameterKnob, SynthSliderField
*/
class SynthModulationTarget
{
public:
    virtual ~SynthModulationTarget() = default;

    /** Returns the parameter the control edits, whose routes it shows. */
    virtual yup::AudioParameter::Ptr getModulationParameter() const = 0;

    /** Shows the routes reaching the parameter and the value they play now, normalized,
        or no value when no note plays. */
    virtual void showModulation (std::vector<SynthKnob::ModulationArc> arcs, std::optional<float> modulatedValue) = 0;

protected:
    /** Asks the nearest SynthModulationPanelHost above a control to show the routes of this
        target, on a right click. */
    void requestModulationPanel (yup::Component& control);
};

//==============================================================================
/** The component that shows the routes reaching a control in a panel, on a right click.

    @see SynthModulationTarget, SynthModulationPanel
*/
class SynthModulationPanelHost
{
public:
    virtual ~SynthModulationPanelHost() = default;

    /** Shows the routes reaching a target, next to it. */
    virtual void showModulationPanel (SynthModulationTarget& target) = 0;
};

inline void SynthModulationTarget::requestModulationPanel (yup::Component& control)
{
    if (auto* host = control.getParentComponentWithType<SynthModulationPanelHost>())
        host->showModulationPanel (*this);
}

/** Returns the color a modulation source is drawn in, inside the interface a component is
    in: the envelopes' and LFOs' section accents, then the other accents for the note
    sources and the random one. */
inline yup::Color getSourceColor (const yup::Component& scope, SynthModulationSource source)
{
    if (getSourceMacro (source) >= 0)
        return SynthTheme::sectionAccent (scope, SynthTheme::Section::macros);

    const auto family = getSourceEnvelope (source) >= 0 ? 0
                      : getSourceLFO (source) >= 0    ? 1
                      : source == SynthModulationSource::random ? 3
                                                                 : 2;

    return SynthTheme::sourceAccent (scope, family);
}
