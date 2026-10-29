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

#include "../dsp/SynthEffectsParameters.h"
#include "ParameterGesture.h"
#include "ParameterKnob.h"
#include "ParameterLed.h"

#include <memory>

//==============================================================================
/** The master effects: the chain on the left, the selected effect's controls on the right.

    Each row of the chain has the effect's switch. Clicking a row shows its effect,
    dragging it up or down moves the effect along the chain, as one undoable step.

    @see SynthEffectsParameters
*/
class SynthEffectsPage : public yup::Component
{
public:
    explicit SynthEffectsPage (const SynthEffectsParameters& parametersToEdit);
    ~SynthEffectsPage() override;

    /** Shows changes made by the host, a preset or a state load. */
    void syncFromParameters();

    void resized() override;
    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;
    void mouseDrag (const yup::MouseEvent& event) override;
    void mouseUp (const yup::MouseEvent& event) override;

private:
    class EffectPanel;

    static constexpr float maximumRowHeight = 28.0f;
    static constexpr float maximumListWidth = 120.0f;

    yup::Rectangle<float> getListBounds() const;
    yup::Rectangle<float> getRowBounds (int row) const;
    int getRowAt (float y) const;
    void selectEffect (SynthEffectType type);
    void layoutRows();

    const SynthEffectsParameters& parameters;
    SynthEffectOrder shownOrder;
    SynthEffectType selected = SynthEffectType::chorus;
    std::array<std::unique_ptr<ParameterLed>, SynthExample::effectCount> leds;
    std::array<std::unique_ptr<EffectPanel>, SynthExample::effectCount> panels;
    std::array<ParameterKnob*, 4> chorusGenericKnobs {};
    ParameterKnob* delayTimeKnob = nullptr;
    ParameterGesture orderGesture;
    int draggedRow = -1;
    yup::Font captionFont;
};
