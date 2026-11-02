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

#include "../dsp/SynthRoutes.h"
#include "SynthIconButton.h"
#include "SynthModulationTarget.h"
#include "SynthSliderField.h"

#include <array>
#include <memory>
#include <utility>
#include <vector>

//==============================================================================
/** The routes reaching one control, in a card shown next to it on a right click: per
    route a dot in the source's color, the source's name, the depth and a remove button.
    A control no route reaches shows how to add one.

    The panel covers the interface it is in and closes on a click outside the card, on
    escape (see SynthInterface) and when the control is hidden. Every edit is one step of
    the undo history.

    @see SynthModulationTarget, SynthModulationRow
*/
class SynthModulationPanel : public yup::Component
{
public:
    using Depths = std::array<yup::AudioParameter::Ptr, SynthExample::modulationSlots>;

    SynthModulationPanel (SynthRoutes& routesToEdit, const Depths& depthParameters);
    ~SynthModulationPanel() override;

    /** Shows the routes reaching a target's parameter next to the target. */
    void show (SynthModulationTarget& target);

    /** Hides the panel. */
    void close();

    /** True while the panel is shown. */
    bool isOpen() const { return isVisible(); }

    /** Shows the macros under the names the patch gives them. */
    void setMacroNames (const std::array<yup::String, SynthExample::macroCount>& macroNames);

    /** Follows changes to the routes and depths, and closes once the control is hidden. */
    void syncFromRoutes();

    void resized() override;
    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    class Row;

    static constexpr float cardWidth = 250.0f;
    static constexpr float lineHeight = 24.0f;
    static constexpr float cardGap = 9.0f;
    static constexpr float pointerSize = 7.0f;

    /** The slots reaching the parameter, with their sources, in slot order. */
    using Routes = std::vector<std::pair<int, SynthModulationSource>>;

    Routes findRoutes() const;
    void rebuild (const Routes& routesToShow);
    void placeCard();

    SynthRoutes& routes;
    const Depths& depths;
    yup::WeakReference<yup::Component> control;
    yup::AudioParameter::Ptr parameter;
    yup::StringArray sourceNames;
    std::vector<std::unique_ptr<Row>> rows;
    Routes shownRoutes;
    yup::Rectangle<float> card;
    float pointerY = 0.0f;
    bool pointsLeft = true;
    yup::Font titleFont;
    yup::Font textFont;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthModulationPanel)
};
