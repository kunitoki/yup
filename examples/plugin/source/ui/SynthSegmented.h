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
#include <initializer_list>
#include <vector>

//==============================================================================
/** A row, or a column, of segments choosing one of them, the chosen one raised out of a
    sunken track.

    A segment shows its icon, a path drawn in a unit square, when it has one, and its
    text otherwise.

    @see ParameterSegmented
*/
class SynthSegmented : public yup::Component
{
public:
    /** One choice. */
    struct Segment
    {
        yup::String text;
        yup::Path icon; /**< In the unit square; drawn instead of the text when not empty. */
    };

    /** Creates the selector, its segments side by side or, when vertical, stacked. */
    explicit SynthSegmented (std::vector<Segment> segmentsToShow, bool isVertical = false);

    /** Returns the chosen segment, or -1 before one is chosen. */
    int getSelectedIndex() const noexcept { return selectedIndex; }

    /** Shows a segment as chosen, without calling onSelect.
        @returns true when the chosen segment changed.
    */
    bool setSelectedIndex (int index);

    /** Called with the segment the user clicked. */
    std::function<void (int)> onSelect;

    /** Builds an icon from straight lines through points of the unit square. */
    static yup::Path makePolyline (std::initializer_list<yup::Point<float>> points);

    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    yup::Rectangle<float> getSegmentBounds (int index) const;

    std::vector<Segment> segments;
    int selectedIndex = -1;
    bool vertical = false;
};
