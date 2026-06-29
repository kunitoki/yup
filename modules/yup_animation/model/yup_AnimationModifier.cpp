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
// AnimationTrim

namespace
{

[[nodiscard]] AnimationTrim::TrimSegment noLoop (float start, float end) noexcept
{
    return { jmin (start, end), jmax (start, end) };
}

[[nodiscard]] AnimationTrim::TrimSegment loop (float start, float end) noexcept
{
    return { jmax (start, end), jmin (start, end) };
}

} // namespace

AnimationTrim::TrimSegment AnimationTrim::getSegment (float frameNo) const
{
    const float rawStart = start.getValueAt (frameNo) / 100.0f;
    const float rawEnd = end.getValueAt (frameNo) / 100.0f;
    const float rawOffset = std::fmod (offset.getValueAt (frameNo), 360.0f) / 360.0f;

    if (std::abs (rawEnd - rawStart) >= 1.0f)
        return { 0.0f, 1.0f };

    float startWithOffset = rawStart + rawOffset;
    float endWithOffset = rawEnd + rawOffset;

    if (rawOffset > 0.0f)
    {
        if (startWithOffset <= 1.0f && endWithOffset <= 1.0f)
            return noLoop (startWithOffset, endWithOffset);

        if (startWithOffset > 1.0f && endWithOffset > 1.0f)
            return noLoop (startWithOffset - 1.0f, endWithOffset - 1.0f);

        return startWithOffset > 1.0f ? loop (startWithOffset - 1.0f, endWithOffset)
                                      : loop (startWithOffset, endWithOffset - 1.0f);
    }

    if (startWithOffset >= 0.0f && endWithOffset >= 0.0f)
        return noLoop (startWithOffset, endWithOffset);

    if (startWithOffset < 0.0f && endWithOffset < 0.0f)
        return noLoop (1.0f + startWithOffset, 1.0f + endWithOffset);

    return startWithOffset < 0.0f ? loop (1.0f + startWithOffset, endWithOffset)
                                  : loop (startWithOffset, 1.0f + endWithOffset);
}

//==============================================================================
// AnimationRepeater

int AnimationRepeater::copiesAt (float frameNo) const
{
    return jmax (1, static_cast<int> (copies.getValueAt (frameNo)));
}

float AnimationRepeater::offsetAt (float frameNo) const
{
    return offset.getValueAt (frameNo);
}

float AnimationRepeater::startOpacityAt (float frameNo) const
{
    return jlimit (0.0f, 1.0f, startOpacity.getValueAt (frameNo) / 100.0f);
}

float AnimationRepeater::endOpacityAt (float frameNo) const
{
    return jlimit (0.0f, 1.0f, endOpacity.getValueAt (frameNo) / 100.0f);
}

//==============================================================================
// AnimationMask

Path AnimationMask::shapeAt (float frameNo) const
{
    return shape.getValueAt (frameNo).toPath();
}

float AnimationMask::opacityAt (float frameNo) const
{
    return jlimit (0.0f, 1.0f, opacity.getValueAt (frameNo) / 100.0f);
}

//==============================================================================
// AnimationRoundedCorner

float AnimationRoundedCorner::radiusAt (float frameNo) const
{
    return jlimit (0.0f, 1.0f, radius.getValueAt (frameNo) / 100.0f);
}

} // namespace yup
