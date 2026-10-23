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

#include "../dsp/SynthFMParameters.h"
#include "ParameterKnob.h"

#include <array>
#include <memory>

//==============================================================================
/** The FM matrix as a grid of cells: each row is an operator, each column the operator
    modulating it, the diagonal its feedback. A cell shows its amount inside its well.

    @see SynthFMParameters
*/
class SynthFMPanel : public yup::Component
{
public:
    explicit SynthFMPanel (const SynthFMParameters& parametersToEdit);

    /** Shows changes made by the host, a preset or a state load. */
    void syncFromParameters();

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    static constexpr float labelSize = 18.0f;
    static constexpr float hintHeight = 30.0f;

    yup::Rectangle<float> grid;
    std::array<std::array<std::unique_ptr<ParameterKnob>, SynthExample::oscillatorCount>, SynthExample::oscillatorCount> knobs;
};
