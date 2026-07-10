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
AnimationEasing::AnimationEasing (float x1, float y1, float x2, float y2) noexcept
    : cx1 (x1)
    , cy1 (y1)
    , cx2 (x2)
    , cy2 (y2)
{
    linearCached = ! holdFrame
                && std::abs (cx1 - cy1) < 1e-5f
                && std::abs (cx2 - cy2) < 1e-5f;
}

//==============================================================================
float AnimationEasing::evaluate (float t) const noexcept
{
    if (holdFrame || t <= 0.0f)
        return 0.0f;

    if (t >= 1.0f)
        return 1.0f;

    if (linearCached)
        return t;

    if (tableDirty)
        buildTable();

    return calcBezier (getTForX (t), cy1, cy2);
}

bool AnimationEasing::isLinear() const noexcept
{
    return ! holdFrame
        && std::abs (cx1 - cy1) < 1e-5f
        && std::abs (cx2 - cy2) < 1e-5f;
}

bool AnimationEasing::isHold() const noexcept
{
    return holdFrame;
}

//==============================================================================
AnimationEasing AnimationEasing::linear() noexcept { return AnimationEasing (0.0f, 0.0f, 1.0f, 1.0f); }

AnimationEasing AnimationEasing::easeIn() noexcept { return AnimationEasing (0.42f, 0.0f, 1.0f, 1.0f); }

AnimationEasing AnimationEasing::easeOut() noexcept { return AnimationEasing (0.0f, 0.0f, 0.58f, 1.0f); }

AnimationEasing AnimationEasing::easeInOut() noexcept { return AnimationEasing (0.42f, 0.0f, 0.58f, 1.0f); }

AnimationEasing AnimationEasing::hold() noexcept
{
    AnimationEasing e;
    e.holdFrame = true;
    e.linearCached = false;
    return e;
}

AnimationEasing AnimationEasing::fromLottieTangents (Point<float> outTangent,
                                                     Point<float> inTangent) noexcept
{
    return AnimationEasing (outTangent.getX(), outTangent.getY(), inTangent.getX(), inTangent.getY());
}

//==============================================================================
bool AnimationEasing::operator== (const AnimationEasing& o) const noexcept
{
    return holdFrame == o.holdFrame
        && std::abs (cx1 - o.cx1) < 1e-6f
        && std::abs (cy1 - o.cy1) < 1e-6f
        && std::abs (cx2 - o.cx2) < 1e-6f
        && std::abs (cy2 - o.cy2) < 1e-6f;
}

bool AnimationEasing::operator!= (const AnimationEasing& o) const noexcept
{
    return ! (*this == o);
}

//==============================================================================
// Private helpers - cubic bezier evaluation matching CSS/rlottie VInterpolator

float AnimationEasing::calcBezier (float t, float a1, float a2) noexcept
{
    // B(t) = 3*t*(1-t)^2*a1 + 3*t^2*(1-t)*a2 + t^3
    return ((1.0f - 3.0f * a2 + 3.0f * a1) * t + (3.0f * a2 - 6.0f * a1)) * t * t
         + 3.0f * a1 * t;
}

float AnimationEasing::getSlope (float t, float a1, float a2) noexcept
{
    return 3.0f * (1.0f - 3.0f * a2 + 3.0f * a1) * t * t
         + 2.0f * (3.0f * a2 - 6.0f * a1) * t
         + 3.0f * a1;
}

void AnimationEasing::buildTable() const noexcept
{
    for (int i = 0; i < kTableSize; ++i)
        sampleTable[i] = calcBezier (static_cast<float> (i) / static_cast<float> (kTableSize - 1), cx1, cx2);

    tableDirty = false;
}

float AnimationEasing::getTForX (float x) const noexcept
{
    // Direct index lookup instead of linear scan
    const float step = 1.0f / static_cast<float> (kTableSize - 1);
    int guess = static_cast<int> (x * static_cast<float> (kTableSize - 1));
    guess = jlimit (0, kTableSize - 2, guess);
    while (guess > 0 && sampleTable[guess] > x)
        --guess;
    while (guess < kTableSize - 2 && sampleTable[guess + 1] < x)
        ++guess;

    const float startT = static_cast<float> (guess) * step;
    const float dist = (x - sampleTable[guess]) / (sampleTable[guess + 1] - sampleTable[guess]);
    float t = startT + dist * step;

    // Newton-Raphson refinement
    for (int i = 0; i < 4; ++i)
    {
        const float slope = getSlope (t, cx1, cx2);
        if (slope < 1e-7f)
            break;
        t -= (calcBezier (t, cx1, cx2) - x) / slope;
    }

    return t;
}

} // namespace yup
