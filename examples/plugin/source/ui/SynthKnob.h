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
#include <optional>
#include <vector>

//==============================================================================
/** The instrument's rotary knob: a dark well with a pointer and an accent value arc.

    It also carries the modulation arcs drawn around the value, one per route reaching
    the control, in the route source's color. Dragging with shift held moves the value
    ten times finer, and a click with command or control held returns the default.

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
        bool bipolar = false; /**< Reaches depth either side of the value rather than one way. */

        bool operator== (const ModulationArc&) const = default;
    };

    SynthKnob();

    /** Called when the mouse enters or leaves the knob, see isMouseOver(). */
    std::function<void()> onHoverChanged;

    /** Called on a right click; when set, the click starts no drag. */
    std::function<void()> onContextClick;

    /** Called on a double-click; when set, it replaces the slider's return to the default. */
    std::function<void()> onDoubleClick;

    /** When set, its text is drawn inside the well in place of the pointer, for knobs too
        small for a caption. */
    std::function<yup::String()> getWellText;

    /** True for a click that returns a control to its default: command or control held. */
    static bool isResetClick (const yup::MouseEvent& event);

    /** Replaces the modulation arcs. */
    void setModulationArcs (std::vector<ModulationArc> newArcs);

    /** Marks the value the modulation currently plays, normalized, or hides the mark. */
    void setModulatedValue (std::optional<float> newModulatedValue);

    /** Returns the modulation arcs. */
    const std::vector<ModulationArc>& getModulationArcs() const noexcept { return modulationArcs; }

    /** Draws the value arc in the color of a modulation source family (see
        SynthTheme::sourceAccent()) rather than the section accent. */
    void setSourceAccent (int family);

    void mouseEnter (const yup::MouseEvent& event) override;
    void mouseExit (const yup::MouseEvent& event) override;
    void mouseDown (const yup::MouseEvent& event) override;
    void mouseDrag (const yup::MouseEvent& event) override;
    void mouseUp (const yup::MouseEvent& event) override;
    void mouseDoubleClick (const yup::MouseEvent& event) override;

private:
    static constexpr double fineDragScale = 0.1;

    static void paintKnob (yup::Graphics& g, const yup::ApplicationTheme& theme, const yup::Slider& slider);

    std::vector<ModulationArc> modulationArcs;
    std::optional<int> accentFamily;
    std::optional<float> modulatedValue;
    yup::Font wellFont;
    yup::Point<float> dragAnchor;
    double dragAnchorValue = 0.0;
    bool fineDrag = false;
};
