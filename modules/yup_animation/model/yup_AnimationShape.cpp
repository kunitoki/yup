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
// AnimationPathData

Path AnimationPathData::toPath() const
{
    Path p;

    const int n = static_cast<int> (vertices.size());
    if (n == 0)
        return p;

    p.moveTo (vertices[0]);

    for (int i = 0; i < n - 1; ++i)
    {
        const Point<float> cp1 = vertices[i] + outTangents[i];
        const Point<float> cp2 = vertices[i + 1] + inTangents[i + 1];
        p.cubicTo (cp1, cp2.getX(), cp2.getY(), vertices[i + 1].getX(), vertices[i + 1].getY());
    }

    if (closed && n > 1)
    {
        const Point<float> cp1 = vertices[n - 1] + outTangents[n - 1];
        const Point<float> cp2 = vertices[0] + inTangents[0];
        p.cubicTo (cp1, cp2.getX(), cp2.getY(), vertices[0].getX(), vertices[0].getY());
        p.close();
    }

    return p;
}

AnimationPathData AnimationPathData::lerp (const AnimationPathData& a,
                                           const AnimationPathData& b,
                                           float t)
{
    AnimationPathData result;
    result.closed = a.closed;

    const int n = static_cast<int> (a.vertices.size());
    if (n != static_cast<int> (b.vertices.size()))
        return a;

    result.vertices.resize (static_cast<size_t> (n));
    result.inTangents.resize (static_cast<size_t> (n));
    result.outTangents.resize (static_cast<size_t> (n));

    for (int i = 0; i < n; ++i)
    {
        result.vertices[i] = AnimationLerp<Point<float>>::lerp (a.vertices[i], b.vertices[i], t);
        result.inTangents[i] = AnimationLerp<Point<float>>::lerp (a.inTangents[i], b.inTangents[i], t);
        result.outTangents[i] = AnimationLerp<Point<float>>::lerp (a.outTangents[i], b.outTangents[i], t);
    }

    return result;
}

bool AnimationPathData::operator== (const AnimationPathData& o) const noexcept
{
    return closed == o.closed && vertices == o.vertices;
}

bool AnimationPathData::operator!= (const AnimationPathData& o) const noexcept
{
    return ! (*this == o);
}

AnimationPathData AnimationPathData::operator+ (const AnimationPathData& o) const
{
    return lerp (*this, o, 1.0f);
}

AnimationPathData AnimationPathData::operator- (const AnimationPathData& o) const
{
    return lerp (o, *this, -1.0f);
}

AnimationPathData AnimationPathData::operator* (float scalar) const
{
    AnimationPathData result = *this;
    for (auto& v : result.vertices)
        v = v * scalar;
    for (auto& v : result.inTangents)
        v = v * scalar;
    for (auto& v : result.outTangents)
        v = v * scalar;
    return result;
}

//==============================================================================
// EllipseShape

Path EllipseShape::buildPath (float frameNo) const
{
    const Point<float> c = center.getValueAt (frameNo);
    const Size<float> s = size.getValueAt (frameNo);

    Path p;
    p.addEllipse (c.getX() - s.getWidth() * 0.5f,
                  c.getY() - s.getHeight() * 0.5f,
                  s.getWidth(),
                  s.getHeight());

    if (direction == 3)
    {
        // TODO: reverse path winding if needed
    }

    return p;
}

//==============================================================================
// RectShape

Path RectShape::buildPath (float frameNo) const
{
    const Point<float> pos = position.getValueAt (frameNo);
    const Size<float> sz = size.getValueAt (frameNo);
    const float r = roundness.getValueAt (frameNo);

    const float x = pos.getX() - sz.getWidth() * 0.5f;
    const float y = pos.getY() - sz.getHeight() * 0.5f;

    Path p;

    if (r <= 0.0f)
    {
        p.addRectangle (x, y, sz.getWidth(), sz.getHeight());
    }
    else
    {
        const float clampedR = jmin (r, sz.getWidth() * 0.5f, sz.getHeight() * 0.5f);
        p.addRoundedRectangle (x, y, sz.getWidth(), sz.getHeight(), clampedR);
    }

    return p;
}

//==============================================================================
// BezierPathShape

Path BezierPathShape::buildPath (float frameNo) const
{
    return pathData.getValueAt (frameNo).toPath();
}

//==============================================================================
// PolystarShape

Path PolystarShape::buildPath (float frameNo) const
{
    const Point<float> pos = position.getValueAt (frameNo);
    const int numPt = jmax (3, static_cast<int> (points.getValueAt (frameNo)));
    const float oR = outerRadius.getValueAt (frameNo);
    const float oRnd = outerRoundness.getValueAt (frameNo) / 100.0f;
    const float startAngle = degreesToRadians (rotation.getValueAt (frameNo)) - MathConstants<float>::halfPi;

    Path p;

    if (starType == StarType::Polygon)
    {
        // Matches rlottie addPolygon(): exactly numPt outer vertices.
        constexpr float kPolygonMagic = 0.25f;
        const float angleStep = MathConstants<float>::twoPi / static_cast<float> (numPt);

        float prevX = oR * std::cos (startAngle);
        float prevY = oR * std::sin (startAngle);
        p.moveTo (pos.getX() + prevX, pos.getY() + prevY);

        for (int i = 1; i <= numPt; ++i)
        {
            const float angle = startAngle + static_cast<float> (i) * angleStep;
            const float x = oR * std::cos (angle);
            const float y = oR * std::sin (angle);

            if (oRnd > 1e-5f)
            {
                const float cp1Theta = std::atan2 (prevY, prevX) - MathConstants<float>::halfPi;
                const float cp2Theta = std::atan2 (y, x) - MathConstants<float>::halfPi;
                const float tLen = oR * oRnd * kPolygonMagic;
                p.cubicTo (pos.getX() + prevX - tLen * std::cos (cp1Theta),
                           pos.getY() + prevY - tLen * std::sin (cp1Theta),
                           pos.getX() + x + tLen * std::cos (cp2Theta),
                           pos.getY() + y + tLen * std::sin (cp2Theta),
                           pos.getX() + x,
                           pos.getY() + y);
            }
            else
            {
                p.lineTo (pos.getX() + x, pos.getY() + y);
            }

            prevX = x;
            prevY = y;
        }
    }
    else // StarType::Star
    {
        const float iR = innerRadius.getValueAt (frameNo);
        const float iRnd = innerRoundness.getValueAt (frameNo) / 100.0f;

        // Matches rlottie addPolystar(): numPt*2 vertices alternating outer/inner.
        // POLYSTAR_MAGIC_NUMBER = 0.47829 / 0.28 (rlottie constant for bezier circle approx).
        constexpr float kPolystarMagic = 0.47829f / 0.28f;
        const float halfStep = MathConstants<float>::twoPi / static_cast<float> (numPt * 2);
        const bool hasRoundness = (oRnd > 1e-5f || iRnd > 1e-5f);

        float prevX = oR * std::cos (startAngle);
        float prevY = oR * std::sin (startAngle);
        p.moveTo (pos.getX() + prevX, pos.getY() + prevY);

        // longSegment tracks whether the PREVIOUS point was outer (true) or inner (false).
        // The first loop point is inner (coming from the outer moveTo).
        bool longSegment = false;

        for (int i = 1; i <= numPt * 2; ++i)
        {
            const float angle = startAngle + static_cast<float> (i) * halfStep;
            const float radius = longSegment ? oR : iR;
            const float x = radius * std::cos (angle);
            const float y = radius * std::sin (angle);

            if (hasRoundness)
            {
                // cp1 is on the FROM vertex, cp2 on the TO vertex.
                const float cp1Roundness = longSegment ? iRnd : oRnd;
                const float cp2Roundness = longSegment ? oRnd : iRnd;
                const float cp1Radius = longSegment ? iR : oR;
                const float cp2Radius = longSegment ? oR : iR;

                const float cp1Theta = std::atan2 (prevY, prevX) - MathConstants<float>::halfPi;
                const float cp2Theta = std::atan2 (y, x) - MathConstants<float>::halfPi;
                const float fn = static_cast<float> (numPt);
                const float cp1Len = cp1Radius * cp1Roundness * kPolystarMagic / fn;
                const float cp2Len = cp2Radius * cp2Roundness * kPolystarMagic / fn;

                p.cubicTo (pos.getX() + prevX - cp1Len * std::cos (cp1Theta),
                           pos.getY() + prevY - cp1Len * std::sin (cp1Theta),
                           pos.getX() + x + cp2Len * std::cos (cp2Theta),
                           pos.getY() + y + cp2Len * std::sin (cp2Theta),
                           pos.getX() + x,
                           pos.getY() + y);
            }
            else
            {
                p.lineTo (pos.getX() + x, pos.getY() + y);
            }

            prevX = x;
            prevY = y;
            longSegment = ! longSegment;
        }
    }

    p.close();
    return p;
}

} // namespace yup
