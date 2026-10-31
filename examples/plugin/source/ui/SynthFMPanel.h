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

#include "../dsp/SynthParameters.h"
#include "ParameterKnob.h"
#include "SynthFMGraph.h"

#include <array>
#include <memory>

//==============================================================================
/** The FM matrix as a grid of cells: each row is an operator, each column the operator
    modulating it, the diagonal its feedback, and a last column how loud each operator
    is heard. A cell shows its amount inside its well.

    Under the grid the same matrix is drawn as a graph. The algorithm button, placed in
    the section's strip, wires the matrix as one of the eight TX81Z algorithms, and the
    reset button beside it returns every amount to its default, each in a single undo
    step.

    @see SynthFMParameters, SynthFMGraph, SynthAlgorithm
*/
class SynthFMPanel : public yup::Component
{
public:
    explicit SynthFMPanel (const SynthParameters& parametersToEdit);

    /** Shows changes made by the host, a preset or a state load. */
    void syncFromParameters();

    /** Returns the button that opens the algorithm menu, for the section's strip. */
    yup::Component& getAlgorithmButton() noexcept { return algorithmButton; }

    /** Returns the button that resets the matrix, for the section's strip. */
    yup::Component& getResetButton() noexcept { return resetButton; }

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    static constexpr float labelSize = 18.0f;
    static constexpr float graphShare = 0.32f;
    static constexpr float newLinkAmount = 1.0f;  /**< In periods, for a link an algorithm adds. */
    static constexpr float newOutputLevel = 0.8f; /**< Normalized, for an operator an algorithm makes heard. */

    using KnobRow = std::array<std::unique_ptr<ParameterKnob>, SynthExample::oscillatorCount>;

    SynthFMGraph::State readState() const;
    int findAlgorithm() const;
    void showAlgorithmMenu();
    void applyAlgorithm (int index);
    void resetMatrix();

    const SynthParameters& parameters;
    yup::Rectangle<float> grid;
    std::array<KnobRow, SynthExample::oscillatorCount> knobs;
    KnobRow levelKnobs;
    SynthFMGraph graph;
    yup::TextButton algorithmButton { "Algorithm" };
    yup::TextButton resetButton { "Reset" };
    yup::Font captionFont;
};
