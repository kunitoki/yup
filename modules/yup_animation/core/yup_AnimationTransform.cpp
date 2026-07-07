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
    const float r = is3DData ? rotationZ.getValueAt (frameNo) : rotation.getValueAt (frameNo);
    const float sk = skew.getValueAt (frameNo);
    const float sa = skewAxis.getValueAt (frameNo);

    const Point<float> p = positionAt (frameNo);

    // Compose: translate(p) * rotate(r) * skew * scale(s/100) * translate(-a)
    AffineTransform t;
    t = t.translated (-a.getX(), -a.getY());
    t = t.scaled (s.getWidth() / 100.0f, s.getHeight() / 100.0f);

    if (is3DData)
    {
        const float rx = degreesToRadians (rotationX.getValueAt (frameNo));
        const float ry = degreesToRadians (rotationY.getValueAt (frameNo));
        // Approximate 3D rotation via scale — cos(angle) on the perpendicular axis
        if (std::abs (rx) > 1e-5f)
            t = t.scaled (1.0f, std::cos (rx));
        if (std::abs (ry) > 1e-5f)
            t = t.scaled (std::cos (ry), 1.0f);
    }

    if (std::abs (sk) > 1e-5f)
    {
        const float skRad = degreesToRadians (-sk);
        const float saRad = degreesToRadians (sa);
        const AffineTransform skewMtx = AffineTransform (1.0f, std::tan (skRad), 0.0f, 0.0f, 1.0f, 0.0f);
        const AffineTransform rot1 = AffineTransform::rotation (saRad);
        const AffineTransform rot2 = AffineTransform::rotation (-saRad);
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
        && skewAxis.isStatic()
        && rotationX.isStatic()
        && rotationY.isStatic()
        && rotationZ.isStatic()
        && spatialKeyframes.empty();
}

Point<float> AnimationTransform::positionAt (float frameNo) const
{
    if (separatePosition)
        return { positionX.getValueAt (frameNo), positionY.getValueAt (frameNo) };

    if (spatialKeyframes.empty())
        return position.getValueAt (frameNo);

    // Spatial bezier interpolation with position tangents
    if (frameNo <= spatialKeyframes.front().frame)
        return spatialKeyframes.front().value;

    if (frameNo >= spatialKeyframes.back().frame)
        return spatialKeyframes.back().endValue.value_or (spatialKeyframes.back().value);

    int lo = 0;
    int hi = static_cast<int> (spatialKeyframes.size()) - 2;
    while (lo < hi)
    {
        const int mid = (lo + hi + 1) / 2;
        if (spatialKeyframes[static_cast<size_t> (mid)].frame <= frameNo)
            lo = mid;
        else
            hi = mid - 1;
    }

    const auto& k0 = spatialKeyframes[static_cast<size_t> (lo)];
    const auto& k1 = spatialKeyframes[static_cast<size_t> (lo + 1)];

    const float span = k1.frame - k0.frame;
    if (span < 1e-6f)
        return k0.endValue.value_or (k1.value);

    // Apply temporal easing
    const float t = k0.easing.isHold() ? 0.0f : k0.easing.evaluate ((frameNo - k0.frame) / span);

    // Match rlottie: temporal easing gives progress, then spatial bezier is
    // sampled at the corresponding arc length rather than raw parameter t.
    const auto P0 = k0.value;
    const auto P1 = k0.value + k0.tangentOut;
    const auto P3 = k0.endValue.value_or (k1.value);
    const auto P2 = P3 + k1.tangentIn;

    const auto bezier = CubicBezier::fromPoints (P0, P1, P2, P3);
    return bezier.pointAt (bezier.tAtLength (t * bezier.length()));
}

} // namespace yup
