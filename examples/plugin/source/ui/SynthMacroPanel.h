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

#include "../RetraceProcessor.h"
#include "ParameterKnob.h"
#include "SynthFieldEditor.h"
#include "SynthModulationHandle.h"

#include <array>
#include <functional>
#include <memory>

//==============================================================================
/** The eight macro knobs, two columns of four, each named underneath.

    A macro is a parameter the host automates and a modulation source the routes read, so
    one knob can move any number of destinations. Its name belongs to the patch: a click
    on it starts editing, and each rename is one step of the undo history. The handle in the
    title above the knob is dragged onto a control to modulate it with the macro.

    @see SynthParameters, SynthModulationSource
*/
class SynthMacroPanel : public yup::Component
{
public:
    explicit SynthMacroPanel (RetraceProcessor& processorToControl);

    /** Shows the macros' values and names, wherever they were changed. */
    void syncFromProcessor();

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    static constexpr float nameHeight = 18.0f;
    static constexpr float handleSize = 16.0f;
    static constexpr float titleHeight = 18.0f;
    static constexpr int columns = 2;

    void writeName (int macro, const yup::String& text);
    static yup::String toStoredName (int macro, const yup::String& text);
    void finishName (int macro);

    RetraceProcessor& processor;
    std::array<std::unique_ptr<ParameterKnob>, SynthExample::macroCount> knobs;
    std::array<SynthFieldEditor, SynthExample::macroCount> names;
    std::array<std::unique_ptr<SynthModulationHandle>, SynthExample::macroCount> handles;
    std::array<yup::String, SynthExample::macroCount> namesBeforeEdit;
    yup::Font captionFont;
};
