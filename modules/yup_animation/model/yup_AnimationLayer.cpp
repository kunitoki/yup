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

float AnimationLayer::DropShadow::opacityAt (float frameNo) const
{
    return jlimit (0.0f, 1.0f, opacity.getValueAt (frameNo) / 100.0f);
}

Point<float> AnimationLayer::DropShadow::offsetAt (float frameNo) const
{
    const float angle = degreesToRadians (direction.getValueAt (frameNo));
    const float offset = distance.getValueAt (frameNo);

    return { std::cos (angle) * offset, std::sin (angle) * offset };
}

float AnimationLayer::FillEffect::opacityAt (float frameNo) const
{
    const float rawOpacity = opacity.getValueAt (frameNo);
    const float normalizedOpacity = rawOpacity > 1.0f ? rawOpacity / 100.0f : rawOpacity;

    return jlimit (0.0f, 1.0f, normalizedOpacity);
}

Color AnimationLayer::FillEffect::colorAt (float frameNo) const
{
    return color.getValueAt (frameNo);
}

float AnimationLayer::localFrame (float compFrame) const noexcept
{
    if (timeRemap.has_value())
        return timeRemap->getValueAt (compFrame);

    if (std::abs (timeStretch) <= 1.0e-6f)
        return compFrame - startFrame;

    return (compFrame - startFrame) / timeStretch;
}

float AnimationLayer::localFrame (float compFrame, float frameRate) const noexcept
{
    if (timeRemap.has_value())
    {
        float remapFrame = compFrame;

        if (timeRemapLoopOutCycle && timeRemap->isAnimated())
        {
            const auto& keyframes = timeRemap->getKeyframes();
            if (keyframes.size() >= 2)
            {
                const float firstFrame = keyframes.front().frame;
                const float lastFrame = keyframes.back().frame;
                const float duration = lastFrame - firstFrame;

                if (duration > 1.0e-6f && remapFrame >= lastFrame)
                    remapFrame = firstFrame + std::fmod (remapFrame - firstFrame, duration);
            }
        }

        return timeRemap->getValueAt (remapFrame) * frameRate;
    }

    return localFrame (compFrame);
}

bool AnimationLayer::isVisibleAt (float compFrame) const noexcept
{
    return ! hidden && compFrame >= inFrame && compFrame < outFrame;
}

} // namespace yup
