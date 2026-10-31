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

#include <yup_core/yup_core.h>

#include <cmath>

//==============================================================================
/** Text for a parameter value, shared by the knobs and by the host's parameter list. */
namespace SynthFormat
{
inline yup::String plain (double value, int decimals) { return yup::String (value, decimals); }
inline yup::String hertz (double value) { return std::round (value) >= 1000.0 ? yup::String (value / 1000.0, 2) + " kHz" : (std::round (value * 100.0) < 10000.0 ? yup::String (value, 2) : yup::String (static_cast<int> (std::round (value)))) + " Hz"; }
inline yup::String percent (double value) { return yup::String (static_cast<int> (std::round (value * 100.0))) + " %"; }
inline yup::String cents (double value) { return yup::String (static_cast<int> (std::round (value))) + " ct"; }
inline yup::String octaves (double value) { return yup::String (static_cast<int> (std::round (value))) + " oct"; }
inline yup::String milliseconds (double seconds) { return std::round (seconds * 1000.0) >= 1000.0 ? yup::String (seconds, 2) + " s" : yup::String (static_cast<int> (std::round (seconds * 1000.0))) + " ms"; }
inline yup::String decibels (double value) { return yup::String (value >= 0.05 ? "+" : "") + yup::String (value, 1) + " dB"; }
inline yup::String ratio (double value) { return yup::String (value, 2) + " x"; }
inline yup::String count (double value) { return yup::String (static_cast<int> (std::round (value))); }
inline yup::String bipolar (double value)
{
    const auto rounded = static_cast<int> (std::round (value * 100.0));
    return yup::String (rounded >= 0 ? "+" : "") + yup::String (rounded) + " %";
}

/** Reads the text of any of the formatters above back into the value it shows, scaled by its unit. */
inline double parse (const yup::String& text)
{
    const auto trimmed = text.trim();
    const auto value = trimmed.getDoubleValue();

    if (trimmed.endsWith ("kHz"))
        return value * 1000.0;

    if (trimmed.endsWith ("ms"))
        return value * 0.001;

    if (trimmed.endsWith ("%"))
        return value * 0.01;

    return value;
}
} // namespace SynthFormat
