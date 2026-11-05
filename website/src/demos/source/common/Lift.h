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

#include <cmath>
#include <optional>

//==============================================================================
/**
    How far a control has lifted from flat 2D into 3D, from 0 to 1.

    The lift is full over the control and fades to 0 in a rounded band around it, so the fade never
    overlaps the control and its corners are rounded rather than square. The band's width scales with
    the window's shorter side, so it stays proportional on any window size. The value follows the
    target smoothly so a control never snaps between 2D and 3D. Without a pointer everything settles
    flat.
*/
class Lift
{
public:
    /** Moves the lift toward the target for the pointer.

        @p area is the control (for a demo whose solid is smaller than its flat panel, pass the
        solid's bounds so the fade rings it without overlapping it) and @p bounds the whole demo, in
        the same coordinates. The band around @p area scales with the shorter side of @p bounds. */
    float update (std::optional<yup::Point<float>> pointer, yup::Rectangle<float> area, yup::Rectangle<float> bounds, float deltaSeconds)
    {
        const auto reference = yup::jmax (1.0f, yup::jmin (bounds.getWidth(), bounds.getHeight()));
        const auto target = pointer.has_value() ? 1.0f - smoothstep (fullLiftFraction * reference, noLiftFraction * reference, distanceTo (area, *pointer)) : 0.0f;

        value += (target - value) * (1.0f - std::exp (-deltaSeconds / timeConstantSeconds));

        // Arrive exactly, so a control at rest stops changing and stops repainting.
        if (std::abs (target - value) < 0.0005f)
            value = target;

        return value;
    }

    /** Returns the current lift. */
    float get() const noexcept { return value; }

private:
    static constexpr float fullLiftFraction = 0.04f; // fully lifted within this share of the shorter side
    static constexpr float noLiftFraction = 0.33f;   // flat beyond this share, measured around the control
    static constexpr float timeConstantSeconds = 0.18f;

    /** Distance from @p point to the nearest edge of @p area, 0 inside it, rounded at the corners. */
    static float distanceTo (yup::Rectangle<float> area, yup::Point<float> point)
    {
        const auto dx = yup::jmax (area.getX() - point.getX(), 0.0f, point.getX() - area.getRight());
        const auto dy = yup::jmax (area.getY() - point.getY(), 0.0f, point.getY() - area.getBottom());
        return std::sqrt (dx * dx + dy * dy);
    }

    static float smoothstep (float edge0, float edge1, float x)
    {
        const auto t = yup::jlimit (0.0f, 1.0f, (x - edge0) / (edge1 - edge0));
        return t * t * (3.0f - 2.0f * t);
    }

    float value = 0.0f;
};
