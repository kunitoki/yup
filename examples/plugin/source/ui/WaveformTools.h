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

#include "../dsp/SynthPartials.h"

//==============================================================================
/** What a stroke in the waveform editor draws. */
enum class WaveformDrawTool
{
    brush,  /**< Freehand. */
    line,   /**< A straight line from the press to the pointer. */
    arc,    /**< Half sine arcs rising from the press level to the pointer's. */
    zigzag, /**< Triangles rising from the press level to the pointer's. */
    steps,  /**< A staircase from the press level to the pointer's. */
    noise   /**< Random values around the press level, as far as the pointer's. */
};

/** What a drag in the partials view changes. */
enum class PartialsTool
{
    magnitude,
    phase
};

/** Changes made to every partial at once, exact on the coefficients. */
enum class PartialsOperation
{
    invert,         /**< Flips the cycle upside down. */
    reverse,        /**< Plays the cycle backwards. */
    mirror,         /**< Replaces the second half of the cycle with the first, played backwards. */
    oddOnly,        /**< Keeps the odd harmonics, the fundamental included. */
    evenOnly,       /**< Keeps the even harmonics. */
    randomizePhases /**< Keeps every magnitude and draws a new phase for each. */
};

//==============================================================================
/** Draws one of the span tools into a cycle of samples.

    The span runs from the press to the pointer, either way. With a grid it grows to
    whole cells, and arcs, zigzags and noise repeat once per cell while steps climb one
    level per cell; without a grid the span is a single cell.

    @param cycle         The samples of one period, from -1 to 1
    @param tool          Any tool but the brush, which the editor draws itself
    @param fromIndex     The sample the stroke was pressed on
    @param fromValue     The level the stroke was pressed at
    @param toIndex       The sample under the pointer
    @param toValue       The level of the pointer
    @param gridDivisions Cells per period, or 0 without a grid
    @param seed          Keeps the noise of one stroke still while the pointer moves
*/
void stampWaveformShape (yup::Span<float> cycle,
                         WaveformDrawTool tool,
                         int fromIndex,
                         float fromValue,
                         int toIndex,
                         float toValue,
                         int gridDivisions,
                         yup::int64 seed) noexcept;

/** Applies an operation to every partial of a snapshot. */
void applyPartialsOperation (SynthPartials::Snapshot& partials, PartialsOperation operation, yup::Random& random);
