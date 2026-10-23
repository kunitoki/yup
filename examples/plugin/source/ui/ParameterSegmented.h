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

#include "ParameterGesture.h"

#include <yup_gui/yup_gui.h>

#include <vector>

//==============================================================================
/** A row of segments choosing one value of a choice parameter, the chosen one raised
    out of a sunken track.

    Segment n picks the parameter's value n. A segment shows its icon, a path drawn in a
    unit square, when it has one, and its text otherwise.

    @see ParameterLed, ParameterChoice
*/
class ParameterSegmented : public yup::Component
{
public:
    /** One choice. */
    struct Segment
    {
        yup::String text;
        yup::Path icon; /**< In the unit square; drawn instead of the text when not empty. */
    };

    ParameterSegmented (yup::AudioParameter::Ptr choiceParameter, std::vector<Segment> segmentsToShow);

    /** Shows the parameter's current choice.
        @returns true when the shown choice changed.
    */
    bool syncFromParameter();

    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    int getSelectedIndex() const;
    yup::Rectangle<float> getSegmentBounds (int index) const;

    ParameterGesture gesture;
    std::vector<Segment> segments;
    int shownIndex = -1;
};
