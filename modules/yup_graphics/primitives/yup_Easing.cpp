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

namespace yup
{

//==============================================================================
Easing::Easing (float x1, float y1, float x2, float y2) noexcept
    : curve ({ 0.0f, 0.0f }, { x1, y1 }, { x2, y2 }, { 1.0f, 1.0f })
{
}

//==============================================================================
float Easing::evaluate (float t) const noexcept
{
    if (holdFrame || t <= 0.0f)
        return 0.0f;

    if (t >= 1.0f)
        return 1.0f;

    if (isLinear())
        return t;

    return curve.pointAt (solveCurveX (t)).getY();
}

bool Easing::isLinear() const noexcept
{
    return ! holdFrame
        && std::abs (getX1() - getY1()) < 1e-5f
        && std::abs (getX2() - getY2()) < 1e-5f;
}

float Easing::solveCurveX (float x) const noexcept
{
    constexpr float epsilon = 1e-6f;

    float t = x;

    for (int i = 0; i < 8; ++i)
    {
        const float error = curve.pointAt (t).getX() - x;
        if (std::abs (error) < epsilon)
            return t;

        const float slope = curve.derivative (t).getX();
        if (std::abs (slope) < epsilon)
            break;

        t -= error / slope;
        if (t < 0.0f || t > 1.0f)
            break;
    }

    // Newton stalled on a flat slope or left the curve, bisect instead
    float lower = 0.0f;
    float upper = 1.0f;
    t = x;

    for (int i = 0; i < 32; ++i)
    {
        const float currentX = curve.pointAt (t).getX();
        if (std::abs (currentX - x) < epsilon)
            break;

        (currentX < x ? lower : upper) = t;
        t = (lower + upper) * 0.5f;
    }

    return t;
}

//==============================================================================
Easing Easing::linear() noexcept { return Easing (0.0f, 0.0f, 1.0f, 1.0f); }

Easing Easing::easeIn() noexcept { return Easing (0.42f, 0.0f, 1.0f, 1.0f); }

Easing Easing::easeOut() noexcept { return Easing (0.0f, 0.0f, 0.58f, 1.0f); }

Easing Easing::easeInOut() noexcept { return Easing (0.42f, 0.0f, 0.58f, 1.0f); }

Easing Easing::hold() noexcept
{
    Easing e;
    e.holdFrame = true;
    return e;
}

Easing Easing::fromTangents (Point<float> outTangent, Point<float> inTangent) noexcept
{
    return Easing (outTangent.getX(), outTangent.getY(), inTangent.getX(), inTangent.getY());
}

//==============================================================================
bool Easing::operator== (const Easing& o) const noexcept
{
    return holdFrame == o.holdFrame
        && std::abs (getX1() - o.getX1()) < 1e-6f
        && std::abs (getY1() - o.getY1()) < 1e-6f
        && std::abs (getX2() - o.getX2()) < 1e-6f
        && std::abs (getY2() - o.getY2()) < 1e-6f;
}

bool Easing::operator!= (const Easing& o) const noexcept
{
    return ! (*this == o);
}

} // namespace yup
