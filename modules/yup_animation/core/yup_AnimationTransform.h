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
/** Bundles all Lottie transform channels as animated properties.

    Call toAffineTransform(frameNo) to collapse all channels into a single
    AffineTransform for rendering. The composition order matches Lottie:
    anchor correction → position → rotation → scale.
*/
class YUP_API AnimationTransform
{
public:
    //==============================================================================
    Vec2Property anchor { AnimationProperty<Point<float>>::staticValue ({ 0.0f, 0.0f }) };
    Vec2Property position { AnimationProperty<Point<float>>::staticValue ({ 0.0f, 0.0f }) };
    SizeProperty scale { AnimationProperty<Size<float>>::staticValue ({ 100.0f, 100.0f }) };
    FloatProperty rotation { AnimationProperty<float>::staticValue (0.0f) };
    FloatProperty opacity { AnimationProperty<float>::staticValue (100.0f) };
    FloatProperty skew { AnimationProperty<float>::staticValue (0.0f) };
    FloatProperty skewAxis { AnimationProperty<float>::staticValue (0.0f) };

    /** When true, the layer has separate X/Y position channels. */
    bool separatePosition = false;
    FloatProperty positionX { AnimationProperty<float>::staticValue (0.0f) };
    FloatProperty positionY { AnimationProperty<float>::staticValue (0.0f) };

    //==============================================================================
    /** Evaluates all channels and returns the composed AffineTransform at frameNo. */
    [[nodiscard]] AffineTransform toAffineTransform (float frameNo) const;

    /** Returns opacity in [0, 1] at the given frame. */
    [[nodiscard]] float opacityAt (float frameNo) const;

    /** Returns true when all channels are static (no keyframe animation). */
    [[nodiscard]] bool isStatic() const noexcept;
};

} // namespace yup
