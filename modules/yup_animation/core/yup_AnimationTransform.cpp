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

AffineTransform AnimationTransform::toAffineTransform (float frameNo) const
{
    const Point<float> a = anchor.getValueAt (frameNo);
    const Size<float> s = scale.getValueAt (frameNo);
    const float r = rotation.getValueAt (frameNo);
    const float sk = skew.getValueAt (frameNo);
    const float sa = skewAxis.getValueAt (frameNo);

    Point<float> p;
    if (separatePosition)
        p = { positionX.getValueAt (frameNo), positionY.getValueAt (frameNo) };
    else
        p = position.getValueAt (frameNo);

    // Compose: translate(p) * rotate(r) * skew * scale(s/100) * translate(-a)
    AffineTransform t;
    t = t.translated (-a.getX(), -a.getY());
    t = t.scaled (s.getWidth() / 100.0f, s.getHeight() / 100.0f);

    if (std::abs (sk) > 1e-5f)
    {
        const float skRad = degreesToRadians (sk);
        const float saRad = degreesToRadians (sa);
        const AffineTransform skewMtx = AffineTransform (1.0f, std::tan (skRad), 0.0f, 0.0f, 1.0f, 0.0f);
        const AffineTransform rot1 = AffineTransform::rotation (-saRad);
        const AffineTransform rot2 = AffineTransform::rotation (saRad);
        t = t.followedBy (rot1).followedBy (skewMtx).followedBy (rot2);
    }

    t = t.rotated (degreesToRadians (r));
    t = t.translated (p.getX(), p.getY());
    return t;
}

float AnimationTransform::opacityAt (float frameNo) const
{
    return jlimit (0.0f, 1.0f, opacity.getValueAt (frameNo) / 100.0f);
}

bool AnimationTransform::isStatic() const noexcept
{
    return anchor.isStatic()
        && position.isStatic()
        && scale.isStatic()
        && rotation.isStatic()
        && opacity.isStatic()
        && skew.isStatic()
        && skewAxis.isStatic();
}

} // namespace yup
