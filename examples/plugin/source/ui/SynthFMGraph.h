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

#include "../dsp/SynthConstants.h"

#include <yup_gui/yup_gui.h>

#include <array>

//==============================================================================
/** The FM matrix drawn as a graph inside a sunken display, like the envelope and LFO
    displays: a node per operator, an arc per link sized by its
    amount and pointing at the operator it modulates, a loop for feedback, and the
    operators that are heard joined to an output bus.

    Arcs from a higher operator to a lower one run above the nodes, the others below.

    @see SynthFMPanel
*/
class SynthFMGraph : public yup::Component
{
public:
    using Row = std::array<float, SynthExample::oscillatorCount>;

    /** What the graph shows, every value normalized. */
    struct State
    {
        std::array<Row, SynthExample::oscillatorCount> amounts {}; /**< amounts[carrier][modulator]. */
        Row levels {};
        std::array<bool, SynthExample::oscillatorCount> enabled {};

        bool operator== (const State&) const = default;
    };

    SynthFMGraph();

    /** Shows a new state, repainting only when it changed. */
    void setState (const State& newState);

    /** Places the nodes at these horizontal positions, to line up with the matrix columns. */
    void setNodePositions (const Row& positions);

    void paint (yup::Graphics& g) override;

private:
    static constexpr float nodeRadius = 10.0f;

    State state;
    Row nodeX {};
};
