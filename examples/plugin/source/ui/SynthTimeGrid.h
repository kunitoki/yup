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
/** The time grids and snapping of the envelope and LFO curve editors.

    A curve stores times in its own units, which the editor turns into seconds, or into
    quarter note beats when the curve follows the tempo. Bars and beats are counted in
    the host's meter; a curve's beat length never depends on it.
*/
namespace SynthTimeGrid
{
/** How a curve's times read. */
struct Timing
{
    bool musical = false;    /**< Times are quarter note beats instead of seconds. */
    double unitLength = 1.0; /**< Seconds, or quarter note beats, per curve time unit. */
    int numerator = 4;
    int denominator = 4;

    bool operator== (const Timing&) const = default;

    /** Returns the length of a bar, in quarter note beats. */
    double getBarLength() const noexcept { return yup::jmax (1, numerator) * 4.0 / yup::jmax (1, denominator); }

    /** Returns the length of a beat of the meter, in quarter note beats. */
    double getBeatLength() const noexcept { return 4.0 / yup::jmax (1, denominator); }
};

/** One vertical line of a grid. */
struct Line
{
    float time = 0.0f; /**< In curve units. */
    bool major = false;
    yup::String label; /**< Only major lines have one. */
};

/** Returns the lines of a grid over [0, length] curve units drawn over a width, as dense
    as stays readable and never more than a few hundred. */
std::vector<Line> getLines (const Timing& timing, float length, float width);

/** Returns the snapping choices for seconds or musical timing, "Off" first. */
yup::StringArray getSnapNames (bool musical);

/** Moves a time in curve units to the nearest step of a snapping choice; choice 0, or an
    invalid one, leaves it alone. */
float snap (float time, const Timing& timing, int snapChoice);

/** Formats a time in curve units: milliseconds or seconds, or bar.beat.sixteenth. */
yup::String format (float time, const Timing& timing);

/** Draws grid lines and their labels in an area, at the positions toX() gives; a line
    toX() gives no position for is left out. */
void paint (yup::Graphics& g,
            const yup::Component& scope,
            const std::vector<Line>& lines,
            yup::Rectangle<float> area,
            const std::function<std::optional<float> (float)>& toX);

} // namespace SynthTimeGrid
